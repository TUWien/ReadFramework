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

#pragma once

#pragma warning(push, 0)	// no warnings from includes
#include <QPolygon>
#include <QLine>

#include "opencv2/core/core.hpp"
#pragma warning(pop)

#ifndef DllCoreExport
#ifdef DLL_CORE_EXPORT
#define DllCoreExport Q_DECL_EXPORT
#else
#define DllCoreExport Q_DECL_IMPORT
#endif
#endif

#pragma warning(disable: 4251)	// dll interface warning

// Qt defines
class QPainter;

namespace rdf {

// read defines
class Vector2D;

class DllCoreExport Polygon {

public:
	Polygon(const QPolygon& polygon = QPolygon());

	bool isEmpty() const;

	void read(const QString& pointList);
	QString write() const;

	int size() const;

	void setPolygon(const QPolygon& polygon);
	QPolygon polygon() const;
	QPolygon closedPolygon() const;

protected:
	QPolygon mPoly;

};

class DllCoreExport BaseLine {

public:
	BaseLine(const QPolygon& baseLine = QPolygon());

	bool isEmpty() const;

	void setPolygon(QPolygon& baseLine);
	QPolygon polygon() const;

	void read(const QString& pointList);
	QString write() const;

	QPoint startPoint() const;
	QPoint endPoint() const;

protected:
	QPolygon mBaseLine;
};


/// <summary>
/// A basic line class including stroke width (thickness).
/// </summary>
class DllCoreExport Line {

public:
	Line(const QLine& line = QLine(), float thickness = 1);
	Line(const Polygon& poly);
	Line(const cv::Point p1, const cv::Point p2, float thickness = 1);
	Line(const Vector2D& p1, const Vector2D& p2, float thickness = 1);

	cv::Point startPointCV() const;
	cv::Point endPointCV() const;
	bool isEmpty() const;
	void setLine(const QLine& line, float thickness = 1);
	QLine line() const;
	float thickness() const;
	double squaredLength() const;
	double length() const;
	double angle() const;
	float minDistance(const Line& l) const;
	float distance(const QPoint p) const;
	int horizontalOverlap(const Line& l) const;
	int verticalOverlap(const Line& l) const;
	Line merge(const Line& l) const;
	Line gapLine(const Line& l) const;
	float diffAngle(const Line& l) const;
	bool within(const QPoint& p) const;
	static bool lessX1(const Line& l1, const Line& l2);
	static bool lessY1(const Line& l1, const Line& l2);
	QPoint startPoint() const;
	QPoint endPoint() const;
	bool isHorizontal(float mAngleTresh = 0.5) const;
	bool isVertical(float mAngleTresh = 0.5) const;
	Vector2D vector() const;

	void draw(QPainter& p) const;

protected:
	QLine mLine;
	float mThickness = 1;
};

class DllCoreExport Vector2D {

public:
	Vector2D();
	Vector2D(double x, double y);
	Vector2D(const QPoint& p);
	Vector2D(const QPointF& p);
	Vector2D(const QSize& s);
	Vector2D(const QSizeF& s);
	Vector2D(const cv::Point& v);

	DllCoreExport friend QDataStream& operator<<(QDataStream& s, const Vector2D& v);
	DllCoreExport friend QDebug operator<< (QDebug d, const Vector2D &v);

	// binary operators
	DllCoreExport friend bool operator==(const Vector2D & l, const Vector2D & r) {
		return l.mX == r.mX && l.mY == r.mY;
	}

	DllCoreExport friend bool operator!=(const Vector2D & l, const Vector2D & r) {
		return !(l == r);
	}

	DllCoreExport friend Vector2D operator+(const Vector2D & l, const Vector2D & r) {
		return Vector2D(l.x() + r.x(), l.y() + r.y());
	}

	DllCoreExport friend Vector2D operator-(const Vector2D & l, const Vector2D & r) {
		return Vector2D(l.x() - r.x(), l.y() - r.y());
	}

	/// <summary>
	/// Computes the scalar product between l and r.
	/// </summary>
	/// <param name="l">A vector l.</param>
	/// <param name="r">A vector r.</param>
	/// <returns></returns>
	DllCoreExport friend double operator*(const Vector2D & l, const Vector2D & r) {
		return l.mX * r.mX + l.mY * r.mY;
	}
	
	// class member access operators
	void operator+=(const Vector2D& vec) {
		mX += vec.mX;
		mY += vec.mY;
	}

	void operator-=(const Vector2D& vec) {
		mX -= vec.mX;
		mY -= vec.mY;
	}

	// scalar operators
	void operator*=(const double& scalar) {
		mX *= scalar;
		mY *= scalar;
	};

	void operator/=(const double& scalar) {
		mX /= scalar;
		mY /= scalar;
	};

	void operator+=(const double& scalar) {
	
		mX += scalar;
		mY += scalar;
	};

	void operator-=(const double& scalar) {
		mX -= scalar;
		mY -= scalar;
	};

	// static functions
	static Vector2D max(const Vector2D& v1, const Vector2D& v2);
	static Vector2D min(const Vector2D& v1, const Vector2D& v2);
	static Vector2D max();
	static Vector2D min();

	bool isNull() const;

	void setX(double x);
	inline double x() const {
		return mX;
	};
	
	void setY(double y);
	double y() const {
		return mY;
	};

	QPoint toQPoint() const;
	QPointF toQPointF() const;
	QSize toQSize() const;
	QSizeF toQSizeF() const;

	cv::Point toCvPoint() const;
	cv::Point2d toCvPointF() const;
	cv::Size toCvSize() const;

	QString toString() const;

	void draw(QPainter& p) const;

	double angle() const;
	double length() const;
	void rotate(double angle);

protected:
	bool mIsNull = true;

	double mX = 0;
	double mY = 0;
};

class DllCoreExport Triangle {

public:
	Triangle();
	Triangle(const cv::Vec6f& vec);

	bool isNull() const;

	void setVec(const cv::Vec6f& vec);

	Vector2D p0() const;
	Vector2D p1() const;
	Vector2D p2() const;

	Vector2D pointAt(int idx = 0) const;

	void draw(QPainter& p) const;

protected:

	bool mIsNull = true;

	QVector<Vector2D> mPts;
};

class DllCoreExport Rect {

public:
	Rect();
	Rect(const QRect& rect);
	Rect(const QRectF& rect);
	Rect(const cv::Rect& rect);
	Rect(const Vector2D& topLeft, const Vector2D& size);
	Rect(double x, double y, double width, double height);
	Rect(const cv::Mat& img);

	DllCoreExport friend bool operator==(const Rect& l, const Rect& r);
	DllCoreExport friend bool operator!=(const Rect& l, const Rect& r);

	bool isNull() const;

	// return types
	double width() const;
	double height() const;
	Vector2D size() const;
	Vector2D diagonal() const;

	double top() const;
	double bottom() const;
	double left() const;
	double right() const;

	Vector2D topLeft() const;
	Vector2D topRight() const;
	Vector2D bottomLeft() const;
	Vector2D bottomRight() const;
	Vector2D center() const;

	// setter
	void move(const Vector2D& vec);
	void expand(double v);
	void setSize(const Vector2D& newSize);

	// conversions
	QRect toQRect() const;
	QRectF toQRectF() const;
	cv::Rect toCvRect() const;

	// geometry
	bool contains(const Rect& o) const;
	bool contains(const Vector2D& pt) const;
	bool isProximate(const Rect& o, double eps = 10.0) const;
	double area() const;
	

protected:
	bool mIsNull = true;
	
	Vector2D mTopLeft;
	Vector2D mSize;
};

class DllCoreExport Ellipse {

public:
	Ellipse();
	Ellipse(const Vector2D& center, const Vector2D& axis = Vector2D(), double angle = 0.0);
	Ellipse(const cv::RotatedRect& rect);

	DllCoreExport friend QDebug operator<< (QDebug d, const Ellipse &e);

	static Ellipse fromData(const std::vector<cv::Point>& pts);
	//static Ellipse fromData(const cv::Mat& means, const cv::Mat& covs);
	//static Ellipse fromImage(const cv::Mat& img);
	//bool axisFromCov(const cv::Mat& cov);

	bool isNull() const;

	QString toString() const;

	void setCenter(const Vector2D& center);
	inline Vector2D center() const {
		return mCenter;
	};

	void setAxis(const Vector2D& axis);
	Vector2D axis() const;

	double majorAxis() const;
	double minorAxis() const;

	void setAngle(double angle);
	double angle() const;

	void move(const Vector2D& vec);

	void draw(QPainter& p, double alpha = 0.0) const;

protected:
	
	bool mIsNull = true;

	Vector2D mCenter;
	Vector2D mAxis;
	double mAngle = 0.0;
};

};