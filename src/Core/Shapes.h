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
	/// <summary>
	/// Initializes a new instance of the <see cref="Line"/> class.
	/// </summary>
	/// <param name="line">The line.</param>
	/// <param name="thickness">The stroke width.</param>
	Line(const QLine& line = QLine(), float thickness = 0);

	/// <summary>
	/// Determines whether this instance is empty.
	/// </summary>
	/// <returns>True if no line is set</returns>
	bool isEmpty() const;

	/// <summary>
	/// Sets the line.
	/// </summary>
	/// <param name="line">The line.</param>
	/// <param name="thickness">The stroke width.</param>
	void setLine(const QLine& line, float thickness = 0);

	/// <summary>
	/// Returns the line information.
	/// </summary>
	/// <returns>The line.</returns>
	QLine line() const;

	/// <summary>
	/// Returns the stroke width of the line.
	/// </summary>
	/// <returns>The stroke width.</returns>
	float thickness() const;

	/// <summary>
	/// Returns the line length.
	/// </summary>
	/// <returns>The line length.</returns>
	float length() const;

	/// <summary>
	/// Returns the line angle.
	/// </summary>
	/// <returns>The line angle [-pi,+pi] in radians.</returns>
	double angle() const;

	/// <summary>
	/// Returns the minimum distance of the line endings of line l to the line endings of the current line instance.
	/// </summary>
	/// <param name="l">The line l to which the minimum distance is computed.</param>
	/// <returns>The minimum distance.</returns>
	float minDistance(const Line& l) const;

	/// <summary>
	/// Returns the minimal distance of point p to the current line instance.
	/// </summary>
	/// <param name="p">The p.</param>
	/// <returns>The distance of point p.</returns>
	float distance(const QPoint p) const;

	/// <summary>
	/// Merges the specified line l with the current line instance.
	/// </summary>
	/// <param name="l">The line to merge.</param>
	/// <returns>The merged line.</returns>
	Line merge(const Line& l) const;

	/// <summary>
	/// Calculates the gap line between line l and the current line instance.
	/// </summary>
	/// <param name="l">The line l to which a gap line is calculated.</param>
	/// <returns>The gap line.</returns>
	Line gapLine(const Line& l) const;

	/// <summary>
	/// The angle difference of line l and the current line instance.
	/// </summary>
	/// <param name="l">The line l.</param>
	/// <returns>The angle difference in rad.</returns>
	float diffAngle(const Line& l) const;

	/// <summary>
	/// Calculated if point p is within the current line instance.
	/// </summary>
	/// <param name="p">The point p to be checked.</param>
	/// <returns>True if p is within the current line instance.</returns>
	bool within(const QPoint& p) const;

	/// <summary>
	/// Returns the start point.
	/// </summary>
	/// <returns>The start point.</returns>
	QPoint startPoint() const;

	/// <summary>
	/// Returns the end point.
	/// </summary>
	/// <returns>The end point.</returns>
	QPoint endPoint() const;

protected:
	QLine mLine;
	float mThickness;
};

};