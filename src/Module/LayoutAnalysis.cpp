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

void LayoutAnalysisConfig::load(const QSettings & settings) {
}

void LayoutAnalysisConfig::save(QSettings & settings) const {
}

// LayoutAnalysis --------------------------------------------------------------------
LayoutAnalysis::LayoutAnalysis(const cv::Mat& img) {

	mImg = img;

	mConfig = QSharedPointer<LayoutAnalysisConfig>::create();
	mConfig->loadSettings();
	mConfig->saveDefaultSettings();
}

bool LayoutAnalysis::isEmpty() const {
	return mImg.empty();
}

bool LayoutAnalysis::compute() {

	if (!checkInput())
		return false;

	Timer dt;

	// find super pixels
	//rdf::SuperPixel spM(mImg);
	rdf::ScaleSpaceSuperPixel spM(mImg);

	if (!spM.compute()) {
		qWarning() << "could not compute super pixels!";
		return false;
	}

	// TODO: automatically find text blocks

	// create an 'all-in' text block
	if (mTextBlockSet.isEmpty()) {
		Rect r(Vector2D(), mImg.size());
		mTextBlockSet << Polygon::fromRect(r);
	}

	PixelSet pixels = spM.superPixels();
	mTextBlockSet.setPixels(pixels);

	// compute text lines for each text block
	for (QSharedPointer<TextBlock> tb : mTextBlockSet.textBlocks()) {

		PixelSet sp = tb->pixelSet();

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
		rdf::TextLineSegmentation textLines(sp);

		if (!textLines.compute()) {
			qWarning() << "could not compute text line segmentation!";
			return false;
		}

		// save text lines
		tb->setTextLines(textLines.textLineSets());
	}
	
	return true;
}

QSharedPointer<LayoutAnalysisConfig> LayoutAnalysis::config() const {
	return qSharedPointerDynamicCast<LayoutAnalysisConfig>(mConfig);
}

cv::Mat LayoutAnalysis::draw(const cv::Mat & img) const {

	QPixmap pm = Image::mat2QPixmap(img);

	QPainter p(&pm);
	
	for (auto tb : mTextBlockSet.textBlocks())
		tb->draw(p);

	return Image::qPixmap2Mat(pm);
}

QString LayoutAnalysis::toString() const {
	return Module::toString();
}

void LayoutAnalysis::setTextRegions(const QVector<QSharedPointer<Region> >& regions) {

	QVector<QSharedPointer<Region> > textRegions;

	for (auto r : regions) {

		if (r->type() == Region::type_text_region)
			textRegions << r;
	}

	mTextBlockSet = TextBlockSet(textRegions);
}

TextBlockSet LayoutAnalysis::textBlockSet() const {
	return mTextBlockSet;
}

bool LayoutAnalysis::checkInput() const {

	return !isEmpty();
}


}