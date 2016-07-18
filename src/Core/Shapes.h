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

// Qt defines
namespace rdf {

// read defines

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
	Line(const QLine& line = QLine(), float thickness = 0);
	Line(const Polygon& poly);


	cv::Point startPointCV() const;
	cv::Point endPointCV() const;
	bool isEmpty() const;
	void setLine(const QLine& line, float thickness = 0);
	QLine line() const;
	float thickness() const;
	float length() const;
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

protected:
	QLine mLine;
	float mThickness;
};

};