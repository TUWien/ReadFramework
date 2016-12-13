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

#pragma warning(push, 0)	// no warnings from includes
#include <QDebug>
#include <QPainter>
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


void TextLineConfig::load(const QSettings & settings) {

	mMinLineLength = settings.value("minLineLength", minLineLength()).toInt();
	mMinPointDist = settings.value("minPointDistance", minPointDistance()).toDouble();
	mErrorMultiplier = settings.value("errorMultiplier", errorMultiplier()).toDouble();
}

void TextLineConfig::save(QSettings & settings) const {

	settings.setValue("minLineLength", minLineLength());
	settings.setValue("minPointDistance", minPointDistance());
	settings.setValue("errorMultiplier", errorMultiplier());
}

// TextLineSegmentation --------------------------------------------------------------------
TextLineSegmentation::TextLineSegmentation(const PixelSet& set) {

	mSet = set;
	mConfig = QSharedPointer<TextLineConfig>::create();
	mConfig->loadSettings();
	mConfig->saveDefaultSettings();
}

bool TextLineSegmentation::isEmpty() const {
	return mSet.isEmpty();
}

bool TextLineSegmentation::compute() {

	Timer dt;

	if (!checkInput())
		return false;

	//RegionPixelConnector rpc;
	//rpc.setLineSpacingMultiplier(2.0);
	//DelauneyPixelConnector rpc;
	//QVector<QSharedPointer<PixelEdge> > pEdges = rpc.connect(mSet.pixels());

	PixelGraph pg(mSet);
	pg.connect(rdf::DelauneyPixelConnector(), PixelGraph::sort_line_edges);

	mTextLines = clusterTextLines(pg);

	//QVector<QSharedPointer<TextLineSet> > ps;
	//for (auto p : mTextLines) {
	//	if (p->size() > 10)
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

QVector<QSharedPointer<TextLineSet> > TextLineSegmentation::clusterTextLines(const PixelGraph & graph) const {

	// TODOs
	// - distance sorting (w.r.t angles) has an issue
	// - addPixel: do not add _all_
	// - merge textlines: check axis ratio (it's basically PCA...)

	QVector<QSharedPointer<TextLineSet> > textLines;

	// debug ------------------------------------
	QString fp("C:/temp/cluster/");
	//Vector2D maxSize = graph.set().boundingBox().bottomRight();
	
	QImage img("D:/read/test/synthetic-test.png");


	//QPixmap pm(maxSize.toQSize());
	//QPixmap pm(QSize(800, 446));
	QPainter p(&img);
	p.setBrush(ColorManager::white(0.5));
	p.drawRect(img.rect());
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
		else
			p.setPen(ColorManager::red(0.4));

		// else drop

		// debug --------------------------------
		e->draw(p);

		if (idx % 100 == 0) {
			cv::Mat imgCv = Image::qImage2Mat(img);
			QString iPath = fp + "img" + QString::number(idx) + ".tif";
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

cv::Mat TextLineSegmentation::draw(const cv::Mat& img) const {

	// draw mser blobs
	Timer dtf;
	QPixmap pm = Image::mat2QPixmap(img);

	QPainter p(&pm);
	
	// this block draws the edges
	Drawer::instance().setColor(ColorManager::darkGray(0.4));
	p.setPen(Drawer::instance().pen());

	for (auto b : mEdges) {
		b->draw(p);
	}

	// show the stop lines
	Drawer::instance().setColor(ColorManager::red(0.4));
	p.setPen(Drawer::instance().pen());

	for (auto l : mStopLines)
		l.draw(p);

	//auto sets = toSets();
	
	for (const QSharedPointer<TextLineSet>& set : mTextLines) {
		Drawer::instance().setColor(ColorManager::getColor());
		p.setPen(Drawer::instance().pen());
		//set->draw(p, (int)PixelSet::draw_pixels, Pixel::draw_ellipse);

		Line baseLine = set->fitLine();
		baseLine.setThickness(6.0);
		
		//if (baseLine.length() > 300)
		baseLine.draw(p);

		//for (auto pixel : set->pixels()) {
		//	pixel->draw(p, .3, Pixel::draw_ellipse_only);
		//}
	}

	//mDebug << mEdges.size() << "edges drawn in" << dtf;

	return Image::qPixmap2Mat(pm);
}

QString TextLineSegmentation::toString() const {
	return Module::toString();
}

void TextLineSegmentation::addLines(const QVector<Line>& lines) {
	mStopLines << lines;
}

QVector<QSharedPointer<TextLine>> TextLineSegmentation::textLines() const {
	
	QVector<QSharedPointer<TextLine>> tls;
	for (auto set : mTextLines)
		tls << set->toTextLine();

	return tls;
}

bool TextLineSegmentation::checkInput() const {

	return !mSet.isEmpty();
}

}