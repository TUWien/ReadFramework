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

double TextLineConfig::minDistFactor() const {
	return mMinDistFactor;
}

void TextLineConfig::setMinDistFactor(double val) {
	mMinDistFactor = val;
}

void TextLineConfig::load(const QSettings & settings) {
	mMinDistFactor = settings.value("minDistFactor", minDistFactor()).toDouble();
}

void TextLineConfig::save(QSettings & settings) const {

	settings.setValue("minDistFactor", minDistFactor());
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

	//qDebug() << "# edges: " << pEdges.size();

	//QVector<QSharedPointer<LineEdge> > mEdges;
	//for (const QSharedPointer<PixelEdge>& pe : pEdges)
	//	mEdges << QSharedPointer<LineEdge>(new LineEdge(*pe));

	//mEdges = filterEdges(mEdges, config()->minDistFactor());

	//if (!mStopLines.empty())
	//	mEdges = filterEdges(mEdges, mStopLines);

	//mTextLines = toSets(mEdges);

	//mDebug << mTextLines.size() << "text lines found";
	//mTextLines = merge(mTextLines, 160);
	//mDebug << mTextLines.size() << "text lines (after merging)";

	QVector<QSharedPointer<TextLineSet> > ps;
	//for (auto p : mTextLines) {
	//	if (p->size() > 10)
	//		ps << p;
	//}
	mTextLines = ps;

	//mTextLines = filter(mTextLines);
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
	Vector2D maxSize = graph.set().boundingBox().bottomRight();
	QPixmap pm(maxSize.toQSize());
	QPainter p(&pm);
	
	QPen goodPen(ColorManager::lightGray(0.8));
	goodPen.setWidth(2);

	QPen badPen(ColorManager::red(0.8));
	badPen.setWidth(2);

	int mIdx = 0;
	// debug ------------------------------------

	int idx = 0;

	for (auto e : graph.edges()) {

		double heat = std::sqrt(++idx / (double)graph.edges().size());
		//qDebug() << "heat" << heat;

		int psIdx1 = locate(e->first(), textLines);
		int psIdx2 = locate(e->second(), textLines);

		// create a new text line
		if (psIdx1 == -1 && psIdx2 == -1) {

			if (e->first()->center() == e->second()->center()) {
				qInfo() << "pixels have the same center - skipping";
				continue;
			}
			
			QVector<QSharedPointer<Pixel> > px;
			px << e->first();
			px << e->second();

			QSharedPointer<TextLineSet> ps = QSharedPointer<TextLineSet>::create(px);
			textLines << ps;

			p.setPen(ColorManager::darkGray(0.8));
		}
		else if (psIdx2 == -1) {

			if (addPixel(textLines[psIdx1], e->second(), heat)) {
				textLines[psIdx1]->add(e->second());
				p.setPen(ColorManager::lightGray(0.8));
			}
			else
				p.setPen(ColorManager::red(0.8));
		}
		else if (psIdx1 == -1) {
			if (addPixel(textLines[psIdx2], e->first(), heat)) {
				textLines[psIdx2]->add(e->first());
				p.setPen(ColorManager::lightGray(0.8));
			}
			else
				p.setPen(ColorManager::red(0.8));
		}
		else if (mergeTextLines(textLines[psIdx1], textLines[psIdx2], heat)) {

			textLines[psIdx1]->append(textLines[psIdx2]->pixels());
			textLines.remove(psIdx2);
			p.setPen(ColorManager::blue(0.8));
		}
		else
			p.setPen(ColorManager::red(1.0));

		// else drop

		// debug --------------------------------
		e->draw(p);

		//p.setPen(ColorManager::getColor(1));

		//Line l1(e->first()->center(), e->first()->center() + (e->first()->stats()->orVec()*20.0));
		//Line l2(e->second()->center(), e->second()->center() + (e->second()->stats()->orVec()*20.0));
		//l1.draw(p);
		//l2.draw(p);
		
		if (mIdx % 100 == 0) {
			cv::Mat img = Image::qImage2Mat(pm.toImage());
			QString iPath = fp + "img" + QString::number(mIdx) + ".png";
			Image::save(img, iPath);
			qDebug() << iPath << "written...";
		}
		mIdx++;
		// debug --------------------------------
	}

	return textLines;
}

int TextLineSegmentation::locate(const QSharedPointer<Pixel>& pixel, const QVector<QSharedPointer<TextLineSet> >& sets) const {

	for (int idx = 0; idx < sets.size(); idx++) {

		if (sets[idx]->contains(pixel))
			return idx;
	}

	return -1;
}

bool TextLineSegmentation::addPixel(QSharedPointer<TextLineSet>& set, const QSharedPointer<Pixel>& pixel, double heat) const {

	// do not create vertical lines
	if (set->line().length() < 10)
		return true;

	double mErr = qMax(set->error() * 2.0, 10.0);
	double newErr = set->line().distance(pixel->center());

	return newErr < mErr;
}

bool TextLineSegmentation::mergeTextLines(const QSharedPointer<TextLineSet>& tln1, const QSharedPointer<TextLineSet>& tln2, double heat) const {

	// do not create vertical lines
	if (tln1->line().length() < 10 || tln2->line().length() < 10)
		return true;

	double maxErr1 = qMax(tln1->error() * 2.0, 20.0);
	double maxErr2 = qMax(tln2->error() * 2.0, 20.0);

	double nErr1 = tln1->computeError(tln2->centers());
	double nErr2 = tln2->computeError(tln1->centers());

	return nErr1 < maxErr1 && nErr2 < maxErr2;
	//Ellipse e1 = textLineEllipse(tln1->pixels());
	//Ellipse e2 = textLineEllipse(tln2->pixels());

	//// compute the new text line error
	//QVector<QSharedPointer<Pixel> > pixels;
	//pixels << tln1->pixels();
	//pixels << tln2->pixels();

	//Ellipse nE = textLineEllipse(pixels);

	//double d1 = Algorithms::angleDist(e1.angle(), nE.angle(), CV_PI);
	//double d2 = Algorithms::angleDist(e2.angle(), nE.angle(), CV_PI);
	//double angleThresh = CV_PI*0.5 / std::sqrt(pixels.size());

	//if (d1 > angleThresh || d2 > angleThresh)
	//	return false;

	////double er1 = e1.minorAxis() / e1.majorAxis();
	////double er2 = e2.minorAxis() / e2.majorAxis();
	//double ern = nE.minorAxis() / nE.majorAxis();


	//double aRatioThresh = 1.0 / std::sqrt(pixels.size());

	//return ern < aRatioThresh;

/*
	double maxErr = qMax(er1, er2);

	return ern < maxErr * maxErrPerc;*/


	//double minErr = qMax(qMin(err1, err2)*maxErrPerc, 0.01);

	//// compute the new text line error
	//QVector<QSharedPointer<Pixel> > pixels;
	//pixels << tln1->pixels();
	//pixels << tln2->pixels();

	//double nErr = textLineError(pixels);

	//return nErr < minErr;
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
	
	for (auto set : mTextLines) {
		Drawer::instance().setColor(ColorManager::getColor());
		p.setPen(Drawer::instance().pen());
		set->draw(p, (int)PixelSet::draw_pixels);

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