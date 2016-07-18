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

#include "Shapes.h"
#include "Utils.h"
#include "Algorithms.h"

#pragma warning(push, 0)	// no warnings from includes
// Qt Includes
#include <QDebug>
#pragma warning(pop)

namespace rdf {

Polygon::Polygon(const QPolygon& polygon) {
	mPoly = polygon;
}
bool Polygon::isEmpty() const {
	return mPoly.isEmpty();
}
void Polygon::read(const QString & pointList) {
	mPoly = Converter::instance().stringToPoly(pointList);
}

QString Polygon::write() const {
	return Converter::instance().polyToString(mPoly);
}

int Polygon::size() const {
	return mPoly.size();
}

void Polygon::setPolygon(const QPolygon & polygon) {
	mPoly = polygon;
}

QPolygon Polygon::polygon() const {
	return mPoly;
}

QPolygon Polygon::closedPolygon() const {
	
	QPolygon closed = mPoly;
	if (!mPoly.isEmpty())
		closed.append(mPoly.first());

	return closed;
}

// BaseLine --------------------------------------------------------------------
BaseLine::BaseLine(const QPolygon & baseLine) {
	mBaseLine = baseLine;
}

bool BaseLine::isEmpty() const {
	return mBaseLine.isEmpty();
}

void BaseLine::setPolygon(QPolygon & baseLine) {
	mBaseLine = baseLine;
}

QPolygon BaseLine::polygon() const {
	return mBaseLine;
}

void BaseLine::read(const QString & pointList) {
	mBaseLine = Converter::instance().stringToPoly(pointList);
}

QString BaseLine::write() const {
	return Converter::instance().polyToString(mBaseLine);
}

QPoint BaseLine::startPoint() const {
	
	if (!mBaseLine.isEmpty())
		return mBaseLine.first();

	return QPoint();
}

QPoint BaseLine::endPoint() const {

	if (!mBaseLine.isEmpty())
		return mBaseLine.last();

	return QPoint();
}

// Line ------------------------------------------------------------------------------------------------------

/// <summary>
/// Initializes a new instance of the <see cref="Line"/> class.
/// </summary>
/// <param name="line">The line.</param>
/// <param name="thickness">The stroke width.</param>
Line::Line(const QLine& line, float thickness) {
	mLine = line;
	mThickness = thickness;
}

Line::Line(const Polygon & poly) {

	if (poly.size() != 2)
		qWarning() << "line initialized with illegal polygon, size: " << poly.size();
	else
		mLine = QLine(poly.polygon()[0], poly.polygon()[1]);

}

cv::Point Line::startPointCV() const
{
	return cv::Point(mLine.p1().x(), mLine.p1().y());
}

cv::Point Line::endPointCV() const
{
	return cv::Point(mLine.p2().x(), mLine.p2().y());
}

/// <summary>
/// Determines whether this instance is empty.
/// </summary>
/// <returns>True if no line is set</returns>
bool Line::isEmpty() const {
	return (mLine.isNull());
}

/// <summary>
/// Sets the line.
/// </summary>
/// <param name="line">The line.</param>
/// <param name="thickness">The stroke width.</param>
void Line::setLine(const QLine& line, float thickness) {
	mLine = line;
	mThickness = thickness;
}

/// <summary>
/// Returns the line information.
/// </summary>
/// <returns>The line.</returns>
QLine Line::line() const {
	return mLine;
}

/// <summary>
/// Returns the stroke width of the line.
/// </summary>
/// <returns>The stroke width.</returns>
float Line::thickness() const {
	return mThickness;
}

/// <summary>
/// Returns the line length.
/// </summary>
/// <returns>The line length.</returns>
float Line::length() const {
	QPoint diff = mLine.p2() - mLine.p1();

	return (float)sqrt(diff.x()*diff.x() + diff.y()*diff.y());

}

/// <summary>
/// Returns the line angle.
/// </summary>
/// <returns>The line angle [-pi,+pi] in radians.</returns>
double Line::angle() const {

	QPoint diff = mLine.p2() - mLine.p1();

	return atan2(diff.y(), diff.x());

}

/// <summary>
/// Returns the start point.
/// </summary>
/// <returns>The start point.</returns>
QPoint Line::startPoint() const {
	return mLine.p1();
}

/// <summary>
/// Returns the end point.
/// </summary>
/// <returns>The end point.</returns>
QPoint Line::endPoint() const {
	return mLine.p2();
}

/// <summary>
/// Returns the minimum distance of the line endings of line l to the line endings of the current line instance.
/// </summary>
/// <param name="l">The line l to which the minimum distance is computed.</param>
/// <returns>The minimum distance.</returns>
float Line::minDistance(const Line& l) const {

	float dist1 = rdf::Algorithms::instance().euclideanDistance(mLine.p1(), l.line().p1());
	float dist2 = rdf::Algorithms::instance().euclideanDistance(mLine.p1(), l.line().p2());
	dist1 = (dist1 < dist2) ? dist1 : dist2;
	dist2 = rdf::Algorithms::instance().euclideanDistance(mLine.p2(), l.line().p1());
	dist1 = (dist1 < dist2) ? dist1 : dist2;
	dist2 = rdf::Algorithms::instance().euclideanDistance(mLine.p2(), l.line().p2());
	dist1 = (dist1 < dist2) ? dist1 : dist2;

	return dist1;
}

/// <summary>
/// Returns the minimal distance of point p to the current line instance.
/// </summary>
/// <param name="p">The p.</param>
/// <returns>The distance of point p.</returns>
float Line::distance(const QPoint p) const {

	QPoint normalVec = mLine.p2() - mLine.p1();

	int x = normalVec.x();
	normalVec.setX(-normalVec.y());
	normalVec.setY(x);

	QPoint tmp = p - mLine.p2();

	return (float)abs(normalVec.x()*tmp.x() + normalVec.y()*tmp.y() / (FLT_EPSILON + sqrt(normalVec.x()*normalVec.x() + normalVec.y()*normalVec.y())));
}

int Line::horizontalOverlap(const Line & l) const
{
	int ol = std::max(mLine.x1(), l.startPoint().x()) - std::min(mLine.x2(), l.endPoint().x());

	return ol;
}

int Line::verticalOverlap(const Line & l) const
{
	int ol = std::max(mLine.y1(), l.startPoint().y()) - std::min(mLine.y2(), l.endPoint().y());

	return ol;
}

/// <summary>
/// Calculated if point p is within the current line instance.
/// </summary>
/// <param name="p">The point p to be checked.</param>
/// <returns>True if p is within the current line instance.</returns>
bool Line::within(const QPoint& p) const {

	QPoint tmp = mLine.p2() - mLine.p1();
	QPoint tmp2(p.x() - mLine.p2().x(), p.y() - mLine.p2().y());	//p-end
	QPoint tmp3(p.x() - mLine.p1().x(), p.y() - mLine.p1().y());	//p-start
	
	return (tmp.x()*tmp2.x() + tmp.y()*tmp2.y()) * (tmp.x()*tmp3.x() + tmp.y()*tmp3.y()) < 0;

}

bool Line::lessX1(const Line& l1, const Line& l2) {

	if (l1.startPoint().x() < l2.startPoint().x())
		return true;
	else
		return false;
}

bool Line::lessY1(const Line& l1, const Line& l2) {

	if (l1.startPoint().y() <l2.startPoint().y())
		return true;
	else
		return false;
}

/// <summary>
/// Merges the specified line l with the current line instance.
/// </summary>
/// <param name="l">The line to merge.</param>
/// <returns>The merged line.</returns>
Line Line::merge(const Line& l) const {

	cv::Mat dist = cv::Mat(1, 4, CV_32FC1);
	float* ptr = dist.ptr<float>();

	ptr[0] = rdf::Algorithms::instance().euclideanDistance(mLine.p1(), l.line().p1());
	ptr[1] = rdf::Algorithms::instance().euclideanDistance(mLine.p1(), l.line().p2());
	ptr[2] = rdf::Algorithms::instance().euclideanDistance(mLine.p2(), l.line().p1());
	ptr[3] = rdf::Algorithms::instance().euclideanDistance(mLine.p2(), l.line().p2());

	cv::Point maxIdxP;
	minMaxLoc(dist, 0, 0, 0, &maxIdxP);
	int maxIdx = maxIdxP.x;

	float thickness = mThickness < l.thickness() ? mThickness : l.thickness();
	Line mergedLine;

	switch (maxIdx) {
	case 0: mergedLine = Line(QLine(mLine.p1(), l.line().p1()), thickness);	break;
	case 1: mergedLine = Line(QLine(mLine.p1(), l.line().p2()), thickness);	break;
	case 2: mergedLine = Line(QLine(mLine.p2(), l.line().p1()), thickness);	break;
	case 3: mergedLine = Line(QLine(mLine.p2(), l.line().p2()), thickness);	break;
	}

	return mergedLine;
}

/// <summary>
/// Calculates the gap line between line l and the current line instance.
/// </summary>
/// <param name="l">The line l to which a gap line is calculated.</param>
/// <returns>The gap line.</returns>
Line Line::gapLine(const Line& l) const {

	cv::Mat dist = cv::Mat(1, 4, CV_32FC1);
	float* ptr = dist.ptr<float>();

	ptr[0] = rdf::Algorithms::instance().euclideanDistance(mLine.p1(), l.line().p1());
	ptr[1] = rdf::Algorithms::instance().euclideanDistance(mLine.p1(), l.line().p2());
	ptr[2] = rdf::Algorithms::instance().euclideanDistance(mLine.p2(), l.line().p1());
	ptr[3] = rdf::Algorithms::instance().euclideanDistance(mLine.p2(), l.line().p2());

	cv::Point minIdxP;
	minMaxLoc(dist, 0, 0, &minIdxP);
	int minIdx = minIdxP.x;

	float thickness = mThickness < l.thickness() ? mThickness : l.thickness();
	Line gapLine;

	switch (minIdx) {
	case 0: gapLine = Line(QLine(mLine.p1(), l.line().p1()), thickness);	break;
	case 1: gapLine = Line(QLine(mLine.p1(), l.line().p2()), thickness);	break;
	case 2: gapLine = Line(QLine(mLine.p2(), l.line().p1()), thickness);	break;
	case 3: gapLine = Line(QLine(mLine.p2(), l.line().p2()), thickness);	break;
	}

	return gapLine;
}

/// <summary>
/// The angle difference of line l and the current line instance.
/// </summary>
/// <param name="l">The line l.</param>
/// <returns>The angle difference in rad.</returns>
float Line::diffAngle(const Line& l) const {

	float angleLine, angleL;

	//angleLine
	angleLine = Algorithms::instance().normAngleRad((float)angle(), 0.0f, (float)CV_PI);
	angleLine = angleLine > (float)CV_PI*0.5f ? (float)CV_PI - angleLine : angleLine;
	angleL = Algorithms::instance().normAngleRad((float)l.angle(), 0.0f, (float)CV_PI);
	angleL = angleL > (float)CV_PI*0.5f ? (float)CV_PI - angleL : angleL;
	
	return fabs(angleLine - angleL);
}


}