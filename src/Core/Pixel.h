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

#pragma once

#include "Shapes.h"
#include "BaseImageElement.h"
#include "Utils.h"
#include "PixelLabel.h"
#include "Algorithms.h"
#include "ScaleFactory.h"

#pragma warning(push, 0)	// no warnings from includes
#include <QObject>
#include <QSharedPointer>
#include <QVector>
#include <QMap>
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
class PixelEdge;
class PixelLabel;
class TextLine;

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

/// <summary>
/// This class holds Pixel statistics which are found 
/// when computing the local orientation.
/// </summary>
/// <seealso cref="BaseElement" />
class DllCoreExport PixelStats : public BaseElement {

public:
	PixelStats(const cv::Mat& orHist = cv::Mat(),
		const cv::Mat& sparsity = cv::Mat(),
		double scale = 0.0,
		QSharedPointer<ScaleFactory> scaleFactory = QSharedPointer<ScaleFactory>(new ScaleFactory()),
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

	void setScaleFactory(const QSharedPointer<ScaleFactory>& scaleFactory);

	void setOrientationIndex(int orIdx);
	void setLineSpacing(int ls);
	int orientationIndex() const;
	int numOrientations() const;
	double orientation() const;
	Vector2D orVec() const;
	
	void scale(double factor) override;
	double scaleFactor() const;
	int lineSpacingIndex() const;
	double lineSpacing() const;

	double minVal() const;
	cv::Mat data(const DataIndex& dIdx = all_data);

	QString toString() const;
	
protected:
	cv::Mat mData;	// MxN orientation histograms M ... idx_end and N ... number of orientations
	double mScale = 0.0;
	double mMinVal = 0.0;
	QSharedPointer<ScaleFactory> mScaleFactory;

	int mHistSize = 0;
	int mOrIdx = -1;
	int mLineSpacing = -1;

	void convertData(const cv::Mat& orHist, const cv::Mat& sparsity);
};

class DllCoreExport PixelTabStop : public BaseElement {

public:

	enum Type {
		type_none,
		type_left,
		type_right,
		type_isolated,

		type_end

	};

	PixelTabStop(const Type& type = type_none);


	static PixelTabStop create(const QSharedPointer<Pixel>& pixel, const QVector<QSharedPointer<PixelEdge> >& edges);

	double orientation() const;
	Type type() const;

protected:
	Type mType;
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
	Pixel(const Ellipse& ellipse, const Rect& bbox = Rect(), const QString& id = QString());

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
	
	void scale(double factor) override;
	void move(const Vector2D& vec);

	void addStats(const QSharedPointer<PixelStats>& stats);
	QSharedPointer<PixelStats> stats(int idx = -1) const;

	void setTabStop(const PixelTabStop& tabStop);
	PixelTabStop tabStop() const;

	void setLabel(const QSharedPointer<PixelLabel>& label);
	QSharedPointer<PixelLabel> label() const;

	void setPyramidLevel(int level);
	int pyramidLevel() const;

	cv::KeyPoint toKeyPoint() const;

	void setValue(double value);
	double value() const;

	cv::Mat toBinaryMask(const Rect& r) const;

	enum mDrawFlags {
		draw_none				= 0x00,
		draw_ellipse			= 0x01,
		draw_stats				= 0x02,
		draw_center				= 0x04,
		draw_id					= 0x08,
		draw_label_colors		= 0x10,
		draw_tab_stops			= 0x20,

		draw_all				= 0x3f,
	};
	typedef Flags<mDrawFlags> DrawFlags;

	void draw(QPainter& p, double alpha = 0.3, const DrawFlags& df = DrawFlags() | /*draw_stats |*/ draw_ellipse | draw_label_colors) const;

protected:
	bool mIsNull = true;

	Ellipse mEllipse;
	Rect mBBox;
	QVector<QSharedPointer<PixelStats> > mStats;
	PixelTabStop mTabStop;
	QSharedPointer<PixelLabel> mLabel = QSharedPointer<PixelLabel>::create();
	double mValue = 0.0;	// e.g. gradient magnitude for GridPixels

	int mPyramidLevel = 0;	// 1.0/std::pow(2, mPyramidLevel) scales back to the pyramid
};

class DllCoreExport PixelEdge : public BaseElement {

public:
	PixelEdge();

	PixelEdge(const QSharedPointer<Pixel> first, 
		const QSharedPointer<Pixel> second,
		const QString& id = QString());

	friend DllCoreExport bool operator<(const PixelEdge& pe1, const PixelEdge& pe2);
	friend DllCoreExport bool operator<(const QSharedPointer<PixelEdge>& pe1, const QSharedPointer<PixelEdge>& pe2);

	virtual bool lessThan(const PixelEdge& e) const;

	bool isNull() const;

	void setEdgeWeightFunction(PixelDistance::EdgeWeightFunction& fnc);
	virtual double edgeWeightConst() const;
	virtual double edgeWeight();
	Line edge() const;

	QSharedPointer<Pixel> first() const;
	QSharedPointer<Pixel> second() const;

	void scale(double s) override;
	void draw(QPainter& p) const;

protected:
	bool mIsNull = true;
	double mEdgeWeight = DBL_MAX;

	PixelDistance::EdgeWeightFunction mWeightFnc = PixelDistance::spacingWeighted;
	QSharedPointer<Pixel> mFirst;
	QSharedPointer<Pixel> mSecond;
	Line mEdge;
};

class DllCoreExport LineEdge : public PixelEdge {

public:
	LineEdge();

	LineEdge(const PixelEdge& pe);

	LineEdge(const QSharedPointer<Pixel> first, 
		const QSharedPointer<Pixel> second,
		const QString& id = QString());
	friend DllCoreExport bool operator<(const QSharedPointer<LineEdge>& le1, const QSharedPointer<LineEdge>& le2);

	virtual double edgeWeightConst() const override;

protected:
	double mEdgeWeight = 0.0;

	double statsWeight(const QSharedPointer<Pixel>& pixel) const;
	double calcWeight() const;
};

}
