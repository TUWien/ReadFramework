/*******************************************************************************************************
 ReadFramework is the basis for modules developed at CVL/TU Wien for the EU project READ. 
  
 Copyright (C) 2016 Markus Diem <diem@cvl.tuwien.ac.at>
 Copyright (C) 2016 Stefan Fiel <fiel@cvl.tuwien.ac.at>
 Copyright (C) 2016 Florian Kleber <kleber@cvl.tuwien.ac.at>

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
 [1] https://cvl.tuwien.ac.at/
 [2] https://transkribus.eu/Transkribus/
 [3] https://github.com/TUWien/
 [4] https://nomacs.org
 *******************************************************************************************************/

#include "Shapes.h"
#include "Utils.h"
#include "Algorithms.h"
#include "Image.h"

#pragma warning(push, 0)	// no warnings from includes
// Qt Includes
#include <QDebug>
#include <QPainter>
#pragma warning(pop)

namespace rdf {

Polygon::Polygon(const QPolygonF& polygon) {
	mPoly = polygon;
}
bool Polygon::isEmpty() const {
	return mPoly.isEmpty();
}
void Polygon::read(const QString & pointList) {
	mPoly = Converter::stringToPoly(pointList);
}

QString Polygon::write() const {
	return Converter::polyToString(mPoly.toPolygon());
}

void Polygon::translate(const QPointF & offset) {
	mPoly.translate(offset);
}

int Polygon::size() const {
	return mPoly.size();
}

Polygon Polygon::fromCvPoints(const std::vector<cv::Point2d>& pts) {
	
	Polygon poly;

	// convert to Qt
	for (const cv::Point2d& pt : pts)
		poly << Converter::cvPointToQt(pt);

	return poly;
}

Polygon Polygon::fromCvPoints(const std::vector<cv::Point2f>& pts) {

	Polygon poly;

	// convert to Qt
	for (const cv::Point2f& pt : pts)
		poly << Converter::cvPointToQt(pt);

	return poly;
}

Polygon Polygon::fromCvPoints(const std::vector<cv::Point>& pts) {

	Polygon poly;

	// convert to Qt
	for (const cv::Point& pt : pts)
		poly << Converter::cvPointToQt(pt);

	return poly;
}

Polygon Polygon::fromRect(const Rect & rect) {

	Polygon p;
	p << rect.topLeft();
	p << rect.topRight();
	p << rect.bottomRight();
	p << rect.bottomLeft();

	return p;
}

void Polygon::setPolygon(const QPolygonF & polygon) {
	mPoly = polygon;
}

void Polygon::scale(double factor) {

	QPolygonF sPoly;

	for (const QPointF& pt : mPoly) {
		sPoly << pt * factor;
	}

	mPoly = sPoly;
}

void Polygon::draw(QPainter & p) const {
	
	QPen oPen = p.pen();
	QPen pen = oPen;
	QColor fc = pen.color();
	fc.setAlpha(50);

	// draw area
	QPainterPath tmpPath;
	tmpPath.addPolygon(closedPolygon());
	p.fillPath(tmpPath, fc);

	pen.setWidth(3);
	p.setPen(pen);

	p.drawPolygon(closedPolygon());
	p.setPen(oPen);
}

bool Polygon::contains(const Vector2D & pt) const {

	return mPoly.containsPoint(pt.toQPointF(), Qt::WindingFill);
}

QPolygonF Polygon::polygon() const {
	return mPoly;
}

QPolygonF Polygon::closedPolygon() const {
	
	QPolygonF closed = mPoly;
	if (!mPoly.isEmpty())
		closed.append(mPoly.first());

	return closed;
}

QVector<Vector2D> Polygon::toPoints() const {

	//QVector<Vector2D> pts(mPoly.size());
	QVector<Vector2D> pts; //bugfix fk
	for (const QPointF& p : mPoly)
		pts << p;

	return pts;
}

// BaseLine --------------------------------------------------------------------
BaseLine::BaseLine(const QPolygonF & baseLine) {
	mBaseLine = baseLine;
}

BaseLine::BaseLine(const Polygon & baseLine) {
	mBaseLine = baseLine.polygon();
}

BaseLine::BaseLine(const Line & line) {
	
	QLineF ql = line.qLine();
	mBaseLine << ql.p1();
	mBaseLine << ql.p2();
}

bool BaseLine::isEmpty() const {
	return mBaseLine.isEmpty();
}

void BaseLine::setPolygon(const QPolygonF & baseLine) {
	mBaseLine = baseLine;
}

QPolygonF BaseLine::polygon() const {
	return mBaseLine;
}

QPolygon BaseLine::toPolygon() const {
	return mBaseLine.toPolygon();
}

void BaseLine::translate(const QPointF & offset) {
	mBaseLine.translate(offset);
}

void BaseLine::read(const QString & pointList) {
	mBaseLine = Converter::stringToPoly(pointList);
}

QString BaseLine::write() const {
	return Converter::polyToString(toPolygon());
}

QPointF BaseLine::startPoint() const {
	
	if (!mBaseLine.isEmpty())
		return mBaseLine.first();

	return QPoint();
}

QPointF BaseLine::endPoint() const {

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
Line::Line(const QLineF& line, float thickness) {
	mLine = line;
	mThickness = thickness;
}

Line::Line(const Polygon & poly) {

	if (poly.size() != 2)
		qWarning() << "line initialized with illegal polygon, size: " << poly.size();
	else
		mLine = QLineF(poly.polygon()[0], poly.polygon()[1]);

}

Line::Line(const cv::Point p1, const cv::Point p2, float thickness) {
	mLine = QLine(QPoint(p1.x, p1.y), QPoint(p2.x, p2.y));
	mThickness = thickness;
}

Line::Line(const Vector2D & p1, const Vector2D & p2, float thickness) {
	mLine = QLineF(p1.toQPointF(), p2.toQPointF());
	mThickness = thickness;
}

Line::Line(double p1x, double p1y, double p2x, double p2y, float thickness) {

	mLine = QLineF(p1x, p1y, p2x, p2y);
	mThickness = thickness;
}

/// <summary>
/// Determines whether this instance is empty.
/// </summary>
/// <returns>True if no line is set</returns>
bool Line::isEmpty() const {
	return (mLine.isNull());
}

void Line::sortEndpoints(bool horizontal) {

	QPointF p1, p2;
	p1 = mLine.p1();
	p2 = mLine.p2();

	if (horizontal) {
		int x1 = (int)p1.x();
		int x2 = (int)p2.x();

		if (x1 < x2)
			return;
		else {
			mLine.setP1(p2);
			mLine.setP2(p1);
			return;
		}
	} else {
		int y1 = (int)p1.y();
		int y2 = (int)p2.y();

		if (y1 < y2)
			return;
		else {
			mLine.setP1(p2);
			mLine.setP2(p1);
		}
	}
}

/// <summary>
/// Sets the line.
/// </summary>
/// <param name="line">The line.</param>
/// <param name="thickness">The stroke width.</param>
void Line::setLine(const QLineF& line, float thickness) {
	mLine = line;
	mThickness = thickness;
}

void Line::setThickness(float thickness) {
	mThickness = thickness;
}

/// <summary>
/// Returns the line information.
/// </summary>
/// <returns>The line.</returns>
QLineF Line::qLine() const {
	return mLine;
}

QPolygonF Line::toPoly() const {
	
	QPolygonF poly;
	poly << mLine.p1();
	poly << mLine.p2();
	return poly;
}

/// <summary>
/// Returns the stroke width of the line.
/// </summary>
/// <returns>The stroke width.</returns>
float Line::thickness() const {
	return mThickness;
}

double Line::squaredLength() const {
	
	QPointF diff = mLine.p2() - mLine.p1();
	return diff.x()*diff.x() + diff.y()*diff.y();
}

/// <summary>
/// Returns the line length.
/// </summary>
/// <returns>The line length.</returns>
double Line::length() const {

	return vector().length();
}

/// <summary>
/// Returns the orientation weighed length.
/// Hence, if orVec is parallel to the line, the length is 0.
/// If it is orthogonal, the line length is returned.
/// </summary>
/// <param name="orVec">An orientation vector.</param>
/// <returns>The length weighed by the orientation</returns>
double Line::weightedLength(const Vector2D & orVec) const {

	Vector2D vec = orVec.normalVec();
	vec /= orVec.length();	// normalize
	return std::abs(vec * vector());
}

/// <summary>
/// Returns the line angle.
/// </summary>
/// <returns>The line angle [-pi,+pi] in radians.</returns>
double Line::angle() const {

	QPointF diff = mLine.p2() - mLine.p1();

	return atan2(diff.y(), diff.x());

}

/// <summary>
/// Returns the start point.
/// </summary>
/// <returns>The start point.</returns>
Vector2D Line::p1() const {
	return mLine.p1();
}

/// <summary>
/// Returns the end point.
/// </summary>
/// <returns>The end point.</returns>
Vector2D Line::p2() const {
	return mLine.p2();
}

Vector2D Line::center() const {

	Vector2D rv = p2() - p1();
	rv /= 2.0;
	return p1() + rv;
}

/// <summary>
/// Determines whether the specified m angle tresh is horizontal.
/// </summary>
/// <param name="mAngleTresh">The m angle tresh.</param>
/// <returns>
///   <c>true</c> if the specified m angle tresh is horizontal; otherwise, <c>false</c>.
/// </returns>
bool Line::isHorizontal(double mAngleTresh) const {
	
	double lineAngle = angle();
	double angleNewLine = Algorithms::normAngleRad(lineAngle, 0.0, CV_PI);
	
	double a = 0.0f;
	double diffangle = cv::min(fabs(Algorithms::normAngleRad(a, 0, CV_PI) - Algorithms::normAngleRad(angleNewLine, 0, CV_PI))
		, CV_PI - fabs(Algorithms::normAngleRad(a, 0, CV_PI) - Algorithms::normAngleRad(angleNewLine, 0, CV_PI)));

	if (diffangle <= mAngleTresh / 180.0*CV_PI)
		return true;
	else
		return false;

}

bool Line::isVertical(double mAngleTresh) const {
	
	double lineAngle = angle();
	double angleNewLine = Algorithms::normAngleRad(lineAngle, 0.0, CV_PI);
	
	double a = CV_PI*0.5;
	double diffangle = cv::min(fabs(Algorithms::normAngleRad(a, 0, CV_PI) - Algorithms::normAngleRad(angleNewLine, 0, CV_PI))
		, CV_PI - fabs(Algorithms::normAngleRad(a, 0, CV_PI) - Algorithms::normAngleRad(angleNewLine, 0, CV_PI)));

	if (diffangle <= mAngleTresh / 180.0*CV_PI)
		return true;
	else
		return false;

}

bool Line::isColinear(const Line &line, double threshold) const {

	double d1 = distance(line.center());
	double d2 = line.distance(center());

	if (d1 < threshold && d2 < threshold)
		return true;
	else
		return false;
}

bool Line::isClose(const Line & line, double threshold) const {

	double d1 = distance(line.p1());
	double d2 = distance(line.p2());
	double d3 = line.distance(p1());
	double d4 = line.distance(p2());

	if ((d1 < threshold && d2 < threshold) || (d3 < threshold && d4 < threshold))
		return true;
	else
		return false;
}

bool Line::intersects(const Line & line, QLineF::IntersectType t) const {

	return mLine.intersect(line.qLine(), 0) == t;
}

/// <summary>
/// Returns the intersection point of both lines.
/// This function returns an empty vector if the lines 
/// do not intersect within the bounds
/// </summary>
/// <param name="line">Another line.</param>
/// <returns>The line intersection.</returns>
Vector2D Line::intersection(const Line & line, QLineF::IntersectType t) const {

	QPointF p;

	QLineF::IntersectType it = mLine.intersect(line.qLine(), &p);

	if (it == t)
		return Vector2D(p);

	return Vector2D();
}

Vector2D Line::intersectionUnrestricted(const Line & line) const {
	QPointF p;

	QLineF::IntersectType it = mLine.intersect(line.qLine(), &p);

	if (it != QLineF::NoIntersection)
		return Vector2D(p);

	return Vector2D();
}

/// <summary>
/// Returns the line's orientation vector.
/// </summary>
/// <returns></returns>
Vector2D Line::vector() const {
	return p1() - p2();
}

void Line::draw(QPainter & p) const {

	QPen pen = p.pen();
	QPen penL = pen;
	
	if (mThickness > 0)
		penL.setWidthF(mThickness);
	p.setPen(penL);

	p.drawLine(mLine);
	p.setPen(pen);
}

/// <summary>
/// Extends the line until the borders of the box.
/// </summary>
/// <param name="box">the 'cropping' box.</param>
Line Line::extendBorder(const Rect & box) const {

	// form DkSnippet
	Vector2D gradient = vector();
	Vector2D gradientBorderEnd, gradientBorderStart;

	//if (gradient.x() == 0)		//line is vertical
	//	return Line(Vector2D(mLine.p1().x(), box.top()), Vector2D(mLine.p1().x(), box.bottom()));
	//if (gradient.y() == 0)		//line is horizontal
	//	return Line(box.left(), mLine.p1().y(), box.right(), mLine.p1().y());

	double xStart, xEnd, yStart, yEnd;

	//gradientBorder is needed to check if line cuts horizontal or vertical border
	//vector points to first or third quadrant
	if (gradient.x()*gradient.y() < 0) {
		gradientBorderEnd.setX(box.right() - mLine.p1().x());	//+
		gradientBorderEnd.setY(box.top() - mLine.p1().y());	//-

		gradientBorderStart.setX(box.left() - mLine.p1().x());	//-
		gradientBorderStart.setY(box.bottom() - mLine.p1().y());//+

		yEnd = box.top();
		yStart = box.bottom();
		xEnd = box.right();
		xStart = box.left();
		//vector goes down
		//vector points to second or fourth quadrant
	} else {
		gradientBorderEnd.setX(box.right() -mLine.p1().x());		//+
		gradientBorderEnd.setY(box.bottom()-mLine.p1().y());		//+

		gradientBorderStart.setX(box.left()-mLine.p1().x());		//-
		gradientBorderStart.setY(box.top() - mLine.p1().y());		//-

		yEnd = box.bottom();
		yStart = box.top();
		xEnd = box.right();
		xStart = box.left();
	}
	
	Vector2D start, end;

	if (std::abs(gradient.y()/gradient.x()) > std::abs(gradientBorderEnd.y()/gradientBorderEnd.x()))
		end = Vector2D(mLine.p1().x() + gradient.x()/gradient.y() * gradientBorderEnd.y(), yEnd);
	else
		end = Vector2D(xEnd, mLine.p1().y() + gradient.y()/gradient.x() * gradientBorderEnd.x());

	if (fabs(gradient.y()/gradient.x()) > fabs(gradientBorderStart.y()/gradientBorderStart.x()))
		start = Vector2D(mLine.p1().x() + gradient.x()/gradient.y() * gradientBorderStart.y() , yStart);
	else
		start = Vector2D(xStart, mLine.p1().y() + gradient.y()/gradient.x() * gradientBorderStart.x());


	if (mLine.p1().x() > p2().x()) {	//line direction is from right to left -> switch end points
		return Line(end, start);
	}
	
	return Line(start, end);
}

/// <summary>
/// Returns the minimum distance of the line endings of line l to the line endings of the current line instance.
/// </summary>
/// <param name="l">The line l to which the minimum distance is computed.</param>
/// <returns>The minimum distance.</returns>
double Line::minDistance(const Line& l) const {

	// we compute the squared line length for each 
	// of the 4 connecting lines
	// we could use the Vector2D class here (it's nicer) - but this is faster...

	// l1.p1 - l2.p1
	double dx = (mLine.p1().x() - l.qLine().p1().x());
	double dy = (mLine.p1().y() - l.qLine().p1().y());
	double dist = dx*dx + dy*dy;

	// l1.p1 - l2.p2
	dx = (mLine.p1().x() - l.qLine().p2().x());
	dy = (mLine.p1().y() - l.qLine().p2().y());
	dist = qMin(dist, dx*dx + dy*dy);

	// l1.p2 - l2.p1
	dx = (mLine.p2().x() - l.qLine().p1().x());
	dy = (mLine.p2().y() - l.qLine().p1().y());
	dist = qMin(dist, dx*dx + dy*dy);
	
	// l1.p2 - ls2.p2
	dx = (mLine.p2().x() - l.qLine().p2().x());
	dy = (mLine.p2().y() - l.qLine().p2().y());
	dist = qMin(dist, dx*dx + dy*dy);

	return std::sqrt(dist);
}

void Line::translate(cv::Point offset) {
	QPointF tmp((float)offset.x, (float)offset.y);

	mLine.translate(tmp);
}

void Line::scale(double s) {

	if (s == 1.0)
		return;

	mLine.setP1(mLine.p1() * s);
	mLine.setP2(mLine.p2() * s);

	mThickness *= (float)s;
}

/// <summary>
/// Returns the minimal distance of point p to the current line instance.
/// </summary>
/// <param name="p">The point p.</param>
/// <returns>The distance of point p.</returns>
double Line::distance(const Vector2D& p) const {

	Vector2D nVec(mLine.p2() - mLine.p1());
	nVec = nVec.normalVec();

	Vector2D linPt = p - Vector2D(mLine.p1());

	return abs((linPt * nVec))/nVec.length();
}

double Line::horizontalOverlap(const Line & l) const {
	double ol;
	//l is on the right side of mLine (no overlap)
	//or on the left side
	if (l.p1().x() > mLine.x2() || l.p2().x() < mLine.x1())
		ol = 0;
	else
		ol = std::min(mLine.x2(), l.p2().x()) - std::max(mLine.x1(), l.p1().x());

	return ol;
}

double Line::horizontalDistance(const Line & l, double threshold) const {

	double distance;
	distance = horizontalOverlap(l);

	//use symmetric distance
	double minP1 = rdf::Line::distance(l.p1());
	double minP1Rev = l.distance(mLine.p1());
	minP1 = minP1 < minP1Rev ? minP1 : minP1Rev;
	double minP2 = rdf::Line::distance(l.p2());
	double minP2rev = l.distance(mLine.p2());
	minP2 = minP2 < minP2rev ? minP2 : minP2rev;

	//double minP1 = std::abs(mLine.y1() - l.p1().y()) < std::abs(mLine.y1() - l.p2().y()) ? std::abs(mLine.y1() - l.p1().y()) : std::abs(mLine.y1() - l.p2().y());
	//double minP2 = std::abs(mLine.y2() - l.p1().y()) < std::abs(mLine.y2() - l.p2().y()) ? std::abs(mLine.y2() - l.p1().y()) : std::abs(mLine.y2() - l.p2().y());

	double maxP = minP1 > minP2 ? minP1 : minP2;
		
	if (maxP < threshold)
		return distance;
	else
		return 0;
}

double Line::verticalOverlap(const Line & l) const {

	double ol;
	//l is before mLine or mLine is before l
	if (l.p2().y() < mLine.y1() || l.p1().y() > mLine.y2())
		ol = 0;
	else
		ol = std::min(mLine.y2(), l.p2().y()) - std::max(mLine.y1(), l.p1().y());

	return ol;
}

double Line::verticalDistance(const Line & l, double threshold) const {
	double distance;
	distance = verticalOverlap(l);

	//use symmetric minimum distance
	double minP1 = rdf::Line::distance(l.p1());
	double minP1Rev = l.distance(mLine.p1());
	minP1 = minP1 < minP1Rev ? minP1 : minP1Rev;
	double minP2 = rdf::Line::distance(l.p2());
	double minP2rev = l.distance(mLine.p2());
	minP2 = minP2 < minP2rev ? minP2 : minP2rev;
	//double minP1 = std::abs(mLine.x1() - l.p1().x()) < std::abs(mLine.x1() - l.p2().x()) ? std::abs(mLine.x1() - l.p1().x()) : std::abs(mLine.x1() - l.p2().x());
	//double minP2 = std::abs(mLine.x2() - l.p1().x()) < std::abs(mLine.x2() - l.p2().x()) ? std::abs(mLine.x2() - l.p1().x()) : std::abs(mLine.x2() - l.p2().x());
	double maxP = minP1 > minP2 ? minP1 : minP2;

	if (maxP < threshold)
		return distance;
	else
		return 0;
}

/// <summary>
/// Calculated if point p is within the current line instance.
/// </summary>
/// <param name="p">The point p to be checked.</param>
/// <returns>True if p is within the current line instance.</returns>
bool Line::within(const Vector2D& p) const {

	Vector2D tmp = mLine.p2() - mLine.p1();
	Vector2D pe = p - mLine.p2();	//p-end
	Vector2D ps = p - mLine.p1();	//p-start
	
	return (tmp.x()*pe.x() + tmp.y()*pe.y()) * (tmp.x()*ps.x() + tmp.y()*ps.y()) < 0;

}

/// <summary>
/// Returns a line moved by the vector mVec.
/// </summary>
/// <param name="mVec">Move vector.</param>
/// <returns></returns>
Line Line::moved(const Vector2D & mVec) const {

	return Line(p1() + mVec, p2() + mVec);
}

bool Line::lessX1(const Line& l1, const Line& l2) {

	if (l1.p1().x() < l2.p1().x())
		return true;
	else
		return false;
}

bool Line::lessY1(const Line& l1, const Line& l2) {

	if (l1.p1().y() < l2.p1().y())
		return true;
	else
		return false;
}

cv::Mat Line::toMat(const Line & l) const {

	cv::Mat distMat = cv::Mat(1, 4, CV_64FC1);
	double* ptr = distMat.ptr<double>();

	ptr[0] = Vector2D(mLine.p1() - l.qLine().p1()).length();
	ptr[1] = Vector2D(mLine.p1() - l.qLine().p2()).length();
	ptr[2] = Vector2D(mLine.p2() - l.qLine().p1()).length();
	ptr[3] = Vector2D(mLine.p2() - l.qLine().p2()).length();

	return distMat;
}

/// <summary>
/// Merges the specified line l with the current line instance.
/// </summary>
/// <param name="l">The line to merge.</param>
/// <returns>The merged line.</returns>
Line Line::merge(const Line& l) const {

	cv::Mat dist = toMat(l);

	cv::Point maxIdxP;
	minMaxLoc(dist, 0, 0, 0, &maxIdxP);
	int maxIdx = maxIdxP.x;

	float thickness = mThickness < l.thickness() ? mThickness : l.thickness();
	Line mergedLine;

	switch (maxIdx) {
	case 0: mergedLine = Line(mLine.p1(), l.qLine().p1(), thickness);	break;
	case 1: mergedLine = Line(mLine.p1(), l.qLine().p2(), thickness);	break;
	case 2: mergedLine = Line(mLine.p2(), l.qLine().p1(), thickness);	break;
	case 3: mergedLine = Line(mLine.p2(), l.qLine().p2(), thickness);	break;
	}

	return mergedLine;
}

Line Line::mergeFit(const Line & l) const {

	QVector<Vector2D> pts;
	pts.push_back(l.p1());
	pts.push_back(l.p2());
	pts.push_back(p1());
	pts.push_back(p2());

	LineFitting newLineFit(pts);
	Line fittedLine = newLineFit.fitLine();
	//take thickness from the longest line
	float thickness = mThickness < l.thickness() ? mThickness : l.thickness();
	fittedLine.setThickness(thickness);

	return fittedLine;
}

/// <summary>
/// Calculates the gap line between line l and the current line instance.
/// </summary>
/// <param name="l">The line l to which a gap line is calculated.</param>
/// <returns>The gap line.</returns>
Line Line::gapLine(const Line& l) const {

	cv::Mat dist = toMat(l);

	cv::Point minIdxP;
	minMaxLoc(dist, 0, 0, &minIdxP);
	int minIdx = minIdxP.x;

	float thickness = mThickness < l.thickness() ? mThickness : l.thickness();
	Line gapLine;

	switch (minIdx) {
	case 0: gapLine = Line(mLine.p1(), l.qLine().p1(), thickness);	break;
	case 1: gapLine = Line(mLine.p1(), l.qLine().p2(), thickness);	break;
	case 2: gapLine = Line(mLine.p2(), l.qLine().p1(), thickness);	break;
	case 3: gapLine = Line(mLine.p2(), l.qLine().p2(), thickness);	break;
	}

	return gapLine;
}

/// <summary>
/// The angle difference of line l and the current line instance.
/// </summary>
/// <param name="l">The line l.</param>
/// <returns>The angle difference in rad.</returns>
double Line::diffAngle(const Line& l) const {

	double angleLine, angleL;

	//angleLine
	angleLine = Algorithms::normAngleRad(angle(), 0.0, CV_PI);
	angleLine = angleLine > CV_PI*0.5 ? CV_PI - angleLine : angleLine;
	angleL = Algorithms::normAngleRad(l.angle(), 0.0, CV_PI);
	angleL = angleL > CV_PI*0.5 ? CV_PI - angleL : angleL;
	
	return std::abs(angleLine - angleL);
}

// Vector2D --------------------------------------------------------------------
Vector2D::Vector2D() {
}

Vector2D::Vector2D(double x, double y) {
	mIsNull = false;
	mX = x;
	mY = y;
}

Vector2D::Vector2D(const QPoint & p) {
	mIsNull = p.isNull();
	mX = p.x();
	mY = p.y();
}

Vector2D::Vector2D(const QPointF & p) {
	mIsNull = p.isNull();
	mX = p.x();
	mY = p.y();
}

Vector2D::Vector2D(const QSize & s) {
	mIsNull = false;
	mX = s.width();
	mY = s.height();
}

Vector2D::Vector2D(const QSizeF & s) {
	mIsNull = false;
	mX = s.width();
	mY = s.height();
}

Vector2D::Vector2D(const cv::Point & p) {
	mIsNull = false;
	mX = p.x;
	mY = p.y;
}

Vector2D::Vector2D(const cv::Size & s) {
	mIsNull = false;
	mX = s.width;
	mY = s.height;
}

Vector2D Vector2D::max(const Vector2D & v1, const Vector2D & v2) {
	return Vector2D(qMax(v1.x(), v2.x()), qMax(v1.y(), v2.y()));
}

Vector2D Vector2D::min(const Vector2D & v1, const Vector2D & v2) {
	return Vector2D(qMin(v1.x(), v2.x()), qMin(v1.y(), v2.y()));
}

Vector2D Vector2D::max() {
	return Vector2D(std::numeric_limits<double>::max(), std::numeric_limits<double>::max());
}

Vector2D Vector2D::min() {
	return Vector2D(std::numeric_limits<double>::min(), std::numeric_limits<double>::min());
}

bool Vector2D::isNull() const {
	return mIsNull;
}

QDebug operator<<(QDebug d, const Vector2D& v) {

	d << qPrintable(v.toString());
	return d;
}

QDataStream& operator<<(QDataStream& s, const Vector2D& v) {

	s << v.toString();	// for now show children too
	return s;
}

void Vector2D::setX(double x) {
	mIsNull = false;
	mX = x;
}

void Vector2D::setY(double y) {
	mIsNull = false;
	mY = y;
}

Vector2D Vector2D::normalVec() const {
	return Vector2D(-y(), x());
}

QPoint Vector2D::toQPoint() const {
	return QPoint(qRound(x()), qRound(y()));
}

QPointF Vector2D::toQPointF() const {
	return QPointF(x(), y());
}

QSize Vector2D::toQSize() const {
	return QSize(qRound(x()), qRound(y()));
}

QSizeF Vector2D::toQSizeF() const {
	return QSizeF(x(), y());
}

cv::Point Vector2D::toCvPoint() const {
	return cv::Point(qRound(x()), qRound(y()));
}

cv::Point2d Vector2D::toCvPoint2d() const {
	return cv::Point2d(x(), y());
}

cv::Point2f Vector2D::toCvPoint2f() const {
	return cv::Point2f((float)x(), (float)y());
}

cv::Size Vector2D::toCvSize() const {
	return cv::Size(qRound(x()), qRound(y()));
}

/// <summary>
/// Converts the vector into a row cv::Mat.
/// </summary>
/// <returns>a 2 x 1 CV_64FC1 mat.</returns>
cv::Mat Vector2D::toMatRow() const {

	cv::Mat m(2, 1, CV_64FC1);
	double* mp = m.ptr<double>();
	mp[0] = mX;
	mp[1] = mY;

	return m;
}

/// <summary>
/// Converts the vector into a column cv::Mat.
/// </summary>
/// <returns>a 1 x 2 CV_64FC2 mat.</returns>
cv::Mat Vector2D::toMatCol() const {

	cv::Mat m(1, 2, CV_64FC1);
	double* mp = m.ptr<double>();
	mp[0] = mX;
	mp[1] = mY;

	return m;
}

QString Vector2D::toString() const {

	QString msg;
	msg += "[" + QString::number(x()) + ", " + QString::number(y()) + "]";
	return msg;
}

void Vector2D::draw(QPainter & p) const {
	p.drawPoint(toQPointF());
}

double Vector2D::angle() const {
	return std::atan(y()/x());
}

/// <summary>
/// Returns the vector's squared length.
/// </summary>
/// <returns></returns>
double Vector2D::sqLength() const {
	return mX*mX + mY*mY;
}

double Vector2D::length() const {
	return std::sqrt(mX*mX + mY*mY);
}

void Vector2D::rotate(double angle) {
	double xTmp = mX;
	mX =  xTmp * std::cos(angle) + mY * std::sin(angle);
	mY = -xTmp * std::sin(angle) + mY * std::cos(angle);
}

/// <summary>
/// Computes the length normalized dot product between the vector and o.
/// </summary>
/// <param name="o">Another vector.</param>
/// <returns>The cosinus of theta, the angle between both vectors</returns>
double Vector2D::theta(const Vector2D & o) const {
	
	double cosTheta = *this * o;
	double denom = length() * o.length();
	if (denom != 0.0)
		cosTheta /= denom;
	else
		return 0.0;
	
	return cosTheta;
}

bool Vector2D::isNeighbor(const Vector2D & p1, double radius) const {

	// speed things up a little
	if (abs(y() - p1.y()) > radius ||
		abs(x() - p1.x()) > radius)
		return false;

	Vector2D lVec(*this - p1);

	if (lVec.length() < radius)
		return true;

	return false;
}

// Triangle --------------------------------------------------------------------
Triangle::Triangle() {

}

Triangle::Triangle(const cv::Vec6f & vec) {
	setVec(vec);
}

bool Triangle::isNull() const {
	return mIsNull;
}

void Triangle::setVec(const cv::Vec6f & vec) {

	mIsNull = false;
	mPts << Vector2D(vec[0], vec[1]);
	mPts << Vector2D(vec[2], vec[3]);
	mPts << Vector2D(vec[4], vec[5]);
}

Vector2D Triangle::p0() const {
	return pointAt(0);
}

Vector2D Triangle::p1() const {
	return pointAt(1);
}

Vector2D Triangle::p2() const {
	return pointAt(2);
}

Vector2D Triangle::pointAt(int idx) const {

	// It's a triangle so the max index is expected to be 2
	assert(idx < 3);

	if (idx >= mPts.size())
		return Vector2D();
	else
		return mPts[idx];
}

void Triangle::draw(QPainter& p) const {

	if (isNull())
		return;

	// draw edges
	p.drawLine(p0().toQPointF(), p1().toQPointF());
	p.drawLine(p1().toQPointF(), p2().toQPointF());
	p.drawLine(p2().toQPointF(), p0().toQPointF());
}

// Rect --------------------------------------------------------------------
Rect::Rect() {
}

Rect::Rect(const QRect & rect) {
	mIsNull = false;
	mTopLeft = rect.topLeft();
	mSize = rect.size();
}

Rect::Rect(const QRectF & rect) {
	mIsNull = false;
	mTopLeft = rect.topLeft();
	mSize = rect.size();
}

Rect::Rect(const cv::Rect & rect) {
	mIsNull = false;
	mTopLeft = Vector2D(rect.x, rect.y);
	mSize = Vector2D(rect.width, rect.height);
}

Rect::Rect(const Vector2D & topLeft, const Vector2D & size) {
	mIsNull = false;
	mTopLeft = topLeft;
	mSize = size;
}

Rect::Rect(double x, double y, double width, double height) {
	mIsNull = false;
	mTopLeft = Vector2D(x,y);
	mSize = Vector2D(width, height);
}

Rect::Rect(const cv::Mat & img) {
	mIsNull = !img.empty();
	mSize = Vector2D(img.cols, img.rows);
}

bool operator==(const Rect & l, const Rect & r) {
	//return l.topLeft() == r.topLeft() && l.size() == r.size();
	return l.mTopLeft == r.mTopLeft && l.mSize == r.mSize;	// this is faster
}

bool operator!=(const Rect & l, const Rect & r) {
	return !(l == r);
}

bool Rect::isNull() const {
	return mIsNull;
}

double Rect::width() const {
	return mSize.x();
}

double Rect::height() const {
	return mSize.y();
}

Vector2D Rect::size() const {
	return mSize;
}

Vector2D Rect::diagonal() const {
	return bottomRight()-mTopLeft;
}

double Rect::top() const {
	return mTopLeft.y();
}

double Rect::bottom() const {
	return top()+height();
}

double Rect::left() const {
	return mTopLeft.x();
}

double Rect::right() const {
	return left()+width();
}

Vector2D Rect::topLeft() const {
	return mTopLeft;
}

Vector2D Rect::topRight() const {
	return Vector2D(right(), top());
}

Vector2D Rect::bottomLeft() const {
	return Vector2D(left(), bottom());
}

Vector2D Rect::bottomRight() const {
	return Vector2D(right(), bottom());
}

Vector2D Rect::center() const {
	return Vector2D(left()+width()/2.0, top()+height()/2.0);
}

void Rect::setTopLeft(const Vector2D & topLeft) {
	mTopLeft = topLeft;
	mIsNull = false;
}

void Rect::move(const Vector2D & vec) {
	mTopLeft += vec;
}

void Rect::scale(double factor) {
	mTopLeft *= factor;
	mSize *= factor;
}

void Rect::expand(double v) {
	mTopLeft -= 0.5*v;
	mSize += v;
}

void Rect::setSize(const Vector2D & newSize) {
	mSize = newSize;
}

QRect Rect::toQRect() const {
	return QRect(mTopLeft.toQPoint(), mSize.toQSize());
}

QRectF Rect::toQRectF() const {
	return QRectF(mTopLeft.toQPointF(), mSize.toQSizeF());
}

cv::Rect Rect::toCvRect() const {
	return cv::Rect(qRound(left()), qRound(top()), qRound(width()), qRound(height()));
}

bool Rect::contains(const Rect & o) const {
	
	return (top()	<= o.top() &&
			bottom()>= o.bottom() &&
			left()	<= o.left() &&
			right() >= o.right());
}

bool Rect::contains(const Vector2D & pt) const {

	return (top()	<= pt.y() &&
		left()		<= pt.x() &&
		bottom()	>= pt.y() &&
		right()		>= pt.x());
}

/// <summary>
/// Returns true if the centers are closer than eps.
/// NOTE: the city block metric is used to compare the centers.
/// </summary>
/// <param name="o">The other rect.</param>
/// <param name="eps">The epsilon region.</param>
/// <returns>
///   <c>true</c> if the specified o is close; otherwise, <c>false</c>.
/// </returns>
bool Rect::isProximate(const Rect & o, double eps) const {

	Vector2D c = center();
	Vector2D co = o.center();

	if (abs(c.x() - co.x()) > eps)
		return false;
	else if (abs(c.y() - co.y()) > eps)
		return false;

	return true;
}

double Rect::area() const {
	return width() * height();
}

Rect Rect::clipped(const Vector2D & size) const {

	Rect c(*this);

	if (left() < 0) {
		c.mSize.setX(mSize.x() + left());
		c.mTopLeft.setX(0);
	}
	if (top() < 0) {
		c.mSize.setY(mSize.y() + top());
		c.mTopLeft.setY(0);
	}
	if (c.right() > size.x())
		c.mSize.setX(qMax(size.x()-c.left(), 0.0));
	if (c.bottom() > size.y())
		c.mSize.setY(qMax(size.y()-c.top(), 0.0));

	return c;
}

Rect Rect::joined(const Rect & o) const {
	
	Vector2D tl = Vector2D::min(topLeft(), o.topLeft());
	Vector2D br = Vector2D::max(bottomRight(), o.bottomRight());

	return Rect(tl, br-tl);
}

Rect Rect::intersected(const Rect & o) const {
	
	Vector2D tl = Vector2D::max(topLeft(), o.topLeft());
	Vector2D br = Vector2D::min(bottomRight(), o.bottomRight());


	Vector2D s(br - tl);

	if (s.x() <= 0 || s.y() <= 0)
		return Rect();
	
	return Rect(tl, s);
}

bool Rect::intersects(const Rect & o) const {
	return !intersected(o).isNull();
}

void Rect::draw(QPainter & p) const {

	p.drawRect(toQRectF());
}

/// <summary>
/// Returns a bounding rectangle that encloses all points.
/// </summary>
/// <param name="pts">The point set.</param>
/// <returns></returns>
Rect Rect::fromPoints(const QVector<Vector2D>& pts) {

	double top = DBL_MAX, left = DBL_MAX;
	double bottom = -DBL_MAX, right = -DBL_MAX;

	for (const Vector2D& pt : pts) {

		if (pt.isNull())
			continue;

		if (pt.y() < top)
			top = pt.y();
		if (pt.y() > bottom)
			bottom = pt.y();
		if (pt.x() < left)
			left = pt.x();
		if (pt.x() > right)
			right = pt.x();
	}

	return Rect(left, top, right-left, bottom-top);
}

QString Rect::toString() const {
	
	QString msg = "Rect [";
	msg += "x: " + QString::number(mTopLeft.x());
	msg += ", y: " + QString::number(mTopLeft.y());
	msg += ", width: " + QString::number(mSize.x());
	msg += ", height: " + QString::number(mSize.y());

	return msg;
}

// Ellipse --------------------------------------------------------------------
Ellipse::Ellipse() {

}

Ellipse::Ellipse(const Vector2D & center, const Vector2D & axis, double angle) {
	
	mIsNull = false;
	mCenter = center;
	mAxis = axis;
	mAngle = angle;
}

Ellipse::Ellipse(const cv::RotatedRect & rect) {

	mIsNull = false;

	mCenter.setX(rect.center.x);
	mCenter.setY(rect.center.y);

	mAxis.setX(rect.size.width/2.0);
	mAxis.setY(rect.size.height/2.0);

	mAngle = rect.angle*DK_DEG2RAD;
}

QDebug operator<<(QDebug d, const Ellipse& e) {

	d << qPrintable(e.toString());
	return d;
}

Ellipse Ellipse::fromData(const std::vector<cv::Point>& pts) {
		
	// estimate center
	Vector2D c;
	for (const cv::Point& p : pts) {
		Vector2D cp(p);
		c += cp;
	}
	c /= (double)pts.size();

	// convert pts
	cv::Mat cPointsMat((int)pts.size(), 2, CV_32FC1);

	for (int rIdx = 0; rIdx < cPointsMat.rows; rIdx++) {
		float* ptrM = cPointsMat.ptr<float>(rIdx);

		ptrM[0] = (float)pts[rIdx].x;
		ptrM[1] = (float)pts[rIdx].y;
	}

	return fromData(cPointsMat, c);
}

Ellipse Ellipse::fromData(const QVector<Vector2D>& pts) {

	// convert pts
	cv::Mat cPointsMat((int)pts.size(), 2, CV_32FC1);
	Vector2D center;

	for (int rIdx = 0; rIdx < cPointsMat.rows; rIdx++) {
		float* ptrM = cPointsMat.ptr<float>(rIdx);

		const Vector2D& pt = pts[rIdx];

		ptrM[0] = (float)pt.x();
		ptrM[1] = (float)pt.y();
		center += pt;
	}
	
	center /= pts.size();

	return fromData(cPointsMat, center);
}

/// <summary>
/// Returns the best fitting ellipse.
/// </summary>
/// <param name="pts">a N x 2 CV_32FC1 matrix representing the 2D points.</param>
/// <param name="center">The ellipses center.</param>
/// <returns></returns>
Ellipse Ellipse::fromData(const cv::Mat & pts, const Vector2D & center) {
	
	// was only one point provided?
	if (pts.rows < 2)
		return Ellipse(center, Vector2D(1, 1));

	// find the angle
	cv::PCA pca(pts, cv::Mat(), CV_PCA_DATA_AS_ROW);

	Vector2D eVec(pca.eigenvectors.at<float>(0,0),
		pca.eigenvectors.at<float>(0,1));

	Ellipse e(center);
	e.setAngle(eVec.angle());

	// now compute and equalize the axis
	double ev0 = pca.eigenvalues.at<float>(0,0);
	double ev1 = pca.eigenvalues.at<float>(1,0);

	Vector2D axis(std::sqrt(ev0), std::sqrt(ev1));
	axis *= 2.0;	// two dimensions - (normalized value)

	e.setAxis(axis);

	return e;
}

bool Ellipse::isNull() const {
	return mIsNull;
}

bool Ellipse::isValid() const {
	return mCenter.x() == mCenter.x() && mCenter.y() == mCenter.y();	// check if the center is nan
}

QString Ellipse::toString() const {

	return QString("c %1 axis %3 angle %5")
		.arg(mCenter.toString())
		.arg(mAxis.toString())
		.arg(mAngle);
}

void Ellipse::setCenter(const Vector2D & center) {
	mIsNull = false;
	mCenter = center;
}


void Ellipse::setAxis(const Vector2D & axis) {
	mIsNull = false;
	mAxis = axis;
}

Vector2D Ellipse::axis() const {
	return mAxis;
}

/// <summary>
/// Scales the ellipse (center and size).
/// </summary>
/// <param name="factor">The scale factor.</param>
void Ellipse::scale(double factor) {
	mAxis *= factor;
	mCenter *= factor;
}

/// <summary>
/// The ellipses major axis.
/// </summary>
/// <returns></returns>
double Ellipse::majorAxis() const {
	return qMax(mAxis.x(), mAxis.y());
}

/// <summary>
/// The ellipse's minor axis.
/// </summary>
/// <returns></returns>
double Ellipse::minorAxis() const {
	return qMin(mAxis.x(), mAxis.y());
}

/// <summary>
/// The mean of major and minor axis.
/// </summary>
/// <returns></returns>
double Ellipse::radius() const {
	return (mAxis.x() + mAxis.y()) / 2.0;
}

double Ellipse::area() const {
	return CV_PI*mAxis.x()*mAxis.y();
}

/// <summary>
/// Returns the ellipse's axis aligned bounding box.
/// </summary>
/// <param name="squared">if set to <c>true</c>, the bounding box is squared (e.g. for pdfs).</param>
/// <returns></returns>
Rect Ellipse::bbox(bool squared) const {
	
	Vector2D a = mAxis;

	double ev1 = a.x();
	double ev2 = a.y();

	if (squared) {
		ev1 *= ev1;
		ev2 *= ev2;
	}

	// compute eigenvectors
	Vector2D v1(ev1, 0);
	Vector2D v2(0, ev2);

	// NOTE possible speed-up: create unit rotation vector, then scale it
	v1.rotate(-mAngle);
	v2.rotate(-mAngle);

	double mW = qMax(std::abs(v1.x()), std::abs(v2.x()));
	double mH = qMax(std::abs(v1.y()), std::abs(v2.y()));

	Rect r(Vector2D(-mW, -mH), Vector2D(mW * 2, mH * 2));
	r.move(center());

	return r;
}

/// <summary>
/// Converts the convidence ellipse to a covariance matrix.
/// </summary>
/// <returns>The covariance matrix (2 x 2 CV_64FC1)</returns>
cv::Mat Ellipse::toCov() const {

	// eigen values
	Vector2D a = mAxis;
	
	double ev1 = a.x();
	double ev2 = a.y();

	// compute eigenvectors
	Vector2D v1(ev1, 0);
	Vector2D v2(0, ev2);

	// NOTE possible speed-up: create unit rotation vector, then scale it
	v1.rotate(-mAngle);
	v2.rotate(-mAngle);

	cv::Mat e1 = v1.toMatRow();
	cv::Mat e2 = v2.toMatRow();

	// derived from : https://math.stackexchange.com/a/1119677
	cv::Mat cov = ev1*(e1*e1.t()) / (e1.t()*e1) + ev2*(e2*e2.t()) / (e2.t()*e2);
	cov = cov.mul(cov);	// we need to square the cov

	return cov;
}

void Ellipse::setAngle(double angle) {
	mIsNull = false;
	mAngle = angle;
}

double Ellipse::angle() const {
	return mAngle;
}

void Ellipse::move(const Vector2D & vec) {
	mCenter += vec;
}

void Ellipse::draw(QPainter& p, double alpha) const {

	if (isNull())
		return;

	QBrush b = p.brush();	// backup

	QColor col = p.pen().brush().color();
	col.setAlpha(qRound(alpha*255));
	p.setBrush(col);

	p.translate(mCenter.toQPointF());
	p.rotate(mAngle*DK_RAD2DEG);
	p.drawEllipse(QPointF(0.0, 0.0), mAxis.x(), mAxis.y());
	p.drawLine(QPointF(), QPointF(mAxis.x(), 0));
	p.rotate(-mAngle*DK_RAD2DEG);
	p.translate(-mCenter.toQPointF());

	// draw center
	p.drawPoint(mCenter.toQPointF());
	p.setBrush(b);
}

cv::Mat Ellipse::toBinaryMask(const Rect& box) const {

	Rect r = box;
	if (r.isNull())
		r = bbox();

	QImage img(r.size().toQSize(), QImage::Format_Grayscale8);
	img.fill(QColor(0, 0, 0));

	QPainter p(&img);
	p.setPen(QColor(255, 255, 255));
	p.setBrush(QColor(255, 255, 255));
	
	Vector2D c = bbox().size() * 0.5;
	p.translate(c.toQPointF());
	p.rotate(mAngle*DK_RAD2DEG);
	p.drawEllipse(QPointF(0.0, 0.0), mAxis.x(), mAxis.y());
	p.rotate(-mAngle*DK_RAD2DEG);
	p.translate(c.toQPointF());

	cv::Mat bw = Image::qImage2Mat(img);

	return bw;
}

void Ellipse::pdf(cv::Mat & img, const Rect& box) const {

	// the provided image has to correspond with the bbox size
	assert(img.depth() == CV_32FC1);

	// allocate the area to draw
	Rect bbc = box;
	if (bbc.isNull())
		bbc = bbox(true);	// squared bounding box
	
	bbc = bbc.clipped(img.size()-Vector2D(1,1));	// -(1,1) to account for rounding
	cv::Mat imgR = img(bbc.toCvRect());
	
	double st = sin(2*mAngle);
	double c2t = cos(mAngle);
	c2t *= c2t;
	double s2t = sin(mAngle);
	s2t *= s2t;

	double s2x = mAxis.x()*mAxis.x();
	double s2y = mAxis.y()*mAxis.y();

	// cache coefficients
	double a = c2t / (2 * s2x) + s2t / (2 * s2y);
	double b = -st/(4*s2x) + st/(4*s2y);
	double c = s2t / (2 * s2x) + c2t / (2 * s2y);

	double y = bbc.top()-center().y();
	for (int rIdx = 0; rIdx < imgR.rows; rIdx++, y++) {

		// img pointer
		float* ptr = imgR.ptr<float>(rIdx);
		double cy = c*(y*y);

		double x = bbc.left()-center().x();
		for (int cIdx = 0; cIdx < imgR.cols; cIdx++, x++) {

			double ax = a*(x*x);
			double bxy = 2.0 * b*(x*y);

			ptr[cIdx] += (float)exp(-(ax - bxy + cy));
		}
	}
}

/// <summary>
/// Returns a point on the ellipse at the specified angle.
/// 0 degree corresponds to the positive x-axis.
/// The angle is clockwise.
/// </summary>
/// <param name="angle">An angle.</param>
/// <returns></returns>
Vector2D Ellipse::getPoint(double angle) const {

	angle = -angle;		// make the angle clockwise
	double lAngle = angle + mAngle;

	double a = mAxis.x();
	double b = mAxis.y();

	// fix numerical issues
	if (b == 0)
		b = 1;

	double tt = std::tan(lAngle);
	double x = (a*b) / (std::sqrt(b*b + a*a * (tt*tt)));

	double xa = x / a;
	xa = xa*xa;
	
	// fix numerical issues
	if (xa > 1.0)
		xa = 1.0;

	double y = std::sqrt(1.0-xa) * b;

	Vector2D ptr(x, y);
	Vector2D pt(1, 0);
	pt *= ptr.length();
	pt.rotate(angle);

	// now transform to world coordinates
	pt += center();

	return pt;
}

LineSegment::LineSegment() {
}

LineSegment::LineSegment(Line l, Vector2D c, double th, Vector2D lo, double pr, double p, double len) {
	mLine = l;
	mCenter = c;
	mTheta = th;
	mLineOrient = lo;
	mPrec = pr;
	mP = p;
	mLength = len;
}

Line LineSegment::line() const {
	return mLine;
}

void LineSegment::setLine(double x1, double y1, double x2, double y2, double width) {
	mLine.setLine(QLineF((float)x1, (float)y1, (float)x2, (float)y2), (float)width);
}

void LineSegment::setLine(Line l) {
	mLine = l;
}

Vector2D LineSegment::center() const {
	return mCenter;
}

void LineSegment::setCenter(double x, double y) {
	mCenter.setX(x);
	mCenter.setY(y);
}

double LineSegment::theta() const {
	return mTheta;
}

void LineSegment::setTheta(double t) {
	mTheta = t;
}

Vector2D LineSegment::lineOrientation() const {
	return mLineOrient;
}

void LineSegment::setOrientation(double dx, double dy) {
	mLineOrient.setX(dx);
	mLineOrient.setY(dy);
}

double LineSegment::prec() const {
	return mPrec;
}

void LineSegment::setPrec(double p){
	mPrec = p;
}

double LineSegment::p() const {
	return mP;
}

void LineSegment::setP(double p) {
	mP = p;
}

double LineSegment::length() const {
	return mLength;
}

void LineSegment::setLength(double l) {
	mLength = l;
}

Vector2D LineSegment::rectIterIni() {
	double vxTmp[4], vyTmp[4];	/* used for iterator */
	int n, offset;

	/* build list of rectangle corners ordered
	in a circular way around the rectangle */
	vx[0] = mLine.p1().x() - mLineOrient.y() * mLine.thickness() / 2.0;
	vy[0] = mLine.p1().y() + mLineOrient.x() * mLine.thickness() / 2.0;
	vx[1] = mLine.p2().x() - mLineOrient.y() * mLine.thickness() / 2.0;
	vy[1] = mLine.p2().y() + mLineOrient.x() * mLine.thickness() / 2.0;
	vx[2] = mLine.p2().x() + mLineOrient.y() * mLine.thickness() / 2.0;
	vy[2] = mLine.p2().y() - mLineOrient.x() * mLine.thickness() / 2.0;
	vx[3] = mLine.p1().x() + mLineOrient.y() * mLine.thickness() / 2.0;
	vy[3] = mLine.p1().y() - mLineOrient.x() * mLine.thickness() / 2.0;

	/* compute rotation of index of corners needed so that the first
	point has the smaller x.

	if one side is vertical, thus two corners have the same smaller x
	value, the one with the largest y value is selected as the first.
	*/
	if (mLine.p1().x() <mLine.p2().x() && mLine.p1().y() <= mLine.p2().y()) offset = 0;
	else if (mLine.p1().x() >= mLine.p2().x() && mLine.p1().y() < mLine.p2().y()) offset = 1;
	else if (mLine.p1().x() >mLine.p2().x() && mLine.p1().y() >= mLine.p2().y()) offset = 2;
	else offset = 3;
	
	/* apply rotation of index. */
	/* apply rotation of index. */
	for (n = 0; n<4; n++)	{
		vxTmp[n] = vx[(offset + n) % 4];
		vyTmp[n] = vy[(offset + n) % 4];
	}
	//copy back
	for (n = 0; n < 4; n++) {
		vx[n] = vxTmp[n];
		vy[n] = vyTmp[n];
	}

	/* Set an initial condition.

	The values are set to values that will cause 'ri_inc' (that will
	be called immediately) to initialize correctly the first 'column'
	and compute the limits 'ys' and 'ye'.

	'y' is set to the integer value of vy[0], the starting corner.

	'ys' and 'ye' are set to very small values, so 'ri_inc' will
	notice that it needs to start a new 'column'.

	The smallest integer coordinate inside of the rectangle is
	'ceil(vx[0])'. The current 'x' value is set to that value minus
	one, so 'ri_inc' (that will increase x by one) will advance to
	the first 'column'.
	*/
	mX = (int)ceil(vx[0]) - 1;
	mY = (int)ceil(vy[0]);
	mYs = mYe = -DBL_MAX;

	return rectIterInc();
}

Vector2D LineSegment::rectIterInc() {

	/* if not at end of exploration,
	increase y value for next pixel in the 'column' */
	if (rectIterEnd()) mY++;

	/* if the end of the current 'column' is reached,
	and it is not the end of exploration,
	advance to the next 'column' */
	while ((double)(mY) > mYe && !rectIterEnd())
	{
		/* increase x, next 'column' */
		mX++;

		/* if end of exploration, return */
		if (rectIterEnd()) return getIterPt();
		//if (rectIterEnd()) return Vector2D(-1,-1);

		/* update lower y limit (start) for the new 'column'.

		We need to interpolate the y value that corresponds to the
		lower side of the rectangle. The first thing is to decide if
		the corresponding side is

		vx[0],vy[0] to vx[3],vy[3] or
		vx[3],vy[3] to vx[2],vy[2]

		Then, the side is interpolated for the x value of the
		'column'. But, if the side is vertical (as it could happen if
		the rectangle is vertical and we are dealing with the first
		or last 'columns') then we pick the lower value of the side
		by using 'inter_low'.
		*/
		if ((double)mX < vx[3])
			mYs = interLow((double)mX, vx[0], vy[0], vx[3], vy[3]);
		else
			mYs = interLow((double)mX, vx[3], vy[3], vx[2], vy[2]);

		/* update upper y limit (end) for the new 'column'.

		We need to interpolate the y value that corresponds to the
		upper side of the rectangle. The first thing is to decide if
		the corresponding side is

		vx[0],vy[0] to vx[1],vy[1] or
		vx[1],vy[1] to vx[2],vy[2]

		Then, the side is interpolated for the x value of the
		'column'. But, if the side is vertical (as it could happen if
		the rectangle is vertical and we are dealing with the first
		or last 'columns') then we pick the lower value of the side
		by using 'inter_low'.
		*/
		if ((double)mX < vx[1])
			mYe = interHigh((double)mX, vx[0], vy[0], vx[1], vy[1]);
		else
			mYe = interHigh((double)mX, vx[1], vy[1], vx[2], vy[2]);

		/* new y */
		mY = (int)ceil(mYs);
	}
	
	return getIterPt();
}

bool LineSegment::rectIterEnd() {
	/* if the current x value is larger than the largest
	x value in the rectangle (vx[2]), we know the full
	exploration of the rectangle is finished. */
	return (double)(mX) > vx[2];
}

Vector2D LineSegment::getIterPt()
{
	return Vector2D(mX,mY);
}

bool LineSegment::doubleEqual(double a, double b) {
	double abs_diff, aa, bb, abs_max;
	double relativeErrorFactor = 100;

	/* trivial case */
	if (a == b) return true;

	abs_diff = fabs(a - b);
	aa = fabs(a);
	bb = fabs(b);
	abs_max = aa > bb ? aa : bb;

	/* DBL_MIN is the smallest normalized number, thus, the smallest
	number whose relative error is bounded by DBL_EPSILON. For
	smaller numbers, the same quantization steps as for DBL_MIN
	are used. Then, for smaller numbers, a meaningful "relative"
	error should be computed by dividing the difference by DBL_MIN. */
	if (abs_max < DBL_MIN) abs_max = DBL_MIN;

	/* equal if relative error <= factor x eps */
	return (abs_diff / abs_max) <= (relativeErrorFactor * DBL_EPSILON);
}

/** Interpolate y value corresponding to 'x' value given, in
the line 'x1,y1' to 'x2,y2'; if 'x1=x2' return the smaller
of 'y1' and 'y2'.

The following restrictions are required:
- x1 <= x2
- x1 <= x
- x  <= x2
*/
double LineSegment::interLow(double x, double x1, double y1, double x2, double y2) {

	/* interpolation */
	if (doubleEqual(x1, x2) && y1<y2) return y1;
	if (doubleEqual(x1, x2) && y1>y2) return y2;
	return y1 + (x - x1) * (y2 - y1) / (x2 - x1);

}

/** Interpolate y value corresponding to 'x' value given, in
the line 'x1,y1' to 'x2,y2'; if 'x1=x2' return the larger
of 'y1' and 'y2'.

The following restrictions are required:
- x1 <= x2
- x1 <= x
- x  <= x2
*/
double LineSegment::interHigh(double x, double x1, double y1, double x2, double y2) {
	/* interpolation */
	if (doubleEqual(x1, x2) && y1<y2) return y2;
	if (doubleEqual(x1, x2) && y1>y2) return y1;
	return y1 + (x - x1) * (y2 - y1) / (x2 - x1);
}

LineCandidates::LineCandidates() {
}

LineCandidates::LineCandidates(Line referenceLine) {
	mReferenceLine = referenceLine;
	if (mReferenceLine.isHorizontal())
		mReferenceLine.sortEndpoints(true);
	else
		mReferenceLine.sortEndpoints(false);
}

void LineCandidates::setReferenceLine(Line referenceLine) {
	mReferenceLine = referenceLine;
	if (mReferenceLine.isHorizontal()) 
		mReferenceLine.sortEndpoints(true);
	else
		mReferenceLine.sortEndpoints(false);
}

Line LineCandidates::referenceLine() const {
	return mReferenceLine;
}

bool LineCandidates::isEmpty() const {
	return mLCandidatesIdx.isEmpty() ? true : false;
}

void LineCandidates::addCandidate(int lIdx, double o, double d) {
	mLCandidatesIdx.push_back(lIdx);
	mOverlaps.push_back(o);
	mDistances.push_back(d);
}

void LineCandidates::addCandidate(Line c, int lIdx) {

	if (mReferenceLine.isEmpty()) {
		qWarning() << "reference line is empty - candidate not added";
		return;
	}

	mLCandidatesIdx.push_back(lIdx);
	//mLCandidates.push_back(c);

	
	double overlap;
	if (mReferenceLine.isHorizontal()) {
		c.sortEndpoints(true);
		overlap = mReferenceLine.horizontalOverlap(c);
	}
	else {
		c.sortEndpoints(false);
		overlap = mReferenceLine.verticalOverlap(c);
	}
	mOverlaps.push_back(overlap);
	double dist = c.distance(mReferenceLine.center());
	mDistances.push_back(dist);
}

void LineCandidates::addLineCandidate(LineCandidates lc) {

	QVector<int> tmp1 = lc.candidatesIdx();
	QVector<double> tmp2 = lc.overlaps();
	QVector<double> tmp3 = lc.distances();
	rdf::Line tmpL = lc.referenceLine();

	mLCandidatesIdx.append(tmp1);
	mOverlaps.append(tmp2);
	mDistances.append(tmp3);

	mReferenceLine = mReferenceLine.merge(tmpL);
}

//QVector<QSharedPointer<LineCandidates>> LineCandidates::findColinearCandidates(QVector<rdf::Line> &lines, double distThreshold) {
//	//distThreshold = 20;
//	QVector<QSharedPointer<LineCandidates>> splitCandidates;
//
//	for (int lineIdx = 0; lineIdx < mLCandidatesIdx.size(); lineIdx++) {
//
//		QSharedPointer<rdf::LineCandidates> newLC(new rdf::LineCandidates());
//		newLC->setReferenceLine(mReferenceLine);
//		newLC->addCandidate(mLCandidatesIdx[lineIdx], mOverlaps[lineIdx], mDistances[lineIdx]);
//
//		for (int lineCmp = lineIdx+1; lineCmp < mLCandidatesIdx.size(); lineCmp++) {
//
//			if (lines[mLCandidatesIdx[lineIdx]].isColinear(lines[mLCandidatesIdx[lineCmp]], distThreshold)) {
//				newLC->addCandidate(mLCandidatesIdx[lineCmp], mOverlaps[mLCandidatesIdx[lineCmp]], mDistances[mLCandidatesIdx[lineCmp]]);
//			}
//		}
//		splitCandidates.push_back(newLC);
//
//
//	}
//
//	return splitCandidates;
//}

rdf::Line LineCandidates::mergedLine() {
	return rdf::Line();
}

rdf::Line LineCandidates::mergeLines(const QVector<rdf::Line>& l) {
	Line fittedLine;

	////rdf::LineCandidates tmpCand = cellsR[cellIdx]->leftLineC();
	QVector<int> cand;
	cand = sortByOverlap();

	if (cand.isEmpty())
		return fittedLine;

	//if only one line is present...
	if (cand.size() == 1)
		return l[cand[0]];

	QVector<Vector2D> pts;
	for (int i = 0; i < cand.size(); i++) {
		rdf::Line tmpLine = l[cand[i]];
		pts.push_back(tmpLine.p1());
		pts.push_back(tmpLine.p2());
	}
	LineFitting newLineFit(pts);
	fittedLine = newLineFit.fitLine();
	//take thickness from the longest line
	fittedLine.setThickness(l[cand[0]].thickness());

	return fittedLine;

	////simplified old version
	////LineFitting
	//rdf::Line newLine;
	//for (int i = 0; i < cand.size(); i++) {
	//	rdf::Line tmpLine = l[cand[i]];
	//	if (newLine.isEmpty()) {
	//		newLine = tmpLine;
	//	}
	//	else {
	//		double dist = newLine.distance(tmpLine.p1()) < newLine.distance(tmpLine.p2()) ? newLine.distance(tmpLine.p1()) : newLine.distance(tmpLine.p2());
	//		if (dist < 20) {
	//			//line is colinear
	//			newLine = newLine.merge(tmpLine);
	//		}
	//	}
	//}

	//return newLine;
}

QVector<int> LineCandidates::sortByOverlap() {

	//double refLength = mReferenceLine.length();

	if (mLCandidatesIdx.size() == mOverlaps.size()) {

		QVector<std::pair<double, int>> overLaps;

		for (int i = 0; i < mLCandidatesIdx.size(); i++) {
			overLaps.push_back(std::make_pair(mOverlaps[i] /*/ refLength*/, mLCandidatesIdx[i]));
		}

		//sort descending order
		std::sort(overLaps.begin(), overLaps.end(), std::greater<std::pair<double, int>>());
		QVector<int> result;
		for (int i = 0; i < overLaps.size(); i++) {
			result.push_back(overLaps[i].second);
		}

		return result;

	} else {
		qWarning() << "sortByOverlap different length of idx and overlap - return emtpy vector";
		return QVector<int>();
	}
}

QVector<int> LineCandidates::sortByDistance() {
	if (mLCandidatesIdx.size() == mDistances.size()) {

		QVector<std::pair<double, int>> distances;

		for (int i = 0; i < mLCandidatesIdx.size(); i++) {
			distances.push_back(std::make_pair(mDistances[i], mLCandidatesIdx[i]));
		}

		//sort ascending
		std::sort(distances.begin(), distances.end());
		QVector<int> result;
		for (int i = 0; i < distances.size(); i++) {
			result.push_back(distances[i].second);
		}

		return result;

	}
	else {
		qWarning() << "sortByDistance different length of idx and overlap - return emtpy vector";
		return QVector<int>();
	}
}



void LineCandidates::clear() {
	mLCandidatesIdx.clear();
	mOverlaps.clear();
	mDistances.clear();
}



//int LineCandidates::bestLineMatch(/*QSharedPointer<QVector<rdf::Line>> lines*/) {
//
//	double distance = std::numeric_limits<double>::max();
//
//	if (mReferenceLine.isEmpty() || mLCandidatesIdx.isEmpty())
//		return -1;
//
//	for (int idx = 0; idx < mLCandidatesIdx.size(); idx++) {
//
//		//rdf::Line testLine = (*lines)[mLCandidatesIdx[idx]];
//		
//		double currOverlap = mOverlaps[idx];
//		double currDistance = mDistances[idx];
//
//		
//
//
//	}
//
//
//
//	return -1;
//}

//QVector<Line> LineCandidates::candidates() const {
//	return mLCandidates;
//}

QVector<int> LineCandidates::candidatesIdx() const {
	return mLCandidatesIdx;
}

QVector<double> LineCandidates::overlaps() const {
	return mOverlaps;
}

QVector<double> LineCandidates::distances() const {
	return mDistances;
}

TableCellRaw::TableCellRaw() {
}

void TableCellRaw::setId(const QString & id) {
	mId = id;
}

QString TableCellRaw::id() const {
	return mId;
}

void TableCellRaw::setCustom(const QString & c) {
	mCostum = c;
}

QString TableCellRaw::custom() const {
	return mCostum;
}

void TableCellRaw::setRow(int r) {
	mRow = r;
}

int TableCellRaw::row() const {
	return mRow;
}

void TableCellRaw::setCol(int c) {
	mCol = c;
}

int TableCellRaw::col() const {
	return mCol;
}

void TableCellRaw::setPolygon(const Polygon & polygon) {
	mRefPoly = polygon;
}

Polygon TableCellRaw::polygon() const{
	return mRefPoly;
}

void TableCellRaw::setNewPolygon(const Polygon & polygon) {
	mNewPoly = polygon;
}

Polygon TableCellRaw::newPolygon() const {
	return mNewPoly;
}

void TableCellRaw::setRowSpan(int r) {
	mRowSpan = r;
}

int TableCellRaw::rowSpan() const {
	return mRowSpan;
}

void TableCellRaw::setColSpan(int c) {
	mColSpan = c;
}

int TableCellRaw::colSpan() const {
	return mColSpan;
}

void TableCellRaw::setLeftBorderVisible(bool b) {
	mLeftBorderVisible = b;
}

bool TableCellRaw::leftBorderVisible() const {
	return mLeftBorderVisible;
}

void TableCellRaw::setRightBorderVisible(bool b) {
	mRightBorderVisible = b;
}

bool TableCellRaw::rightBorderVisible() const {
	return mRightBorderVisible;
}

void TableCellRaw::setTopBorderVisible(bool b) {
	mTopBorderVisible = b;
}

bool TableCellRaw::topBorderVisible() const {
	return mTopBorderVisible;
}

void TableCellRaw::setBottomBorderVisible(bool b) {
	mBottomBorderVisible = b;
}

bool TableCellRaw::bottomBorderVisible() const {
	return mBottomBorderVisible;
}

void TableCellRaw::setLeftIdx(int i) {
	mLeftIdx.push_back(i);
}

QVector<int> TableCellRaw::leftIdx() const {
	return mLeftIdx;
}

void TableCellRaw::setRightIdx(int i) {
	mRightIdx.push_back(i);
}

QVector<int> TableCellRaw::rightIdx() const {
	return mRightIdx;
}

void TableCellRaw::setTopIdx(int i) {
	mTopIdx.push_back(i);
}

QVector<int> TableCellRaw::topIdx() const {
	return mTopIdx;
}

void TableCellRaw::setBottomIdx(int i) {
	mBottomIdx.push_back(i);
}

QVector<int> TableCellRaw::bottomIdx() const {
	return mBottomIdx;
}

void TableCellRaw::setHeader(bool b) {
	mHeader = b;
}

bool TableCellRaw::header() const {
	return mHeader;
}

void TableCellRaw::clearCandidates() {
	mLeftLine.clear();
	mRightLine.clear();
	mTopLine.clear();
	mBottomLine.clear();
}

void TableCellRaw::setLineCandidatesLeftLine(LineCandidates l) {
	mLeftLine = l;
}

LineCandidates TableCellRaw::leftLineC() const {
	return mLeftLine;
}

void TableCellRaw::setRefLineLeft(Line l) {
	mLeftLine.setReferenceLine(l);
}

void TableCellRaw::addLineCandidateLeft(Line c, int lIdx) {
	mLeftLine.addCandidate(c, lIdx);
}

void TableCellRaw::addLineCandidateLeft(LineCandidates l) {
	if (mLeftLine.isEmpty()) {
		mLeftLine = l;
	}
	else {
		mLeftLine.addLineCandidate(l);
	}

}

void TableCellRaw::clearLineCandidatesLeft() {
	mLeftLine.clear();
}

void TableCellRaw::setLineCandidatesRightLine(LineCandidates l) {
	mRightLine = l;
}

LineCandidates TableCellRaw::rightLineC() const {
	return mRightLine;
}

void TableCellRaw::setRefLineRight(Line l) {
	mRightLine.setReferenceLine(l);
}

void TableCellRaw::addLineCandidateRight(Line c, int lIdx) {
	mRightLine.addCandidate(c, lIdx);
}

void TableCellRaw::addLineCandidateRight(LineCandidates l) {
	if (mRightLine.isEmpty()) {
		mRightLine = l;
	}
	else {
		mRightLine.addLineCandidate(l);
	}

}

void TableCellRaw::clearLineCandidatesRight() {
	mRightLine.clear();
}

void TableCellRaw::setLineCandidatesTopLine(LineCandidates l) {
	mTopLine = l;
}

LineCandidates TableCellRaw::topLineC() const {
	return mTopLine;
}

void TableCellRaw::setRefLineTop(Line l) {
	mTopLine.setReferenceLine(l);
}

void TableCellRaw::addLineCandidateTop(Line c, int lIdx) {
	mTopLine.addCandidate(c, lIdx);
}

void TableCellRaw::addLineCandidateTop(LineCandidates l) {
	if (mTopLine.isEmpty()) {
		mTopLine = l;
	}
	else {
		mTopLine.addLineCandidate(l);
	}
}

void TableCellRaw::clearLineCandidatesTop() {
	mTopLine.clear();
}

void TableCellRaw::setLineCandidatesBottomLine(LineCandidates l) {
	mBottomLine = l;
}

LineCandidates TableCellRaw::bottomLineC() const {
	return mBottomLine;
}

void TableCellRaw::setRefLineBottom(Line l) {
	mBottomLine.setReferenceLine(l);
}

void TableCellRaw::addLineCandidateBottom(Line c, int lIdx) {
	mBottomLine.addCandidate(c, lIdx);
}

void TableCellRaw::addLineCandidateBottom(LineCandidates l) {

	if (mBottomLine.isEmpty()) {
		mBottomLine = l;
	} else {
		mBottomLine.addLineCandidate(l);
	}
}

void TableCellRaw::clearLineCandidatesBottom() {
	mBottomLine.clear();
}


void TableCellRaw::setCornerPts(QVector<int> cPts) {
	mRefCornerPts = cPts;
}

QVector<int> TableCellRaw::cornerPty() const {
	return mRefCornerPts;
}

double TableCellRaw::width() const {

	Line l = leftBorder();
	Line r = rightBorder();

	return l.distance(r.center());
}

double TableCellRaw::height() const {
	Line t = topBorder();
	Line b = bottomBorder();

	return t.distance(b.center());
}

rdf::Line TableCellRaw::topBorder() const {
	//check if cornerpoints are defined
	if (mRefPoly.size() >= 4 && mRefCornerPts.size() == 4) {

		QPointF p1 = mRefPoly.polygon()[mRefCornerPts[0]];
		QPointF p2 = mRefPoly.polygon()[mRefCornerPts[3]];
		QPolygonF tmp;
		tmp << p1 << p2;
		return rdf::Line(rdf::Polygon(tmp));

		//no cornerpoints but polygon is present
	}
	else if (mRefPoly.size() == 4 && mRefCornerPts.isEmpty()) {

		QPointF p1 = mRefPoly.polygon()[0];
		QPointF p2 = mRefPoly.polygon()[3];
		QPolygonF tmp;
		tmp << p1 << p2;
		return rdf::Line(rdf::Polygon(tmp));
	}

	return rdf::Line();
}

rdf::Line TableCellRaw::bottomBorder() const {
	if (mRefPoly.size() >= 4 && mRefCornerPts.size() == 4) {

		QPointF p1 = mRefPoly.polygon()[mRefCornerPts[1]];
		QPointF p2 = mRefPoly.polygon()[mRefCornerPts[2]];
		QPolygonF tmp;
		tmp << p1 << p2;
		return rdf::Line(rdf::Polygon(tmp));

	}
	else if (mRefPoly.size() == 4 && mRefCornerPts.isEmpty()) {
		QPointF p1 = mRefPoly.polygon()[1];
		QPointF p2 = mRefPoly.polygon()[2];
		QPolygonF tmp;
		tmp << p1 << p2;
		return rdf::Line(rdf::Polygon(tmp));
	}

	return rdf::Line();

}

rdf::Line TableCellRaw::leftBorder() const {
	if (mRefPoly.size() >= 4 && mRefCornerPts.size() == 4) {

		QPointF p1 = mRefPoly.polygon()[mRefCornerPts[0]];
		QPointF p2 = mRefPoly.polygon()[mRefCornerPts[1]];
		QPolygonF tmp;
		tmp << p1 << p2;
		return rdf::Line(rdf::Polygon(tmp));

	}
	else if (mRefPoly.size() == 4 && mRefCornerPts.isEmpty()) {
		QPointF p1 = mRefPoly.polygon()[0];
		QPointF p2 = mRefPoly.polygon()[1];
		QPolygonF tmp;
		tmp << p1 << p2;
		return rdf::Line(rdf::Polygon(tmp));
	}

	return rdf::Line();
}

rdf::Line TableCellRaw::rightBorder() const {
	if (mRefPoly.size() >= 4 && mRefCornerPts.size() == 4) {

		QPointF p1 = mRefPoly.polygon()[mRefCornerPts[3]];
		QPointF p2 = mRefPoly.polygon()[mRefCornerPts[2]];
		QPolygonF tmp;
		tmp << p1 << p2;
		return rdf::Line(rdf::Polygon(tmp));

	}
	else if (mRefPoly.size() == 4 && mRefCornerPts.isEmpty()) {
		QPointF p1 = mRefPoly.polygon()[3];
		QPointF p2 = mRefPoly.polygon()[2];
		QPolygonF tmp;
		tmp << p1 << p2;
		return rdf::Line(rdf::Polygon(tmp));
	}

	return rdf::Line();
}

rdf::Vector2D TableCellRaw::upperLeft() const {
	//check if cornerpoints are defined
	if (mRefPoly.size() >= 4 && mRefCornerPts.size() == 4) {

		QPointF p = mRefPoly.polygon()[mRefCornerPts[0]];
		return rdf::Vector2D(p);

		//no cornerpoints but polygon is present
	}
	else if (mRefPoly.size() == 4 && mRefCornerPts.isEmpty()) {

		QPointF p = mRefPoly.polygon()[0];
		return rdf::Vector2D(p);
	}

	return rdf::Vector2D();
}


bool TableCellRaw::operator<(const TableCellRaw & cell) const {
	if (row() == cell.row()) {
		return col() < cell.col();
	}
	else {
		return row() < cell.row();
	}
}

bool TableCellRaw::compareCells(const QSharedPointer<rdf::TableCellRaw> l1, const QSharedPointer<rdf::TableCellRaw> l2) {
	return *l1 < *l2;
}

}