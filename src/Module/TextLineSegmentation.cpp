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

	QVector<QSharedPointer<PixelEdge> > pEdges = PixelSet::connect(mSuperPixels, mImgRect);

	for (const QSharedPointer<PixelEdge>& pe : pEdges)
		mEdges << QSharedPointer<LineEdge>(new LineEdge(*pe));

	mEdges = filterEdges(mEdges);

	mDebug << "computed in" << dt;

	return true;
}

QSharedPointer<TextLineConfig> TextLineSegmentation::config() const {
	return qSharedPointerDynamicCast<TextLineConfig>(mConfig);
}

QVector<QSharedPointer<LineEdge> > TextLineSegmentation::filterEdges(const QVector<QSharedPointer<LineEdge>>& pixelEdges, double factor) const {

	QVector<QSharedPointer<LineEdge> > pixelEdgesClean;

	for (auto pe : pixelEdges) {

		if (pe->edgeWeight() < factor)
			pixelEdgesClean << pe;
	}

	return pixelEdgesClean;
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
	QColor col = QColor(0, 60, 60);
	Drawer::instance().setColor(col);
	p.setPen(Drawer::instance().pen());

	for (auto b : mEdges) {
		b->draw(p);
	}

	// why do I have to do this?
	QVector<QSharedPointer<PixelEdge> > pe;
	for (auto e : mEdges)
		pe << e;

	auto sets = PixelSet::fromEdges(pe);

	for (auto set : sets) {
		Drawer::instance().setColor(ColorManager::instance().getRandomColor());
		p.setPen(Drawer::instance().pen());

		for (auto pixel : set->pixels()) {
			pixel->draw(p, .3, Pixel::draw_ellipse_only);
		}
	}

	mDebug << mEdges.size() << "edges drawn in" << dtf;

	return Image::instance().qPixmap2Mat(pm);
}

QString TextLineSegmentation::toString() const {
	return Module::toString();
}

bool TextLineSegmentation::checkInput() const {

	return !mSuperPixels.isEmpty();
}

}