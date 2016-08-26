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

	mEdges = PixelSet::connect(mSuperPixels, mImgRect);
	mEdges = filterEdges(mEdges);

	mDebug << "computed in" << dt;

	return true;
}

QSharedPointer<TextLineConfig> TextLineSegmentation::config() const {
	return qSharedPointerDynamicCast<TextLineConfig>(mConfig);
}

QVector<QSharedPointer<PixelEdge> > TextLineSegmentation::filterEdges(const QVector<QSharedPointer<PixelEdge>>& pixelEdges, double factor) const {

	QVector<QSharedPointer<PixelEdge> > pixelEdgesClean;

	for (auto pe : pixelEdges) {

		double w1 = edgeWeight(pe->first(), pe);
		double w2 = edgeWeight(pe->second(), pe);

		if (qMax(abs(w1), abs(w2)) < factor)
			pixelEdgesClean << pe;
	}

	return pixelEdgesClean;
}

double TextLineSegmentation::edgeWeight(const QSharedPointer<Pixel>& pixel, const QSharedPointer<PixelEdge>& edge) const {

	if (!pixel || !edge)
		return DBL_MAX;

	if (!pixel->stats()) {
		mWarning << "pixel stats are NULL where they must not be...";
		return DBL_MAX;
	}

	QSharedPointer<PixelStats> stats = pixel->stats();

	Vector2D vec(1, 0);
	vec.rotate(stats->orientation());

	Vector2D eVec = edge->edge().vector();

	return vec * eVec;
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

	auto sets = PixelSet::fromEdges(mEdges);

	for (auto set : sets) {
		Drawer::instance().setColor(ColorManager::instance().getRandomColor());
		p.setPen(Drawer::instance().pen());

		for (auto pixel : set->pixels()) {
			pixel->draw(p, .3, Pixel::draw_ellipse_stats);
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