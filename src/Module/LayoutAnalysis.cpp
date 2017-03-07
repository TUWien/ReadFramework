/*******************************************************************************************************
 ReadFramework is the basis for modules developed at CVL/TU Wien for the EU project READ. 
  
 Copyright (C) 2016 Markus Diem <diem@caa.tuwien.ac.at>
 Copyright (C) 2016 Stefan Fiel <fiel@caa.tuwien.ac.at>
 Copyright (C) 2016 Florian Kleber <kleber@caa.tuwien.ac.at>

 This file is part of ReadFramework.

 ReadFramework is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 ReadFramework is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.

 The READ project  has  received  funding  from  the European  Union’s  Horizon  2020  
 research  and innovation programme under grant agreement No 674943
 
 related links:
 [1] http://www.caa.tuwien.ac.at/cvl/
 [2] https://transkribus.eu/Transkribus/
 [3] https://github.com/TUWien/
 [4] http://nomacs.org
 *******************************************************************************************************/

#include "LayoutAnalysis.h"

#include "Image.h"

#include "Utils.h"
#include "SuperPixel.h"
#include "TextLineSegmentation.h"
#include "Elements.h"
#include "ElementsHelper.h"
#include "PageParser.h"

#pragma warning(push, 0)	// no warnings from includes
 // Qt Includes
#pragma warning(pop)

namespace rdf {


// LayoutAnalysisConfig --------------------------------------------------------------------
LayoutAnalysisConfig::LayoutAnalysisConfig() : ModuleConfig("Layout Analysis Module") {
}

QString LayoutAnalysisConfig::toString() const {
	return ModuleConfig::toString();
}

void LayoutAnalysisConfig::setMaxImageSide(int maxSide) {
	mMaxImageSide = maxSide;
}

int LayoutAnalysisConfig::maxImageSide() const {
	return ModuleConfig::checkParam(mMaxImageSide, -1, INT_MAX, "maxImageSide");
}

void LayoutAnalysisConfig::setScaleMode(const ScaleSideMode & mode) {
	mScaleMode = mode;
}

int LayoutAnalysisConfig::scaleMode() const {
	return ModuleConfig::checkParam(mScaleMode, 0, (int)scale_end, "scaleMode");
}

void LayoutAnalysisConfig::setRemoveWeakTextLiens(bool remove) {
	mRemoveWeakTextLines = remove;
}

bool LayoutAnalysisConfig::removeWeakTextLines() const {
	return mRemoveWeakTextLines;
}

void LayoutAnalysisConfig::setMinSuperPixelsPerBlock(int minPx) {
	mMinSuperPixelsPerBlock = minPx;
}

int LayoutAnalysisConfig::minSuperixelsPerBlock() const {
	return ModuleConfig::checkParam(mMinSuperPixelsPerBlock, 0, INT_MAX, "minSuperPixelsPerBlock");
}

void LayoutAnalysisConfig::load(const QSettings & settings) {

	mMaxImageSide			= settings.value("maxImageSide", maxImageSide()).toInt();
	mScaleMode				= settings.value("scaleMode", scaleMode()).toInt();
	mMinSuperPixelsPerBlock	= settings.value("minSuperPixelsPerBlock", minSuperixelsPerBlock()).toInt();
	mRemoveWeakTextLines	= settings.value("removeWeakTextLines", removeWeakTextLines()).toBool();
}

void LayoutAnalysisConfig::save(QSettings & settings) const {
	
	settings.setValue("maxImageSide", maxImageSide());
	settings.setValue("scaleMode", scaleMode());
	settings.setValue("minSuperPixelsPerBlock", minSuperixelsPerBlock());
	settings.setValue("removeWeakTextLines", removeWeakTextLines());
}

// LayoutAnalysis --------------------------------------------------------------------
LayoutAnalysis::LayoutAnalysis(const cv::Mat& img) {

	mImg = img;

	mConfig = QSharedPointer<LayoutAnalysisConfig>::create();
	mConfig->loadSettings();
}

bool LayoutAnalysis::isEmpty() const {
	return mImg.empty();
}

bool LayoutAnalysis::compute() {

	if (!checkInput())
		return false;

	Timer dt;

	mScale = scaleFactor();

	// resize if necessary
	if (mScale != 1.0) {
		cv::resize(mImg, mImg, cv::Size(), mScale, mScale, CV_INTER_LINEAR);
		qInfo() << "image resized, new dimension" << mImg.cols << "x" << mImg.rows << "scale factor:" << mScale;
	}

	// find super pixels
	//rdf::SuperPixel spM(mImg);
	rdf::ScaleSpaceSuperPixel spM(mImg);

	if (!spM.compute()) {
		mWarning << "could not compute super pixels!";
		return false;
	}

	// TODO: automatically find text blocks

	// create an 'all-in' text block
	mTextBlockSet = createTextBlocks();

	if (mTextBlockSet.isEmpty()) {
		Rect r(Vector2D(), mImg.size());
		mTextBlockSet << Polygon::fromRect(r);
	}

	PixelSet pixels = spM.superPixels();
	
	// scale back to original coordinates
	mTextBlockSet.setPixels(pixels);

	QVector<Line> stopLines = createStopLines();

	// compute text lines for each text block
	for (QSharedPointer<TextBlock> tb : mTextBlockSet.textBlocks()) {

		PixelSet sp = tb->pixelSet();

		if (sp.isEmpty()) {
			qInfo() << tb << "is empty...";
			continue;
		}

		// find local orientation per pixel
		rdf::LocalOrientation lo(sp);
		if (!lo.compute()) {
			qWarning() << "could not compute local orientation";
			return false;
		}

		// smooth estimation
		rdf::GraphCutOrientation pse(sp);

		if (!pse.compute()) {
			qWarning() << "could not compute set orientation";
			return false;
		}

		//// find tab stops
		//rdf::TabStopAnalysis tabStops(sp);
		//if (!tabStops.compute())
		//	qWarning() << "could not compute text block segmentation!";

		// find text lines
		QVector<QSharedPointer<TextLineSet> > textLines;
		if (sp.size() > config()->minSuperixelsPerBlock()) {

			rdf::TextLineSegmentation tlM(sp);
			tlM.addSeparatorLines(stopLines);

			if (!tlM.compute()) {
				qWarning() << "could not compute text line segmentation!";
				return false;
			}

			// save text lines
			textLines = tlM.textLineSets();
		}

		// paragraph is a single textline
		if (textLines.empty()) {
			textLines << QSharedPointer<TextLineSet>(new TextLineSet(sp.pixels()));
		}

		tb->setTextLines(textLines);
	}
	
	// scale back to original coordinates
	mTextBlockSet.scale(1.0 / mScale);

	// clean-up
	if (config()->removeWeakTextLines()) {
		mTextBlockSet.removeWeakTextLines();
	}

	return true;
}

QSharedPointer<LayoutAnalysisConfig> LayoutAnalysis::config() const {
	return qSharedPointerDynamicCast<LayoutAnalysisConfig>(mConfig);
}

cv::Mat LayoutAnalysis::draw(const cv::Mat & img) const {

	QPixmap pm = Image::mat2QPixmap(img);
	QPainter p(&pm);
	
	for (auto tb : mTextBlockSet.textBlocks()) {
		p.setPen(ColorManager::getColor());
		tb->draw(p, (TextBlock::DrawFlag)(TextBlock::draw_text_lines /*| TextBlock::draw_pixels*/));
	}

	//// LSD OpenCV --------------------------------------------------------------------
	//Timer dt;
	//
	//double scale = 4.0;
	//cv::Mat lImg;
	//cv::resize(mImg, lImg, cv::Size(), 1.0/scale, 1.0/scale);
	//cv::line_descriptor::LSDDetector lsd;
	//std::vector<cv::line_descriptor::KeyLine> lines;
	//lsd.detect(lImg, lines, 2, 1);
	//
	//qDebug() << lines.size() << "lines detected in" << dt;

	//p.setPen(ColorManager::red());
	//for (auto kl : lines) {
	//	
	//	Line l(kl.getStartPoint(), kl.getEndPoint());
	//	l.scale(scale);
	//	l.draw(p);
	//}

	//// LSD Flo --------------------------------------------------------------------
	//
	//cv::Mat imgIn = mImg;
	////cv::resize(imgIn, imgIn, cv::Size(), 0.5, 0.5);
	//
	//Timer dtl;
	//cv::Mat imgInG = imgIn;
	//if (imgIn.channels() != 1) 
	//	cv::cvtColor(imgIn, imgInG, CV_RGB2GRAY);

	//ReadLSD lsdr(imgInG);
	//lsdr.config()->setScale(0.5);

	//if (!lsdr.compute())
	//	qWarning() << "could not compute LSD";

	//qDebug() << "Read LSD lines computed in" << dtl;

	//p.setPen(ColorManager::blue(0.4));

	//for (auto ls : lsdr.lines()) {
	//	
	//	Line l = ls.line();
	//	l.setThickness(1);
	//	l.draw(p);
	//}

	//// old school --------------------------------------------------------------------
	
	return Image::qPixmap2Mat(pm);
}

QString LayoutAnalysis::toString() const {
	return Module::toString();
}

void LayoutAnalysis::setRootRegion(const QSharedPointer<Region>& region) {

	mRoot = region;
}

TextBlockSet LayoutAnalysis::textBlockSet() const {
	return mTextBlockSet;
}

double LayoutAnalysis::scaleFactor() const {

	int cms = config()->maxImageSide();

	if (cms > 0) {

		if (cms < 500) {
			mWarning << "you chose the maximal image side to be" << cms << "px - this is pretty low";
		}

		// find the image side
		int mSide = 0;
		if (config()->scaleMode() == LayoutAnalysisConfig::scale_max_side)
			mSide = qMax(mImg.rows, mImg.cols);
		else
			mSide = mImg.rows;

		double sf = (double)cms / mSide;

		// do not rescale if the factor is close to 1
		if (sf <= 0.95)
			return sf;

		// inform user that we do not resize if the scale factor is close to 1
		if (sf < 1.0)
			mInfo << "I won't resize the image since the scale factor is" << sf;
	}

	return 1.0;
}

bool LayoutAnalysis::checkInput() const {

	return !isEmpty();
}

TextBlockSet LayoutAnalysis::createTextBlocks() const {

	if (mRoot) {
		
		// get (potential) text regions - from XML
		QVector<QSharedPointer<Region> > textRegions = RegionManager::filter<Region>(mRoot, Region::type_text_region);
		TextBlockSet tbs(textRegions);
		tbs.scale(mScale);

		return tbs;
	}

	return TextBlockSet();
}

QVector<Line> LayoutAnalysis::createStopLines() const {

	QVector<Line> stopLines;

	if (mRoot) {

		auto separators = RegionManager::filter<SeparatorRegion>(mRoot, Region::type_separator);

		for (auto s : separators) {
			Line sl = s->line();
			sl.scale(mScale);
			stopLines << sl;
		}

		qDebug() << stopLines.size() << "lines loaded";

	}

	// TODO: detect lines here

	return stopLines;
}


}