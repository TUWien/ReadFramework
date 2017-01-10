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

#include "TabStopAnalysis.h"

#include "Image.h"
#include "Drawer.h"
#include "Utils.h"
#include "Algorithms.h"

#pragma warning(push, 0)	// no warnings from includes
#include <QDebug>
#include <QPainter>

#pragma warning(pop)

namespace rdf {

// TabStopConfig --------------------------------------------------------------------
TabStopConfig::TabStopConfig() {
	mModuleName = "Tab Stop";
}

QString TabStopConfig::toString() const {
	return ModuleConfig::toString();
}

// TabStopAnalysis --------------------------------------------------------------------
TabStopAnalysis::TabStopAnalysis(const QVector<QSharedPointer<Pixel> >& superPixels) {
	
	mSuperPixels = superPixels;
	mConfig = QSharedPointer<TabStopConfig>::create();
}

bool TabStopAnalysis::isEmpty() const {
	return mSuperPixels.empty();
}

bool TabStopAnalysis::compute() {

	Timer dt;

	if (!checkInput())
		return false;

	mGraph = QSharedPointer<PixelGraph>::create(mSuperPixels);
	mGraph->connect(RegionPixelConnector());

	QVector<QSharedPointer<Pixel> > tabStops = findTabStopCandidates(mGraph);
	mTabStops = findTabs(tabStops);

	mDebug << "I found" << tabStops.size() << "tab stop candidates in" << dt;

	return true;
}

QSharedPointer<TabStopConfig> TabStopAnalysis::config() const {
	return qSharedPointerDynamicCast<TabStopConfig>(mConfig);
}

cv::Mat TabStopAnalysis::draw(const cv::Mat& img) const {
	
	// draw mser blobs
	Timer dtf;
	QPixmap pm = Image::mat2QPixmap(img);
	
	QPainter p(&pm);

	//// uncomment to see rejected tabstops
	//QColor col = ColorManager::darkGray();
	//Drawer::instance().setColor(col);
	//p.setPen(Drawer::instance().pen());

	//p.setPen(ColorManager::colors()[2]);
	//for (auto px : mSuperPixels)
	//	if (px->tabStop().type() != PixelTabStop::type_none)
	//		px->draw(p, 0.4, Pixel::draw_ellipse_stats);

	for (auto ts : mTabStops) {
		Drawer::instance().setColor(ColorManager::getColor());
		p.setPen(Drawer::instance().pen());
		ts->draw(p);
	}

	return Image::qPixmap2Mat(pm);
}

QString TabStopAnalysis::toString() const {
	return Module::toString();
}

QVector<QSharedPointer<TabStopCluster>> TabStopAnalysis::tabStopClusters() const {

	return mTabStops;
}

QVector<Line> TabStopAnalysis::tabStopLines(double offset) const {
	
	QVector<Line> lines;

	for (const QSharedPointer<TabStopCluster>& tc : mTabStops) {

		Line line = tc->line();
		if (offset != 0) {
			Vector2D movVec(offset, 0);
			movVec.rotate(tc->angle() + CV_PI);
			line = line.moved(movVec);
		}

		lines << line;
	}
	
	return lines;
}

bool TabStopAnalysis::checkInput() const {
	
	return !mSuperPixels.isEmpty();
}

QVector<QSharedPointer<Pixel> > TabStopAnalysis::findTabStopCandidates(const QSharedPointer<PixelGraph>& graph) const {

	QVector<QSharedPointer<Pixel> > tabStops;
	
	for (const QSharedPointer<Pixel>& pixel : graph->set().pixels()) {

		if (!pixel->stats()) {
			mWarning << "pixel stats NULL where they should not be, pixel ID:" << pixel->id();
			continue;
		}

		QVector<QSharedPointer<PixelEdge> > edges = graph->edges(pixel->id());
		pixel->setTabStop(PixelTabStop::create(pixel, edges));

		if (pixel->tabStop().type() != PixelTabStop::type_none)
			tabStops << pixel;
	}

	mInfo << "I found" << tabStops.size() << "tab stop candidates";

	return tabStops;
}

QVector<QSharedPointer<TabStopCluster> > TabStopAnalysis::findTabs(const QVector<QSharedPointer<Pixel>>& pixel) const {
	
	// parameters 
	int minClusterSize = 4;

	TabStopPixelConnector connector;
	QVector<QSharedPointer<PixelEdge> > edges = connector.connect(pixel);
	QVector<QSharedPointer<PixelSet> > ps = PixelSet::fromEdges(edges);

	QVector<QSharedPointer<TabStopCluster> > tabStops;
	mDebug << ps.size() << "initial tab stop sets";

	for (const QSharedPointer<PixelSet>& set : ps) {

		if (set->pixels().size() >= minClusterSize) {
		
			double medAngle = medianOrientation(set);
			updateTabStopCandidates(set, medAngle);		// removes 'illegal' candidates

			// create tab stop line
			Line line = set->fitLine(medAngle);
			//line.setThickness(4);

			// // TODO: the idea here is simple: tabstops must be orthogonal to text lines - this is not true in general
			// so I thought now: what if we fit the line and see how many are 'robustly' fitting - if to few we have to reject!
			//Vector2D lineVec = line.vector();
			//Vector2D tabVec(1, 0);
			//tabVec.rotate(medAngle);

			//double cosTheta = (lineVec * tabVec) / (lineVec.length() * tabVec.length());

			//// we only find 'orthogonal' tab lines - shouldn't we remove this constraint?
			//if (abs(cosTheta) < 0.5) {
				
				QSharedPointer<TabStopCluster> tabStop(new TabStopCluster(set));
				tabStop->setLine(line);
				tabStop->setAngle(medAngle);
				tabStops << tabStop;
			//}
			//else
			//	qDebug() << "tab stops rejected - wrong angle: " << abs(cosTheta);
		}
	}

	//// update tab stop candidates
	//pixel = cPixel;

	return tabStops;
}

double TabStopAnalysis::medianOrientation(const QSharedPointer<PixelSet>& set) const {


	QList<double> angles;
	for (const QSharedPointer<Pixel>& px : set->pixels()) {
		
		if (!px->stats()) {
			mWarning << "stats is NULL where it should not be...";
			continue;
		}
		
		angles << px->stats()->orientation() - px->tabStop().orientation();
	}

	return Algorithms::statMoment(angles, 0.5);
}

void TabStopAnalysis::updateTabStopCandidates(const QSharedPointer<PixelSet>& set, double orientation, const PixelTabStop::Type & newType) const {

	for (const QSharedPointer<Pixel>& px : set->pixels()) {

		if (!px->stats())
			continue;

		double pxa = px->stats()->orientation() - px->tabStop().orientation();
		double d = Algorithms::angleDist(pxa, orientation);

		if (d > 0.1) {
			px->setTabStop(PixelTabStop(newType));
			set->remove(px);
		}
	}
}

// TabStopCluster --------------------------------------------------------------------
TabStopCluster::TabStopCluster(const QSharedPointer<PixelSet>& ps) {
	mSet = ps;
}

void TabStopCluster::setLine(const Line & line) {
	mLine = line;
}

Line TabStopCluster::line() const {
	return mLine;
}

QSharedPointer<PixelSet> TabStopCluster::set() const {
	return mSet;
}

void TabStopCluster::setAngle(double angle) {
	mMedAngle = angle;
}

double TabStopCluster::angle() const {
	return mMedAngle;
}

void TabStopCluster::draw(QPainter & p) const {

	//// uncomment to show tabstop cluster
	//mSet->draw(p);

	QPen oPen = p.pen();
	p.setPen(ColorManager::colors()[0]);
	mLine.draw(p);
	
	p.setPen(oPen);
}

}