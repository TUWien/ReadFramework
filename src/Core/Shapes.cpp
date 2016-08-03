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

Line::Line(const cv::Point p1, const cv::Point p2, float thickness) {
	mLine = QLine(QPoint(p1.x, p1.y), QPoint(p2.x, p2.y));
	mThickness = thickness;
}

Line::Line(const Vector2D & p1, const Vector2D & p2, float thickness) {
	mLine = QLine(p1.toQPoint(), p2.toQPoint());
	mThickness = thickness;
}

cv::Point Line::startPointCV() const {
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
double Line::length() const {
	QPoint diff = mLine.p2() - mLine.p1();

	return sqrt(diff.x()*diff.x() + diff.y()*diff.y());

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

bool Line::isHorizontal(float mAngleTresh) const
{
	double lineAngle = angle();

	float angleNewLine = Algorithms::instance().normAngleRad((float)lineAngle, 0.0f, (float)CV_PI);
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

bool Line::isVertical(float mAngleTresh) const
{
	double lineAngle = angle();
	//lineAngle = o = blob.blobOrientation
	//lineAngle = angle


	float angleNewLine = Algorithms::instance().normAngleRad((float)lineAngle, 0.0f, (float)CV_PI);
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

void Line::draw(QPainter & p) const {

	QPen pen = p.pen();
	QPen penL = pen;
	penL.setWidthF(mThickness);
	p.setPen(penL);

	p.drawLine(mLine);
	p.setPen(pen);
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

void Vector2D::operator+=(const Vector2D & vec) {
	mX += vec.x();
	mY += vec.y();
}

void Vector2D::operator-=(const Vector2D & vec) {
	mX -= vec.x();
	mY -= vec.y();
}

void Vector2D::operator*=(const double & scalar) {
	mX *= scalar;
	mY *= scalar;
}

void Vector2D::operator/=(const double & scalar) {
	mX /= scalar;
	mY /= scalar;
}

bool Vector2D::isNull() const {
	return mIsNull;
}

QDebug operator<<(QDebug d, const Vector2D& v) {

	d << qPrintable(v.toString());
	return d;
}

bool operator==(const Vector2D & l, const Vector2D & r) {
	return l.mX == r.mX && l.mY == r.mY;
}

bool operator!=(const Vector2D & l, const Vector2D & r) {
	return !(l == r);
}

Vector2D operator+(const Vector2D & l, const Vector2D & r) {
	return Vector2D(l.x() + r.x(), l.y() + r.y());
}

Vector2D operator-(const Vector2D & l, const Vector2D & r) {
	return Vector2D(l.x() - r.x(), l.y() - r.y());
}

QDataStream& operator<<(QDataStream& s, const Vector2D& v) {

	s << v.toString();	// for now show children too
	return s;
}

void Vector2D::setX(double x) {
	mIsNull = false;
	mX = x;
}

double Vector2D::x() const {
	return mX;
}

void Vector2D::setY(double y) {
	mIsNull = false;
	mY = y;
}

double Vector2D::y() const {
	return mY;
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

cv::Point2d Vector2D::toCvPointF() const {
	return cv::Point2d(x(), y());
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

Vector2D Ellipse::center() const {
	return mCenter;
}

void Ellipse::setAxis(const Vector2D & axis) {
	mIsNull = false;
	mAxis = axis;
}

Vector2D Ellipse::axis() const {
	return mAxis;
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

	QPen pen = p.pen();
	QColor col = pen.brush().color();
	col.setAlpha(qRound(alpha*100));
	//pen.setColor(col);
	p.setBrush(col);

	p.translate(mCenter.toQPointF());
	p.rotate(mAngle*DK_RAD2DEG);
	p.drawEllipse(QPointF(0.0, 0.0), mAxis.x(), mAxis.y());
	p.rotate(-mAngle*DK_RAD2DEG);
	p.translate(-mCenter.toQPointF());

	// draw center
	p.drawPoint(mCenter.toQPointF());

}

}