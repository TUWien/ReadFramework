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
//cv::Mat Drawer::drawPoints(const cv::Mat & img, const std::vector<cv::Point> & pts, const QColor& col) {
//
//	// convert image
//	QPixmap pm = QPixmap::fromImage(Image::mat2QImage(img));
//	QPainter p(&pm);
//
//	drawPoints(p, pts);
//
//	return Image::qImage2Mat(pm.toImage());
//}

void Drawer::drawPoints(QPainter & p, const std::vector<cv::Point>& pts) {

	// convert points
	QVector<QPointF> qPts;

	for (const cv::Point& cp : pts)
		qPts << QPointF(cp.x, cp.y);

	drawPoints(p, qPts);
}

void Drawer::drawPoints(QPainter & p, const QVector<QPointF>& pts) {

	for (const QPointF& cp : pts)
		p.drawPoint(cp);
}

//cv::Mat Drawer::drawRects(const cv::Mat & img, const std::vector<cv::Rect>& rects) {
//	
//	// convert image
//	QPixmap pm = QPixmap::fromImage(Image::mat2QImage(img));
//	QPainter p(&pm);
//
//	// convert points
//	QVector<QRectF> qRects;
//
//	for (const cv::Rect& r : rects)
//		qRects << Converter::cvRectToQt(r);
//
//	drawRects(p, qRects);
//
//	return Image::qImage2Mat(pm.toImage());
//}

void Drawer::drawRects(QPainter & p, const QVector<QRectF>& rects) {

	for (const QRectF& r : rects)
		p.drawRect(r);

}

// ColorManager --------------------------------------------------------------------
/// <summary>
/// Returns a pleasent color.
/// </summary>
/// <param name="idx">If idx != -1 a specific color is chosen from the palette.</param>
/// <returns></returns>
QColor ColorManager::randColor(double alpha) {

	QVector<QColor> cols = colors();
	int maxCols = cols.size();
	
	int	idx = qRound(Utils::rand()*maxCols * 3);

	return getColor(idx, alpha);
}

/// <summary>
/// Returns a pleasent color.
/// </summary>
/// <param name="idx">If idx != -1 a specific color is chosen from the palette.</param>
/// <returns></returns>
QColor ColorManager::getColor(int idx, double alpha) {
	
	assert(idx >= 0);

	QVector<QColor> cols = colors();
	assert(cols.size() > 0);
	
	QColor col = cols[idx % cols.size()];

	// currently not hit
	if (idx > 2 * cols.size())
		col = col.darker();
	else if (idx > cols.size())
		col = col.lighter();

	col.setAlpha(qRound(alpha * 255));

	return col;
}

/// <summary>
/// Returns our color palette.
/// </summary>
/// <returns></returns>
QVector<QColor> ColorManager::colors() {

	static QVector<QColor> cols;

	if (cols.empty()) {
		cols << QColor(238, 120, 34);
		cols << QColor(240, 168, 47);
		cols << QColor(120, 192, 167);
		cols << QColor(251, 234, 181);
	}

	return cols;
}

/// <summary>
/// Returns a light gray.
/// </summary>
/// <param name="alpha">Optional alpha [0 1].</param>
/// <returns></returns>
QColor ColorManager::lightGray(double alpha) {
	return QColor(200, 200, 200, qRound(alpha * 255));
}

/// <summary>
/// Returns a dark gray.
/// </summary>
/// <param name="alpha">Optional alpha [0 1].</param>
/// <returns></returns>
QColor ColorManager::darkGray(double alpha) {
	return QColor(66, 66, 66, qRound(alpha * 255));
}

/// <summary>
/// Returns a dark red.
/// </summary>
/// <param name="alpha">Optional alpha [0 1].</param>
/// <returns></returns>
QColor ColorManager::red(double alpha) {
	return QColor(200, 50, 50, qRound(alpha * 255));
}

/// <summary>
/// Returns a light green.
/// </summary>
/// <param name="alpha">Optional alpha [0 1].</param>
/// <returns></returns>
DllCoreExport QColor ColorManager::green(double alpha) {
	
	return QColor(120, 192, 167, qRound(alpha*255)); 
}

/// <summary>
/// Returns the TU Wien blue.
/// </summary>
/// <param name="alpha">Optional alpha [0 1].</param>
/// <returns></returns>
QColor ColorManager::blue(double alpha) {
	return QColor(0, 102, 153, qRound(alpha * 255));
}

/// <summary>
/// Returns a pink color - not the artist.
/// </summary>
/// <param name="alpha">Optional alpha [0 1].</param>
/// <returns></returns>
QColor ColorManager::pink(double alpha) {
	return QColor(255, 0, 127, qRound(alpha * 255));
}

/// <summary>
/// Returns white - yes it's #fff.
/// </summary>
/// <param name="alpha">Optional alpha [0 1].</param>
/// <returns></returns>
QColor ColorManager::white(double alpha) {
	return QColor(255, 255, 255, qRound(alpha * 255));
}

}