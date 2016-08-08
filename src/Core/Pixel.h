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

#include "Shapes.h"
#include "BaseImageElement.h"

#pragma warning(push, 0)	// no warnings from includes
#include <QObject>
#include <QSharedPointer>
#include <QVector>
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

class Pixel;

class DllCoreExport MserBlob : public BaseElement {

public:
	MserBlob(const std::vector<cv::Point>& pts = std::vector<cv::Point>(),
		const Rect& bbox = Rect(),
		const QString& id = QString());

	bool isNull() const;

	double area() const;
	Vector2D center() const;
	Rect bbox() const;

	std::vector<cv::Point> pts() const;
	std::vector<cv::Point> relativePts(const Vector2D& origin) const;
	
	//double uniqueArea(const QVector<MserBlob>& blobs) const;
	void draw(QPainter& p);

	// conversions
	QSharedPointer<Pixel> toPixel() const;
	cv::Mat toBinaryMask() const;

	double overlapArea(const Rect& r) const;

protected:
	Vector2D mCenter;
	Rect mBBox;
	std::vector<cv::Point> mPts;
};

// read defines
/// <summary>
/// This class represents a single instance
/// of super pixels which are needed for the 
/// layout analysis.
/// </summary>
/// <seealso cref="BaseElement" />
class DllCoreExport Pixel : public BaseElement {

public:
	Pixel();
	Pixel(const Ellipse& ellipse, const Rect& bbox, const QString& id = QString());

	bool isNull() const;

	Vector2D center() const;
	Vector2D size() const;
	double angle() const;
	Ellipse ellipse() const;
	Rect bbox() const;

	void draw(QPainter& p) const;

protected:
	bool mIsNull = true;

	Ellipse mEllipse;
	Rect mBBox;
};

class DllCoreExport PixelEdge : public BaseElement {

public:
	PixelEdge();

	PixelEdge(const QSharedPointer<Pixel> first, 
		const QSharedPointer<Pixel> second,
		const QString& id = QString());

	bool isNull() const;

	double scaledEdgeLength() const;
	Line edge() const;
	void draw(QPainter& p) const;

	QSharedPointer<Pixel> first() const;
	QSharedPointer<Pixel> second() const;

protected:
	bool mIsNull = true;

	QSharedPointer<Pixel> mFirst;
	QSharedPointer<Pixel> mSecond;
	Line mEdge;
};

class DllCoreExport PixelSet : public BaseElement {

public:
	PixelSet();
	//PixelSet(const QVector<QSharedPointer<Pixel> >& set);

	bool contains(const QSharedPointer<Pixel>& pixel) const;
	void merge(const PixelSet& o);
	void add(const QSharedPointer<Pixel>& pixel);

	QVector<QSharedPointer<Pixel> > pixels() const;

	Polygon polygon();
	Rect boundingBox();

	void draw(QPainter& p);

protected:
	QVector<QSharedPointer<Pixel> > mSet;
};

};