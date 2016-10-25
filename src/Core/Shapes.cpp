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
#include <QPainter>
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
	return Converter::instance().polyToString(mPoly.toPolygon());
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

void Polygon::setPolygon(const QPolygonF & polygon) {
	mPoly = polygon;
}

void Polygon::draw(QPainter & p) const {
	
	QPen oPen = p.pen();
	QPen pen = oPen;
	pen.setWidth(3);
	p.setPen(pen);
	
	p.drawPolygon(closedPolygon());
	p.setPen(oPen);
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

// BaseLine --------------------------------------------------------------------
BaseLine::BaseLine(const QPolygon & baseLine) {
	mBaseLine = baseLine;
}

bool BaseLine::isEmpty() const {
	return mBaseLine.isEmpty();
}

void BaseLine::setPolygon(const QPolygon & baseLine) {
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
QLineF Line::line() const {
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

/// <summary>
/// Determines whether the specified m angle tresh is horizontal.
/// </summary>
/// <param name="mAngleTresh">The m angle tresh.</param>
/// <returns>
///   <c>true</c> if the specified m angle tresh is horizontal; otherwise, <c>false</c>.
/// </returns>
bool Line::isHorizontal(float mAngleTresh) const {
	
	double lineAngle = angle();

	double angleNewLine = Algorithms::instance().normAngleRad(lineAngle, 0.0f, CV_PI);
	//old version
	//angleNewLine = angleNewLine > (float)CV_PI*0.5f ? (float)CV_PI - angleNewLine : angleNewLine;

	//float diffangle = fabs(0.0f - angleNewLine);

	//angleNewLine = angleNewLine > (float)CV_PI*0.25f ? (float)CV_PI*0.5f - angleNewLine : angleNewLine;

	//diffangle = diffangle < fabs(0.0f - (float)angleNewLine) ? diffangle : fabs(0.0f - (float)angleNewLine);
	float a = 0.0f;
	float diffangle = (float)cv::min(fabs(Algorithms::instance().normAngleRad(a, 0, (float)CV_PI) - Algorithms::instance().normAngleRad(angleNewLine, 0, (float)CV_PI))
		, (float)CV_PI - fabs(Algorithms::instance().normAngleRad(a, 0, (float)CV_PI) - Algorithms::instance().normAngleRad(angleNewLine, 0, (float)CV_PI)));

	if (diffangle <= mAngleTresh / 180.0f*(float)CV_PI)
		return true;
	else
		return false;

}

bool Line::isVertical(float mAngleTresh) const {
	double lineAngle = angle();
	//lineAngle = o = blob.blobOrientation
	//lineAngle = angle


	double angleNewLine = Algorithms::instance().normAngleRad((float)lineAngle, 0.0f, (float)CV_PI);
	//old version
	//angleNewLine = angleNewLine > (float)CV_PI*0.5f ? (float)CV_PI - angleNewLine : angleNewLine;

	//float diffangle = fabs((float)CV_PI*0.5f - (float)angleNewLine);

	//angleNewLine = angleNewLine > (float)CV_PI*0.25f ? (float)CV_PI*0.5f - angleNewLine : angleNewLine;

	//diffangle = diffangle < fabs(0.0f - (float)angleNewLine) ? diffangle : fabs(0.0f - (float)angleNewLine);
	float a = (float)CV_PI*0.5f;
	float diffangle = (float)cv::min(fabs(Algorithms::instance().normAngleRad(a, 0, (float)CV_PI) - Algorithms::instance().normAngleRad(angleNewLine, 0, (float)CV_PI))
		, (float)CV_PI - fabs(Algorithms::instance().normAngleRad(a, 0, (float)CV_PI) - Algorithms::instance().normAngleRad(angleNewLine, 0, (float)CV_PI)));

	if (diffangle <= mAngleTresh / 180.0f*(float)CV_PI)
		return true;
	else
		return false;

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

	double dist1 = Vector2D(mLine.p1() - l.line().p1()).length();
	double dist2 = Vector2D(mLine.p1() - l.line().p2()).length();
	dist1 = (dist1 < dist2) ? dist1 : dist2;
	dist2 = Vector2D(mLine.p2() - l.line().p1()).length();
	dist1 = (dist1 < dist2) ? dist1 : dist2;
	dist2 = Vector2D(mLine.p2() - l.line().p2()).length();
	dist1 = (dist1 < dist2) ? dist1 : dist2;

	return dist1;
}

/// <summary>
/// Returns the minimal distance of point p to the current line instance.
/// </summary>
/// <param name="p">The p.</param>
/// <returns>The distance of point p.</returns>
double Line::distance(const Vector2D& p) const {

	Vector2D nVec(mLine.p2() - mLine.p1());
	nVec = nVec.normalVec();

	Vector2D linPt = p - Vector2D(mLine.p1());

	return abs((linPt * nVec))/nVec.length();
}

double Line::horizontalOverlap(const Line & l) const {
	double ol = std::max(mLine.x1(), l.p1().x()) - std::min(mLine.x2(), l.p2().x());

	return ol;
}

double Line::verticalOverlap(const Line & l) const {
	double ol = std::max(mLine.y1(), l.p1().y()) - std::min(mLine.y2(), l.p2().y());

	return ol;
}

/// <summary>
/// Calculated if point p is within the current line instance.
/// </summary>
/// <param name="p">The point p to be checked.</param>
/// <returns>True if p is within the current line instance.</returns>
bool Line::within(const Vector2D& p) const {

	Vector2D tmp = mLine.p2() - mLine.p1();
	Vector2D pe = p - mLine.p2();	//p-end
	Vector2D ps = p - mLine.p2();	//p-start
	
	return (tmp.x()*pe.x() + tmp.y()*pe.y()) * (tmp.x()*ps.x() + tmp.y()*ps.y()) < 0;

}

bool Line::lessX1(const Line& l1, const Line& l2) {

	if (l1.p1().x() < l2.p2().x())
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

	ptr[0] = Vector2D(mLine.p1() - l.line().p1()).length();
	ptr[1] = Vector2D(mLine.p1() - l.line().p2()).length();
	ptr[2] = Vector2D(mLine.p2() - l.line().p1()).length();
	ptr[3] = Vector2D(mLine.p2() - l.line().p2()).length();

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
	case 0: mergedLine = Line(mLine.p1(), l.line().p1(), thickness);	break;
	case 1: mergedLine = Line(mLine.p1(), l.line().p2(), thickness);	break;
	case 2: mergedLine = Line(mLine.p2(), l.line().p1(), thickness);	break;
	case 3: mergedLine = Line(mLine.p2(), l.line().p2(), thickness);	break;
	}

	return mergedLine;
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
	case 0: gapLine = Line(mLine.p1(), l.line().p1(), thickness);	break;
	case 1: gapLine = Line(mLine.p1(), l.line().p2(), thickness);	break;
	case 2: gapLine = Line(mLine.p2(), l.line().p1(), thickness);	break;
	case 3: gapLine = Line(mLine.p2(), l.line().p2(), thickness);	break;
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
	angleLine = Algorithms::instance().normAngleRad(angle(), 0.0, CV_PI);
	angleLine = angleLine > CV_PI*0.5 ? CV_PI - angleLine : angleLine;
	angleL = Algorithms::instance().normAngleRad(l.angle(), 0.0, CV_PI);
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

double Vector2D::length() const {
	return std::sqrt(mX*mX + mY*mY);
}

void Vector2D::rotate(double angle) {
	double xTmp = mX;
	mX =  xTmp * std::cos(angle) + mY * std::sin(angle);
	mY = -xTmp * std::sin(angle) + mY * std::cos(angle);
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

void Rect::move(const Vector2D & vec) {
	mTopLeft += vec;
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
	return cv::Rect(qRound(top()), qRound(left()), qRound(width()), qRound(height()));
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

void Rect::draw(QPainter & p) const {

	p.drawRect(toQRectF());
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

//Ellipse Ellipse::fromData(const cv::Mat & means, const cv::Mat & cov) {
//
//	// TODO: not tested
//	assert(means.depth() == CV_64FC1 && means.rows == 1 && means.cols == 2);
//	assert(cov.depth() == CV_64FC1 && cov.rows == 2 && cov.cols == 2);
//
//	// get the center
//	Ellipse e(Vector2D(means.ptr<double>()[0], means.ptr<double>()[1]));
//
//	if (e.axisFromCov(cov))
//		return e;
//	else
//		return Ellipse();
//}

Ellipse Ellipse::fromData(const std::vector<cv::Point>& pts) {
		
	// estimate center
	Vector2D c;
	for (const cv::Point& p : pts) {
		Vector2D cp(p);
		c += cp;
	}
	c /= (double)pts.size();

	Ellipse e(c);

	// convert pts
	cv::Mat cPointsMat((int)pts.size(), 2, CV_32FC1);

	for (int rIdx = 0; rIdx < cPointsMat.rows; rIdx++) {
		float* ptrM = cPointsMat.ptr<float>(rIdx);

		ptrM[0] = (float)pts[rIdx].x;
		ptrM[1] = (float)pts[rIdx].y;
	}

	// find the angle
	cv::PCA pca(cPointsMat, cv::Mat(), CV_PCA_DATA_AS_ROW);

	Vector2D eVec(pca.eigenvectors.at<float>(0,0),
				 pca.eigenvectors.at<float>(0,1));

	e.setAngle(eVec.angle());

	// now compute and equalize the axis
	double ev0 = pca.eigenvalues.at<float>(0,0);
	double ev1 = pca.eigenvalues.at<float>(1,0);

	Vector2D axis(std::sqrt(ev0), std::sqrt(ev1));
	axis *= 2.0;	// two dimensions - (normalized value)

	e.setAxis(axis);

	return e;
}

//Ellipse Ellipse::fromImage(const cv::Mat & img) {
//
//	// we expect a binary region here
//	assert(img.depth() == CV_8U);
//
//	cv::Moments m = cv::moments(img, true);
//
//	// get the center
//	double area = m.m00;
//	Vector2D c(m.m01 / area, m.m10 / area);
//	Ellipse e(c);
//
//	cv::Mat cov(2, 2, CV_64FC1);
//	cov.at<double>(0, 0) = m.nu21;
//	cov.at<double>(0, 1) = m.nu02;
//	cov.at<double>(1, 0) = m.nu20;
//	cov.at<double>(1, 1) = m.nu12;
//
//	if (e.axisFromCov(cov))
//		return e;
//	else
//		return Ellipse();
//}
//
//bool Ellipse::axisFromCov(const cv::Mat & cov) {
//
//	assert(cov.depth() == CV_64FC1);
//
//	cv::Mat eVal, eVec;
//	bool worked = cv::eigen(cov, eVal, eVec);
//
//	if (!worked) {
//		qWarning() << "warning cv::eigen did not seem to work...";
//		return false;
//	}
//
//	// get axis from eigen values
//	mAxis = Vector2D(eVal.ptr<double>()[0], eVal.ptr<double>(1)[0]);
//	
//	// get angle of first eigen vector
//	Vector2D vec(eVec.ptr<double>()[0], eVec.ptr<double>()[1]);
//	mAngle = vec.angle();
//
//	return true;
//}

bool Ellipse::isNull() const {
	return mIsNull;
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

double Ellipse::majorAxis() const {
	return qMax(mAxis.x(), mAxis.y());
}

double Ellipse::minorAxis() const {
	return qMin(mAxis.x(), mAxis.y());
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
	col.setAlpha(qRound(alpha*100));
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

/// <summary>
/// Returns the ellipse point of angle.
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
	
	double tt = std::tan(lAngle);
	double x = (a*b) / (std::sqrt(b*b + a*a * (tt*tt)));

	double xa = x / a;
	double y = std::sqrt(1.0-(xa*xa)) * b;

	Vector2D ptr(x, y);
	Vector2D pt(1, 0);
	pt *= ptr.length();
	pt.rotate(angle);

	// now transform to world coordinates
	pt += center();

	return pt;
}

}