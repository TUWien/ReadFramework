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
#include "Pixel.h"

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
class QPainter;

namespace rdf {

// elements
class TextLine;
class PixelSet;

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
	DelauneyPixelConnector();
	virtual QVector<QSharedPointer<PixelEdge> > connect(const QVector<QSharedPointer<Pixel> >& pixels) const override;
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

/// <summary>
/// Connects Pixels using the DBScan.
/// </summary>
/// <seealso cref="PixelConnector" />
class DllCoreExport DBScanPixelConnector : public PixelConnector {

public:
	DBScanPixelConnector();

	virtual QVector<QSharedPointer<PixelEdge> > connect(const QVector<QSharedPointer<Pixel> >& pixels) const override;

protected:
	double mEpsMultiplier = 1.2;

};

/// <summary>
/// PixelSet stores and manipulates pixel collections.
/// </summary>
/// <seealso cref="BaseElement" />
class DllCoreExport PixelSet : public BaseElement {

public:
	PixelSet();
	PixelSet(const QVector<QSharedPointer<Pixel> >& set);

	enum ConnectionMode {
		connect_delauney,
		connect_region,

		connect_end
	};

	enum DrawFlag {
		draw_nothing = 0x0,
		draw_pixels = 0x1,
		draw_poly = 0x2,

		draw_end
	};

	Q_DECLARE_FLAGS(DrawFlags, DrawFlag)

	QSharedPointer<Pixel> operator[](int idx) const;

	bool contains(const QSharedPointer<Pixel>& pixel) const;
	void merge(const PixelSet& o);
	void add(const QSharedPointer<Pixel>& pixel);
	void remove(const QSharedPointer<Pixel>& pixel);

	QVector<QSharedPointer<Pixel> > pixels() const;

	int size() const;
	QVector<Vector2D> pointSet(double offsetAngle = 0.0) const;
	Polygon convexHull() const;
	//Polygon polyLine(double angle, double maxCosThr = 0.9) const;
	Rect boundingBox() const;
	Line fitLine(double offsetAngle = 0.0) const;
	Ellipse profileRect() const;					// TODO: remove!

	double orientation(double statMoment = 0.5) const;
	double lineSpacing(double statMoment = 0.5) const;

	void draw(QPainter& p, const QFlag& options = draw_pixels | draw_poly) const;

	static QVector<QSharedPointer<PixelEdge> > connect(const QVector<QSharedPointer<Pixel> >& superPixels, const ConnectionMode& mode = connect_delauney);
	static QVector<QSharedPointer<PixelSet> > fromEdges(const QVector<QSharedPointer<PixelEdge> >& edges);
	QSharedPointer<TextLine> toTextLine() const;

protected:
	QVector<QSharedPointer<Pixel> > mSet;

	Polygon polygon(const QVector<Vector2D>& pts) const;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(PixelSet::DrawFlags)

/// <summary>
/// Represents a pixel graph.
/// This class comes in handy if you want
/// to map pixel edges with pixels.
/// </summary>
/// <seealso cref="BaseElement" />
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

/// <summary>
/// DBScan clustering for pixels.
/// </summary>
class DllCoreExport DBScanPixel {

public:
	DBScanPixel(const QVector<QSharedPointer<Pixel> >& pixels);

	void compute();

	void setEpsilonMultiplier(double eps);
	void setDistanceFunction(const PixelDistance::PixelDistanceFunction& distFnc);

	QVector<PixelSet> sets() const;
	QVector<QSharedPointer<PixelEdge> > edges() const;

protected:
	PixelSet mPixels;		// input


	enum Label {
		not_visited = 0,
		visited,
		noise, 

		cluster0
	};

	// cache
	cv::Mat mDists;
	cv::Mat mLabels;
	unsigned int* mLabelPtr;
	double mLineSpacing = 0;

	unsigned int mCLabel = cluster0;

	// parameters
	PixelDistance::PixelDistanceFunction mDistFnc = &PixelDistance::euclidean;
	double mEpsMultiplier = 2.0;
	int mMinPts = 3;

	void expandCluster(int pixelIndex, unsigned int clusterIndex, const QVector<int>& neighbors, double eps, int minPts) const;
	QVector<int> regionQuery(int pixelIdx, double eps) const;

	cv::Mat calcDists(const PixelSet& pixels) const;
};

};