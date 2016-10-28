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
TextLineConfig::TextLineConfig() {
	mModuleName = "Text Line";
}

QString TextLineConfig::toString() const {
	return ModuleConfig::toString();
}

// TextLineSegmentation --------------------------------------------------------------------
TextLineSegmentation::TextLineSegmentation(const Rect& rect, const QVector<QSharedPointer<Pixel> >& superPixels) {

	mSuperPixels = superPixels;
	mImgRect = rect;
	mConfig = QSharedPointer<TextLineConfig>::create();
}

bool TextLineSegmentation::isEmpty() const {
	return mSuperPixels.empty();
}

bool TextLineSegmentation::compute() {

	Timer dt;

	if (!checkInput())
		return false;

	RegionPixelConnector rpc;
	rpc.setLineSpacingMultiplier(2.0);
	QVector<QSharedPointer<PixelEdge> > pEdges = rpc.connect(mSuperPixels);

	qDebug() << "# edges: " << pEdges.size();

	for (const QSharedPointer<PixelEdge>& pe : pEdges)
		mEdges << QSharedPointer<LineEdge>(new LineEdge(*pe));

	mEdges = filterEdges(mEdges);

	if (!mStopLines.empty())
		mEdges = filterEdges(mEdges, mStopLines);

	mDebug << "computed in" << dt;

	return true;
}

QSharedPointer<TextLineConfig> TextLineSegmentation::config() const {
	return qSharedPointerDynamicCast<TextLineConfig>(mConfig);
}

QVector<QSharedPointer<LineEdge> > TextLineSegmentation::filterEdges(const QVector<QSharedPointer<LineEdge>>& edges, double factor) const {

	QVector<QSharedPointer<LineEdge> > pixelEdgesClean;

	for (auto pe : edges) {

		if (pe->edgeWeight() < factor)
			pixelEdgesClean << pe;
	}

	return pixelEdgesClean;
}

QVector<QSharedPointer<LineEdge>> TextLineSegmentation::filterEdges(const QVector<QSharedPointer<LineEdge>>& edges, const QVector<Line>& lines) const {

	QVector<QSharedPointer<LineEdge> > pixelEdgesClean;

	for (const QSharedPointer<LineEdge>& pe : edges) {

		bool filter = false;
		
		// does the edge intersect with any 'stop' line?
		for (const Line& line : lines) {
			if (pe->edge().intersects(line)) {
				filter = true;
				break;
			}
		}

		if (!filter)
			pixelEdgesClean << pe;
	}

	return pixelEdgesClean;
}

QVector<QSharedPointer<PixelSet> > TextLineSegmentation::toSets() const {

	// why do I have to do this?
	QVector<QSharedPointer<PixelEdge> > pe;
	for (auto e : mEdges)
		pe << e;

	QVector<QSharedPointer<PixelSet> > allSets = PixelSet::fromEdges(pe);
	QVector<QSharedPointer<PixelSet> > filteredSets;

	for (auto s : allSets) {

		if (s->pixels().size() > 3)
			filteredSets << s;
	}

	return filteredSets;
}

void TextLineSegmentation::slac(const QVector<QSharedPointer<LineEdge>>& edges) const {

	QVector<QSharedPointer<PixelSet> > clusters;

	for (const QSharedPointer<PixelEdge>& pe : edges) {
		QSharedPointer<PixelSet> ps(new PixelSet());
		ps->add(pe->first());
		ps->add(pe->second());
	}

	// TODO: go on here...

}

cv::Mat TextLineSegmentation::draw(const cv::Mat& img) const {

	// draw mser blobs
	Timer dtf;
	QPixmap pm = Image::instance().mat2QPixmap(img);

	QPainter p(&pm);
	
	// this block draws the edges
	Drawer::instance().setColor(ColorManager::instance().darkGray(0.4));
	p.setPen(Drawer::instance().pen());

	for (auto b : mEdges) {
		b->draw(p);
	}

	// show the stop lines
	Drawer::instance().setColor(ColorManager::instance().red(0.4));
	p.setPen(Drawer::instance().pen());

	for (auto l : mStopLines)
		l.draw(p);

	auto sets = toSets();
	
	for (auto set : sets) {
		Drawer::instance().setColor(ColorManager::instance().getRandomColor());
		p.setPen(Drawer::instance().pen());
		set->draw(p);

		//for (auto pixel : set->pixels()) {
		//	pixel->draw(p, .3, Pixel::draw_ellipse_only);
		//}
	}

	mDebug << mEdges.size() << "edges drawn in" << dtf;

	return Image::instance().qPixmap2Mat(pm);
}

QString TextLineSegmentation::toString() const {
	return Module::toString();
}

void TextLineSegmentation::addLines(const QVector<Line>& lines) {
	mStopLines << lines;
}

QVector<QSharedPointer<TextLine>> TextLineSegmentation::textLines() const {
	
	QVector<QSharedPointer<TextLine>> tls;
	for (auto set : toSets())
		tls << set->toTextLine();

	return tls;
}

bool TextLineSegmentation::checkInput() const {

	return !mSuperPixels.isEmpty();
}

}