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

class DllCoreExport PixelStats : public BaseElement {

public:
	PixelStats(const cv::Mat& orHist = cv::Mat(), 
		const cv::Mat& sparsity = cv::Mat(), 
		double scale = 0.0, 
		const QString& id = QString());

	/* row index of data */
	enum DataIndex {
		all_data = -1,
		max_val_idx,
		sparsity_idx,
		spacing_idx,
		combined_idx,

		idx_end,
	};

	bool isEmpty() const;

	void setOrientationIndex(int orIdx);
	int orientationIndex() const;
	double orientation() const;
	
	double scale() const;

	int numOrientations() const;

	int lineSpacingIndex() const;
	double lineSpacing() const;

	double minVal() const;

	cv::Mat data(const DataIndex& dIdx = all_data);

protected:
	cv::Mat mData;	// MxN orientation histograms M ... idx_end and N ... number of orientations
	double mScale = 0.0;
	double mMinVal = 0.0;

	int mHistSize = 0;
	int mOrIdx = -1;

	//int mDominantOrIdx = -1;
	//int mDominantScaleIdx = -1;
	//int mDominantPeakIdx = -1;
	//double mDominantPeakVal = 0.0;

	void convertData(const cv::Mat& orHist, const cv::Mat& sparsity);
};

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

	inline Vector2D center() const {
		return mEllipse.center();
	};

	Rect bbox() const {
		return mBBox;
	};

	Vector2D size() const;
	double angle() const;
	Ellipse ellipse() const;
	
	void addStats(const QSharedPointer<PixelStats>& stats);
	QSharedPointer<PixelStats> stats(int idx = -1) const;

	void draw(QPainter& p, double alpha = 0.3, bool showEllipse = false, bool showId = false) const;

protected:
	bool mIsNull = true;

	Ellipse mEllipse;
	Rect mBBox;
	QVector<QSharedPointer<PixelStats> > mStats;
};

class DllCoreExport PixelEdge : public BaseElement {

public:
	PixelEdge();

	PixelEdge(const QSharedPointer<Pixel> first, 
		const QSharedPointer<Pixel> second,
		const QString& id = QString());

	bool isNull() const;

	double edgeWeight() const;
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
	PixelSet(const QVector<QSharedPointer<Pixel> >& set);

	bool contains(const QSharedPointer<Pixel>& pixel) const;
	void merge(const PixelSet& o);
	void add(const QSharedPointer<Pixel>& pixel);

	QVector<QSharedPointer<Pixel> > pixels() const;

	Polygon polygon();
	Rect boundingBox();

	void draw(QPainter& p);

	static QVector<QSharedPointer<PixelEdge> > connect(QVector<QSharedPointer<Pixel> >& superPixels, const Rect& rect);

protected:
	QVector<QSharedPointer<Pixel> > mSet;
};

};