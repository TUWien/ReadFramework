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
	Vector2D orVec() const;

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

	void setTabStop(const PixelTabStop& tabStop);
	PixelTabStop tabStop() const;

	enum DrawFlag {
		draw_ellipse_only = 0,
		draw_stats_only,
		draw_ellipse_stats,
		draw_all,

		draw_end
	};

	void draw(QPainter& p, double alpha = 0.3, const DrawFlag& df = draw_stats_only) const;

protected:
	bool mIsNull = true;

	Ellipse mEllipse;
	Rect mBBox;
	QVector<QSharedPointer<PixelStats> > mStats;
	PixelTabStop mTabStop;
};

class DllCoreExport PixelEdge : public BaseElement {

public:
	PixelEdge();

	PixelEdge(const QSharedPointer<Pixel> first, 
		const QSharedPointer<Pixel> second,
		const QString& id = QString());

	bool isNull() const;

	virtual double edgeWeight() const;
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

class DllCoreExport LineEdge : public PixelEdge {

public:
	LineEdge();

	LineEdge(const PixelEdge& pe);

	LineEdge(const QSharedPointer<Pixel> first, 
		const QSharedPointer<Pixel> second,
		const QString& id = QString());

	virtual double edgeWeight() const override;

protected:
	double mStatsWeight = 0.0;

	double statsWeight(const QSharedPointer<Pixel>& pixel) const;
	double calcWeight() const;
};

class DllCoreExport PixelSet : public BaseElement {

public:
	PixelSet();
	PixelSet(const QVector<QSharedPointer<Pixel> >& set);

	enum ConnectionMode {
		connect_delauney,
		connect_region,

		connect_end
	};

	bool contains(const QSharedPointer<Pixel>& pixel) const;
	void merge(const PixelSet& o);
	void add(const QSharedPointer<Pixel>& pixel);
	void remove(const QSharedPointer<Pixel>& pixel);

	QVector<QSharedPointer<Pixel> > pixels() const;

	QVector<Vector2D> pointSet(double offsetAngle = 0.0) const;
	Polygon polygon() const;
	Rect boundingBox() const;
	Line fitLine(double offsetAngle = 0.0) const;
	Ellipse profileRect() const;					// TODO: remove!

	void draw(QPainter& p) const;

	static QVector<QSharedPointer<PixelEdge> > connect(const QVector<QSharedPointer<Pixel> >& superPixels, const Rect& rect, const ConnectionMode& mode = connect_delauney);
	static QVector<QSharedPointer<PixelSet> > fromEdges(const QVector<QSharedPointer<PixelEdge> >& edges);

protected:
	QVector<QSharedPointer<Pixel> > mSet;
};

/// <summary>
/// Abstract class PixelConnector.
/// This is the base class for all
/// pixel connecting classes which
/// implement different algorithms for
/// connecting super pixels.
/// </summary>
class DllCoreExport PixelConnector {

public:
	PixelConnector();

	virtual QVector<QSharedPointer<PixelEdge> > connect(const QVector<QSharedPointer<Pixel> >& pixels) const = 0;

};

/// <summary>
/// Connects pixels using the Delauney triangulation.
/// </summary>
/// <seealso cref="PixelConnector" />
class DllCoreExport DelauneyPixelConnector : public PixelConnector {

public:
	DelauneyPixelConnector(
		const Rect& r = Rect());

	virtual QVector<QSharedPointer<PixelEdge> > connect(const QVector<QSharedPointer<Pixel> >& pixels) const override;

	void setRect(const Rect& rect);

protected:
	Rect mRect;

};

/// <summary>
/// Fully connected graph.
/// Super pixels are connected with all other super pixels within a region.
/// </summary>
/// <seealso cref="PixelConnector" />
class DllCoreExport RegionPixelConnector : public PixelConnector {

public:
	RegionPixelConnector(double multiplier = 2.0);

	virtual QVector<QSharedPointer<PixelEdge> > connect(const QVector<QSharedPointer<Pixel> >& pixels) const override;

	void setRadius(double radius);
	void setLineSpacingMultiplier(double multiplier);

protected:
	double mRadius = 0.0;
	double mMultiplier = 2.0;

};

/// <summary>
/// Connects tab stops.
/// </summary>
/// <seealso cref="PixelConnector" />
class DllCoreExport TabStopPixelConnector : public PixelConnector {

public:
	TabStopPixelConnector();

	virtual QVector<QSharedPointer<PixelEdge> > connect(const QVector<QSharedPointer<Pixel> >& pixels) const override;

	void setLineSpacingMultiplier(double multiplier);

protected:
	double mMultiplier = 1.0;

};

class DllCoreExport PixelGraph : public BaseElement {

public:
	PixelGraph();
	PixelGraph(const QVector<QSharedPointer<Pixel> >& set);

	bool isEmpty() const;

	void draw(QPainter& p) const;
	void connect(const PixelConnector& connector = DelauneyPixelConnector());

	QSharedPointer<PixelSet> set() const;
	QVector<QSharedPointer<PixelEdge> > edges(const QString& pixelID) const;
	QVector<QSharedPointer<PixelEdge> > edges(const QVector<int>& edgeIDs) const;
	QVector<QSharedPointer<PixelEdge> > edges() const;

	int pixelIndex(const QString & pixelID) const;
	QVector<int> edgeIndexes(const QString & pixelID) const;

protected:
	QSharedPointer<PixelSet> mSet;
	QVector<QSharedPointer<PixelEdge> > mEdges;
	
	QMap<QString, int> mPixelLookup;			// maps pixel IDs to their current vector index
	QMap<QString, QVector<int> > mPixelEdges;	// maps pixel IDs to their corresponding edge index

};

};