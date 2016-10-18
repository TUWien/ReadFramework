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

#include "TextBlockSegmentation.h"

#include "Image.h"
#include "Drawer.h"
#include "Utils.h"
#include "Algorithms.h"

#pragma warning(push, 0)	// no warnings from includes
#include <QDebug>
#include <QPainter>

#pragma warning(pop)

namespace rdf {

// TextBlockConfig --------------------------------------------------------------------
TextBlockConfig::TextBlockConfig() {
	mModuleName = "Text Block";
}

QString TextBlockConfig::toString() const {
	return ModuleConfig::toString();
}

// TextBlockSegmentation --------------------------------------------------------------------
TextBlockSegmentation::TextBlockSegmentation(const cv::Mat& srcImg, 
	const QVector<QSharedPointer<Pixel> >& superPixels) {
	
	mSrcImg = srcImg;
	mSuperPixels = superPixels;
	mConfig = QSharedPointer<TextBlockConfig>::create();
}

bool TextBlockSegmentation::isEmpty() const {
	return mSrcImg.empty();
}

bool TextBlockSegmentation::compute() {

	Timer dt;

	if (!checkInput())
		return false;

	Rect r(0, 0, mSrcImg.cols, mSrcImg.rows);
	mGraph = QSharedPointer<PixelGraph>::create(mSuperPixels);
	mGraph->connect(r, PixelSet::connect_region);

	mSuperPixels = findTabStopCandidates(mGraph);
	mTabStops = findTabs(mSuperPixels);
	//mEdges = PixelSet::connect(mSuperPixels, r);
	//mEdges = filterEdges(mEdges);
	//
	//mTextBlocks = createTextBlocks(mEdges);

	mDebug << "I found" << mSuperPixels.size() << "tab stop candidates";
	mDebug << "text blocks computed in" << dt;

	return true;
}

QSharedPointer<TextBlockConfig> TextBlockSegmentation::config() const {
	return qSharedPointerDynamicCast<TextBlockConfig>(mConfig);
}

QVector<QSharedPointer<PixelEdge> > TextBlockSegmentation::filterEdges(const QVector<QSharedPointer<PixelEdge>>& pixelEdges, double factor) {
	
	double meanEdgeLength = 0;

	for (auto pe : pixelEdges)
		meanEdgeLength += pe->edgeWeight();
	
	meanEdgeLength /= pixelEdges.size();
	double maxEdgeLength = meanEdgeLength * factor;

	QVector<QSharedPointer<PixelEdge> > pixelEdgesClean;

	for (auto pe : pixelEdges) {

		if (pe->edgeWeight() < maxEdgeLength)
			pixelEdgesClean << pe;
	}

	return pixelEdgesClean;
}

cv::Mat TextBlockSegmentation::draw(const cv::Mat& img) const {
	
	// draw mser blobs
	Timer dtf;
	QPixmap pm = Image::instance().mat2QPixmap(img);
	
	QPainter p(&pm);
	QColor col = ColorManager::instance().darkGray();
	Drawer::instance().setColor(col);
	p.setPen(Drawer::instance().pen());

	//for (auto b : mGraph->edges()) {
	//	b->draw(p);
	//}

	//PixelGraph pg(mSuperPixels);
	//pg.connect(Rect(0, 0, mSrcImg.cols, mSrcImg.rows));
	//pg.draw(p);

	//mGraph->draw(p);

	p.setPen(ColorManager::instance().colors()[2]);
	for (auto px : mSuperPixels)
		px->draw(p, 0.4, Pixel::draw_ellipse_stats);

	for (auto ts : mTabStops) {
		Drawer::instance().setColor(ColorManager::instance().getRandomColor());
		p.setPen(Drawer::instance().pen());
		ts->draw(p);
	}

	for (auto tb : mTextBlocks) {
		Drawer::instance().setColor(ColorManager::instance().getRandomColor());
		p.setPen(Drawer::instance().pen());

		tb->draw(p);
	}

	return Image::instance().qPixmap2Mat(pm);
}

QString TextBlockSegmentation::toString() const {
	return Module::toString();
}

bool TextBlockSegmentation::checkInput() const {
	
	return !mSuperPixels.isEmpty();
}

QVector<QSharedPointer<PixelSet> > TextBlockSegmentation::createTextBlocks(const QVector<QSharedPointer<PixelEdge> >& edges) const {
	
	QVector<QSharedPointer<PixelSet> > sets;

	for (const QSharedPointer<PixelEdge> e : edges) {

		int fIdx = -1;
		int sIdx = -1;

		for (int idx = 0; idx < sets.size(); idx++) {

			if (sets[idx]->contains(e->first()))
				fIdx = idx;
			if (sets[idx]->contains(e->second()))
				sIdx = idx;

			if (fIdx != -1 && sIdx != -1)
				break;
		}

		// none is contained in a set
		if (fIdx == -1 && sIdx == -1) {
			QSharedPointer<PixelSet> ps(new PixelSet());
			ps->add(e->first());
			ps->add(e->second());
			sets << ps;
		}
		// add first to the set of second
		else if (fIdx == -1) {
			sets[sIdx]->add(e->first());
		}
		// add second to the set of first
		else if (sIdx == -1) {
			sets[fIdx]->add(e->second());
		}
		// two different idx? - merge the sets
		else if (fIdx != sIdx) {
			sets[fIdx]->merge(*sets[sIdx]);
			sets.remove(sIdx);
		}
		// else : nothing to do - they are both already added

	}

	qDebug() << "I found" << sets.size() << "text blocks";
	
	return sets;
}

QVector<QSharedPointer<Pixel> > TextBlockSegmentation::findTabStopCandidates(const QSharedPointer<PixelGraph>& graph) const {

	QVector<QSharedPointer<Pixel> > tabStops;
	
	for (const QSharedPointer<Pixel>& pixel : graph->set()->pixels()) {

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

QVector<QSharedPointer<TabStopCluster> > TextBlockSegmentation::findTabs(const QVector<QSharedPointer<Pixel>>& pixel) const {
	
	// parameters 
	int minClusterSize = 4;
	double tabLineAngleDist = 0.6;	// angle difference (in radians) between the tabline found and the median tab orientation

	QVector<QSharedPointer<PixelEdge> > edges = PixelSet::connect(pixel, Rect(), PixelSet::connect_tab_stops);
	QVector<QSharedPointer<PixelSet> > ps = PixelSet::fromEdges(edges);

	QVector<QSharedPointer<TabStopCluster> > tabStops;
	mDebug << ps.size() << "initial tab stop sets";

	for (const QSharedPointer<PixelSet>& set : ps) {

		if (set->pixels().size() >= minClusterSize) {
		
			double medAngle = medianOrientation(set);
			updateTabStopCandidates(set, medAngle);		// removes 'illegal' candidates

			Line line = set->baseline(medAngle);
			line.setThickness(4);

			Vector2D lineVec = line.vector();
			lineVec = lineVec / lineVec.length();

			Vector2D tabVec(1, 0);
			tabVec.rotate(medAngle);

			if (abs(lineVec*tabVec) < 0.5) {
				
				QSharedPointer<TabStopCluster> tabStop(new TabStopCluster(set));
				tabStop->setLine(line);
				tabStops << tabStop;
			}
			else
				qDebug() << "tab stops rejected - wrong angle: " << abs(lineVec*tabVec);
		}
	}

	//// update tab stop candidates
	//pixel = cPixel;

	return tabStops;
}

double TextBlockSegmentation::medianOrientation(const QSharedPointer<PixelSet>& set) const {


	QList<double> angles;
	for (const QSharedPointer<Pixel>& px : set->pixels()) {
		
		if (!px->stats()) {
			mWarning << "stats is NULL where it should not be...";
			continue;
		}
		
		angles << px->stats()->orientation() - px->tabStop().orientation();
	}

	double medAngle = Algorithms::statMoment(angles, 0.5);

	return medAngle;
}

void TextBlockSegmentation::updateTabStopCandidates(const QSharedPointer<PixelSet>& set, double orientation, const PixelTabStop::Type & newType) const {

	for (const QSharedPointer<Pixel>& px : set->pixels()) {

		if (!px->stats())
			continue;

		double pxa = px->stats()->orientation() - px->tabStop().orientation();
		double d = Algorithms::instance().angleDist(pxa, orientation);

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

void TabStopCluster::draw(QPainter & p) const {

	mSet->draw(p);

	QPen oPen = p.pen();
	p.setPen(ColorManager::instance().colors()[0]);
	
	mLine.draw(p);
	
	p.setPen(oPen);
}

}