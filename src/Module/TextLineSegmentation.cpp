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

#include "TextLineSegmentation.h"

#include "Utils.h"
#include "Image.h"
#include "Drawer.h"
#include "Algorithms.h"
#include "ImageProcessor.h"

#pragma warning(push, 0)	// no warnings from includes
#include <QDebug>
#include <QPainter>
#include <QFileInfo>
#include <QDir>
#pragma warning(pop)

namespace rdf {

// TextLineConfig --------------------------------------------------------------------
TextLineConfig::TextLineConfig() : ModuleConfig("Text Line Module") {
}

QString TextLineConfig::toString() const {
	return ModuleConfig::toString();
}

void TextLineConfig::setMinLineLength(int length) {

	mMinLineLength = length;
}

int TextLineConfig::minLineLength() const {
	return checkParam(mMinLineLength, 0, INT_MAX, "minLineLength");
}

void TextLineConfig::setMinPointDistance(double dist) {
	mMinPointDist = dist;
}

double TextLineConfig::minPointDistance() const {
	return checkParam(mMinPointDist, 0.0, DBL_MAX, "minPointDist");
}

void TextLineConfig::setErrorMultiplier(double multiplier) {
	mErrorMultiplier = multiplier;
}

double TextLineConfig::errorMultiplier() const {
	return checkParam(mErrorMultiplier, 0.0, DBL_MAX, "errorMultiplier");
}

QString TextLineConfig::debugPath() const {

	return mDebugPath;
}


void TextLineConfig::load(const QSettings & settings) {

	mMinLineLength = settings.value("minLineLength", minLineLength()).toInt();
	mMinPointDist = settings.value("minPointDistance", minPointDistance()).toDouble();
	mErrorMultiplier = settings.value("errorMultiplier", errorMultiplier()).toDouble();
	mDebugPath = settings.value("debugPath", debugPath()).toString();
}

void TextLineConfig::save(QSettings & settings) const {

	settings.setValue("minLineLength", minLineLength());
	settings.setValue("minPointDistance", minPointDistance());
	settings.setValue("errorMultiplier", errorMultiplier());
	settings.setValue("debugPath", debugPath());
}

// TextLineSegmentation --------------------------------------------------------------------
TextLineSegmentation::TextLineSegmentation(const PixelSet& set) {

	mSet = set;
	mConfig = QSharedPointer<TextLineConfig>::create();
	mConfig->loadSettings();

}

bool TextLineSegmentation::isEmpty() const {
	return mSet.isEmpty();
}

bool TextLineSegmentation::compute() {

	return compute(cv::Mat());
}

bool TextLineSegmentation::compute(const cv::Mat& img) {

	Timer dt;

	if (!checkInput())
		return false;

	filterDuplicates(mSet);

	// create delauney graph
	DelauneyPixelConnector dpc;
	dpc.setStopLines(mStopLines);

	PixelGraph pg(mSet);
	pg.connect(dpc, PixelGraph::sort_line_edges);
	// TODO: add stop lines here...

	if (img.empty())
		mTextLines = clusterTextLines(pg/*, &mRemovedEdges*/);
	else
		mTextLines = clusterTextLinesDebug(pg, img);

	mergeUnstableTextLines(mTextLines);

	// there is still a warning for small textlines
	//QVector<QSharedPointer<TextLineSet> > ps;
	//for (auto p : mTextLines) {
	//	if (p->size() > 3)
	//		ps << p;
	//}
	//mTextLines = ps;

	mDebug << mTextLines.size() << "text lines (after filtering)";
	mDebug << "computed in" << dt;

	return true;
}

QSharedPointer<TextLineConfig> TextLineSegmentation::config() const {
	return qSharedPointerDynamicCast<TextLineConfig>(mConfig);
}

QVector<QSharedPointer<TextLineSet> > TextLineSegmentation::clusterTextLines(const PixelGraph & graph, QVector<QSharedPointer<PixelEdge> >* removedEdges) const {
	
	QVector<QSharedPointer<TextLineSet> > textLines;
	
	int idx = 0;

	for (auto e : graph.edges()) {

		double heat = 1.0 - (++idx / (double)graph.edges().size());

		int psIdx1 = locate(e->first(), textLines);
		int psIdx2 = locate(e->second(), textLines);

		bool drop = false;

		// create a new text line
		if (psIdx1 == -1 && psIdx2 == -1) {

			// let's call it a pair & create a new text line
			QVector<QSharedPointer<Pixel> > px;
			px << e->first();
			px << e->second();
			textLines << QSharedPointer<TextLineSet>::create(px);
		}
		// already clustered -> nothing todo
		else if (psIdx1 == psIdx2) {
			// this is nothing
		}
		// merge one pixel
		else if (psIdx2 == -1) {

			if (addPixel(textLines[psIdx1], e->second(), heat)) {
				textLines[psIdx1]->add(e->second());
			}
			// else drop
			else
				drop = true;
		}
		// merge one pixel
		else if (psIdx1 == -1) {
			if (addPixel(textLines[psIdx2], e->first(), heat)) {
				textLines[psIdx2]->add(e->first());
			}
			// else drop
			else
				drop = true;
		}
		// merge to same text line
		else if (mergeTextLines(textLines[psIdx1], textLines[psIdx2], heat)) {

			textLines[psIdx2]->append(textLines[psIdx1]->pixels());
			textLines.remove(psIdx1);
		}
		// else drop
		else
			drop = true;

		// remember removed edges
		if (removedEdges && drop)
			*removedEdges << e;
	}

	return textLines;
}

QVector<QSharedPointer<TextLineSet> > TextLineSegmentation::clusterTextLinesDebug(const PixelGraph & graph, const cv::Mat& img) const {

	QVector<QSharedPointer<TextLineSet> > textLines;

	// debug ------------------------------------
	QImage imgR = Image::mat2QImage(img);
	imgR = imgR.convertToFormat(QImage::Format_ARGB32);
	
	QPainter p(&imgR);
	p.setBrush(ColorManager::white(0.5));
	p.drawRect(imgR.rect());
	QPen pen = p.pen();
	pen.setWidth(3);
	p.setPen(pen);
	// debug ------------------------------------

	int idx = 0;

	for (auto e : graph.edges()) {

		double heat = 1.0 - (++idx / (double)graph.edges().size());
		//qDebug() << "heat" << heat;

		int psIdx1 = locate(e->first(), textLines);
		int psIdx2 = locate(e->second(), textLines);

		// create a new text line
		if (psIdx1 == -1 && psIdx2 == -1) {

			//if (e->first()->center() == e->second()->center()) {
			//	qInfo() << "pixels have the same center - skipping";
			//	continue;
			//}
			
			// let's call it a pair & create a new text line
			QVector<QSharedPointer<Pixel> > px;
			px << e->first();
			px << e->second();
			textLines << QSharedPointer<TextLineSet>::create(px);

			p.setPen(ColorManager::blue(1.0));
		}
		// already clustered -> nothing todo
		else if (psIdx1 == psIdx2) {
			p.setPen(ColorManager::blue(1.0));
		}
		// merge one pixel
		else if (psIdx2 == -1) {

			if (addPixel(textLines[psIdx1], e->second(), heat)) {
				textLines[psIdx1]->add(e->second());
				p.setPen(ColorManager::blue(1.0));
			}
			else
				p.setPen(ColorManager::red(0.4));
		}
		// merge one pixel
		else if (psIdx1 == -1) {
			if (addPixel(textLines[psIdx2], e->first(), heat)) {
				textLines[psIdx2]->add(e->first());
				p.setPen(ColorManager::blue(1.0));
			}
			else
				p.setPen(ColorManager::red(0.4));
		}
		// merge same text line
		else if (mergeTextLines(textLines[psIdx1], textLines[psIdx2], heat)) {

			textLines[psIdx2]->append(textLines[psIdx1]->pixels());
			textLines.remove(psIdx1);

			p.setPen(ColorManager::blue(1.0));
		}
		// else drop
		else
			p.setPen(ColorManager::red(0.4));

		// debug --------------------------------
		e->draw(p);

		if (idx % 200 == 0) {
			cv::Mat imgCv = Image::qImage2Mat(imgR);
			QString fName("img" + QString::number(idx) + ".tif");
			QString iPath = QFileInfo(QDir(config()->debugPath()), fName).absoluteFilePath();
			Image::save(imgCv, iPath);
			qDebug() << iPath << "written...";
		}

		//qDebug() << "# textlines" << textLines.size();
		// debug --------------------------------
	}

	return textLines;
}

int TextLineSegmentation::locate(const QSharedPointer<Pixel>& pixel, const QVector<QSharedPointer<TextLineSet> >& sets) const {

	assert(pixel);

	for (int idx = 0; idx < sets.size(); idx++) {

		if (sets[idx]->contains(pixel))
			return idx;
	}

	Vector2D c = pixel->center();
	for (int idx = 0; idx < sets.size(); idx++) {

		assert(sets[idx]);
		Line l = sets[idx]->line();

		if (l.within(c) && l.distance(c) < 10) {
			sets[idx]->add(pixel);
			return idx;
		}
	}

	return -1;
}

bool TextLineSegmentation::addPixel(QSharedPointer<TextLineSet>& set, const QSharedPointer<Pixel>& pixel, double heat) const {

	// do not create vertical lines
	if (set->line().length() < 10)
		return true;

	double mErr = qMax(set->error() * config()->errorMultiplier(), config()->minPointDistance() * heat);
	double newErr = set->line().distance(pixel->center());

	return newErr < mErr;
}

bool TextLineSegmentation::mergeTextLines(const QSharedPointer<TextLineSet>& tln1, const QSharedPointer<TextLineSet>& tln2, double heat) const {

	// do not merge one and the same textline
	if (tln1 == tln2)
		return false;

	// do not create vertical lines
	if (tln1->line().length() < config()->minLineLength() || 
		tln2->line().length() < config()->minLineLength())
		return true;

	double maxErr1 = qMax(tln1->error() * config()->errorMultiplier(), config()->minPointDistance() * heat);
	double maxErr2 = qMax(tln2->error() * config()->errorMultiplier(), config()->minPointDistance() * heat);

	double nErr1 = tln1->computeError(tln2->centers());
	double nErr2 = tln2->computeError(tln1->centers());
	
	return nErr1 < maxErr1 && nErr2 < maxErr2;
}

void TextLineSegmentation::filterDuplicates(PixelSet & set) const {

	Timer dt;
	double md = config()->minLineLength();

	Rect boxD(Vector2D(), Vector2D(2*md, 2*md));
	auto pxs = set.pixels();

	QVector<QSharedPointer<Pixel> > remPixels;

	for (int idx = 0; idx < set.size(); idx++) {
	
		const QSharedPointer<Pixel> px = pxs[idx];
		
		if (remPixels.contains(px))
			continue;
		
		Rect box = boxD;
		box.move(px->center()-Vector2D(md, md));
		
		for (int i = idx+1; i < set.size(); i++) {

			const QSharedPointer<Pixel> pxi = pxs[i];
			const Vector2D& c = pxi->center();

			// speed up
			if (!box.contains(c))
				continue;

			// skip if it is already 'removed'
			if (remPixels.contains(pxi))
				continue;

			if (PixelDistance::euclidean(px, pxi) < config()->minLineLength()) {

				// remove the smaller one
				remPixels << (px->ellipse().radius() < pxi->ellipse().radius() ? px : pxi);
				break;
			}
		}
	}
	
	for (auto px : remPixels)
		set.remove(px);

	qDebug() << remPixels.size() << "px removed in " << dt;
}

void TextLineSegmentation::mergeUnstableTextLines(QVector<QSharedPointer<TextLineSet> >& textLines) const {

	// parameter - how much do we extend the text line?
	double tlExtFactor = 1.2;

	QVector<QSharedPointer<TextLineSet> > unstable = TextLineHelper::filterAngle(textLines);

	// cache convex hulls
	QVector<Polygon> polys;
	for (const auto tl : textLines) {
		polys << tl->convexHull();
	}

	for (int uIdx = 0; uIdx < unstable.size(); uIdx++) {

		const auto utl = unstable[uIdx];

		// compute left & right points (w.r.t text orientation)
		Ellipse el = utl->fitEllipse();
		Vector2D vl = el.getPoint(0);
		Vector2D vr = el.getPoint(CV_PI);

		// compute extended points
		vl = el.center() + (el.center()-vl)*tlExtFactor;
		vr = el.center() + (el.center()-vr)*tlExtFactor;

		double cErr = DBL_MAX;
		int bestIdx = -1;
		QVector<Vector2D> pts = utl->centers();

		// find merging candidates (textlines that contain the right/left most point)
		for (int idx = 0; idx < textLines.size(); idx++) {

			// do not merge myself
			if (textLines[idx]->id() == utl->id())
				continue;

			// find all candidate textlines
			if (polys[idx].contains(vl) || polys[idx].contains(vr)) {

				double err = textLines[idx]->computeError(pts);

				if (err < cErr) {
					bestIdx = idx;
					cErr = err;
				}
			}
		}

		// merge
		if (bestIdx != -1) {

			if (utl->size() > 2)
				textLines[bestIdx]->append(utl->pixels());

			int rIdx = textLines.indexOf(utl);
			textLines.remove(rIdx);
			polys.remove(rIdx);
		}
	}

}

cv::Mat TextLineSegmentation::draw(const cv::Mat& img) const {

	//QPixmap pm = Image::mat2QPixmap(img);

	//QPainter p(&pm);

	//// show the stop lines
	//Drawer::instance().setColor(ColorManager::red(0.4));
	//p.setPen(Drawer::instance().pen());

	//for (auto l : mStopLines)
	//	l.draw(p);
	//p.end();

	return draw(img, mTextLines);
}

cv::Mat TextLineSegmentation::draw(const cv::Mat& img,  const QVector<QSharedPointer<TextLineSet> >& textLines) {

	QPixmap pm = Image::mat2QPixmap(img);

	QPainter p(&pm);
	
	// draw text lines
	for (const QSharedPointer<TextLineSet>& tl : textLines) {
		Drawer::instance().setColor(ColorManager::getColor());
		p.setPen(Drawer::instance().pen());

		tl->draw(p, (PixelSet::DrawFlag)(PixelSet::draw_poly /*| PixelSet::draw_pixels*/), (Pixel::DrawFlag)(Pixel::draw_stats));

		Vector2D c = tl->center();
		c.setX(c.x() + 20);
		p.drawText(c.toQPointF(), QString::number(tl->density()));
	}

	// draw errored/crucial text lines
	Drawer::instance().setColor(ColorManager::red());
	p.setPen(Drawer::instance().pen());

	for (const auto tl : TextLineHelper::filterAngle(textLines)) {

		tl->draw(p, (PixelSet::DrawFlag)(PixelSet::draw_rect | PixelSet::draw_pixels), (Pixel::DrawFlag)(/*Pixel::draw_ellipse |*/ Pixel::draw_stats));
	}

	//Drawer::instance().setColor(ColorManager::red());
	//p.setPen(Drawer::instance().pen());

	//for (const auto e : mRemovedEdges)
	//	e->draw(p);


	return Image::qPixmap2Mat(pm);
}

QString TextLineSegmentation::toString() const {
	return Module::toString();
}

void TextLineSegmentation::addSeparatorLines(const QVector<Line>& lines) {
	mStopLines << lines;
}

QVector<QSharedPointer<TextLine>> TextLineSegmentation::textLines() const {
	
	QVector<QSharedPointer<TextLine>> tls;
	for (auto set : mTextLines)
		tls << set->toTextLine();

	return tls;
}

QVector<QSharedPointer<TextLineSet>> TextLineSegmentation::textLineSets() const {
	return mTextLines;
}

void TextLineSegmentation::scale(double s) {

	//for (auto e : mRemovedEdges)
	//	e->scale(s);

	for (auto tl : mTextLines)
		tl->scale(s);

	for (Line& sl : mStopLines)
		sl.scale(s);
}

bool TextLineSegmentation::checkInput() const {

	return !mSet.isEmpty();
}

}