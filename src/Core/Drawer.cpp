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

#include "Drawer.h"

#include "Image.h"
#include "Utils.h"

#pragma warning(push, 0)	// no warnings from includes
#include <QSharedPointer>
#include <QPixmap>
#include <QPainter>

#include <opencv2/core/core.hpp>
#pragma warning(pop)

namespace rdf {


// Drawer --------------------------------------------------------------------
Drawer::Drawer() {

	mPen.setColor(QColor(255, 153, 51));
}

Drawer& Drawer::instance() {

	static QSharedPointer<Drawer> inst;
	if (!inst)
		inst = QSharedPointer<Drawer>(new Drawer());
	return *inst;
}

cv::Mat Drawer::drawPoints(const cv::Mat & img, const std::vector<cv::Point> & pts) const {

	// convert image
	QPixmap pm = QPixmap::fromImage(Image::mat2QImage(img));
	QPainter p(&pm);

	drawPoints(p, pts);

	return Image::qImage2Mat(pm.toImage());
}

void Drawer::drawPoints(QPainter & p, const std::vector<cv::Point>& pts) const {

	// convert points
	QVector<QPointF> qPts;

	for (const cv::Point& cp : pts)
		qPts << QPointF(cp.x, cp.y);

	drawPoints(p, qPts);
}

void Drawer::drawPoints(QPainter & p, const QVector<QPointF>& pts) const {

	p.setPen(mPen);

	for (const QPointF& cp : pts)
		p.drawPoint(cp);
}

void Drawer::drawPoint(QPainter & p, const QPointF & pt) const {
	
	p.setPen(mPen);
	p.drawPoint(pt);
}

cv::Mat Drawer::drawRects(const cv::Mat & img, const std::vector<cv::Rect>& rects) const {
	
	// convert image
	QPixmap pm = QPixmap::fromImage(Image::mat2QImage(img));
	QPainter p(&pm);

	// convert points
	QVector<QRectF> qRects;

	for (const cv::Rect& r : rects)
		qRects << Converter::cvRectToQt(r);

	drawRects(p, qRects);

	return Image::qImage2Mat(pm.toImage());
}

void Drawer::drawRects(QPainter & p, const QVector<QRectF>& rects) const {

	for (const QRectF& r : rects)
		drawRect(p, r);
}

void Drawer::drawRect(QPainter & p, const QRectF& rect) const {

	p.setPen(mPen);
	p.drawRect(rect);
}

void Drawer::setColor(const QColor & col) {
	mPen.setColor(col);
}

void Drawer::setStrokeWidth(int strokeWidth) {
	mPen.setWidth(strokeWidth);
}

void Drawer::setPen(const QPen& pen) {
	mPen = pen;
}

QPen Drawer::pen() const {
	return mPen;
}

}