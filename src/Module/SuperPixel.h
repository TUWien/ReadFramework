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

#include "BaseModule.h"

#include "Shapes.h"
#include "Pixel.h"
#include "PixelSet.h"
#include "Image.h"	// TODO: remove (with GridPixel)

#pragma warning(push, 0)	// no warnings from includes
#include <QRectF>
#include <QVector>
#include <QMap>
#include <QPainter>
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

/// <summary>
/// Base class implementation for SuperPixel generating modules.
/// </summary>
/// <seealso cref="Module" />
class DllCoreExport SuperPixelBase : public Module {

public:
	SuperPixelBase(const cv::Mat& img);

	bool isEmpty() const override;

	virtual PixelSet pixelSet() const;

	void setPyramidLevel(int level);
	int pyramidLevel();

protected:
	cv::Mat mSrcImg;
	PixelSet mSet;

	int mPyramidLevel = 0;

	bool checkInput() const override;
};

/// <summary>
/// Configuration class for MserSuperPixel.
/// </summary>
/// <seealso cref="ModuleConfig" />
class DllCoreExport SuperPixelConfig : public ModuleConfig {

public:
	SuperPixelConfig();

	virtual QString toString() const override;

	int mserMinArea() const;
	int mserMaxArea() const;
	int erosionStep() const;
	
	void setNumErosionLayers(int numLayers);
	int numErosionLayers() const;


protected:
	int mMserMinArea = 25;
	int mMserMaxArea = 500;
	int mErosionStep = 4;
	int mNumErosionLayers = 3;

	void load(const QSettings& settings) override;
	void save(QSettings& settings) const override;
};

/// <summary>
/// Container for MSER elements.
/// This class maps OpenCVs MSER regions
/// with their bounding boxes.
/// </summary>
class MserContainer {

public:
	MserContainer() {};

	void append(const MserContainer& o);
	
	QVector<QSharedPointer<MserBlob> > toBlobs() const;
	size_t size() const;

	std::vector<std::vector<cv::Point> > pixels;
	std::vector<cv::Rect> boxes;
};

/// <summary>
/// SuperPixel generator using MSER regions.
/// An erosion pyramid improves the MSER
/// regions specifically if cursive handwriting
/// is present.
/// </summary>
/// <seealso cref="SuperPixelBase" />
class DllCoreExport SuperPixel : public SuperPixelBase {

public:
	SuperPixel(const cv::Mat& img);

	bool compute() override;
		
	QString toString() const override;
	QSharedPointer<SuperPixelConfig> config() const;

	// results - available after compute() is called
	QVector<QSharedPointer<MserBlob> > getMserBlobs() const;

	cv::Mat draw(const cv::Mat& img, const QColor& col = QColor()) const;
	cv::Mat drawMserBlobs(const cv::Mat& img, const QColor& col = QColor()) const;

private:
	// results
	QVector<QSharedPointer<MserBlob> > mBlobs;
	
	QSharedPointer<MserContainer> getBlobs(const cv::Mat& img, int kernelSize) const;
	QSharedPointer<MserContainer> mser(const cv::Mat& img) const;
	int filterAspectRatio(MserContainer& blobs, double aRatio = 0.1) const;
	int filterDuplicates(MserContainer& blobs, int eps = 5, int upperBound = -1) const;

};

/// <summary>
/// Configuration class for LineSuperPixel.
/// </summary>
/// <seealso cref="ModuleConfig" />
class DllCoreExport LinePixelConfig : public ModuleConfig {

public:
	LinePixelConfig();

	virtual QString toString() const override;

	int minLineLength() const;

protected:
	int mMinLineLength = 5;

	void load(const QSettings& settings) override;
	void save(QSettings& settings) const override;
};

/// <summary>
/// SuperPixel generator based on the LSD line detector.
/// </summary>
/// <seealso cref="SuperPixelBase" />
class DllCoreExport LineSuperPixel : public SuperPixelBase {

public:
	LineSuperPixel(const cv::Mat& img);

	bool compute() override;

	QString toString() const override;
	QSharedPointer<LinePixelConfig> config() const;

	cv::Mat draw(const cv::Mat& img) const;

protected:

	// results
	QVector<Line> mLines;	// debug only

	bool checkInput() const override;
};

/// <summary>
/// Configuration class for GridSuperPixel.
/// </summary>
/// <seealso cref="ModuleConfig" />
class DllCoreExport GridPixelConfig : public ModuleConfig {

public:
	GridPixelConfig();

	virtual QString toString() const override;

	int winSize() const;
	double winOverlap() const;
	double minEnergy() const;

protected:
	int mWinSize = 20;				// the window size in px per scale
	double mWinOverlap = 0.5;		// the window overlaps
	double mMinEnergy = 0.05;		// minimum energy per cell

	void load(const QSettings& settings) override;
	void save(QSettings& settings) const override;
};

class DllCoreExport GridPixel {

public:
	GridPixel(int index = -1, int numColumns = -1);

	bool operator==(const GridPixel& gpr);
	void compute(const cv::Mat& mag, const cv::Mat& phase, const cv::Mat& weight = cv::Mat());

	bool isDead() const;
	void kill();
	void move(const Vector2D& vec);

	int row() const;
	int col() const;

	int orIdx() const;

	Ellipse ellipse() const;
	void draw(QPainter& p) const;

	int index(int row, int col) const;
	QVector<int> neighbors() const;

private:

	int mIndex = -1;
	int mNumColumns = -1;
	
	// results
	int mEdgeCnt = 0;
	int mOrIdx = -1;
	//Histogram mOrHist;
	Ellipse mEllipse;
};

/// <summary>
/// Grid based SuperPixel extraction.
/// </summary>
/// <seealso cref="SuperPixelBase" />
class DllCoreExport GridSuperPixel : public SuperPixelBase {

public:
	GridSuperPixel(const cv::Mat& img);

	bool compute() override;

	QString toString() const override;
	QSharedPointer<GridPixelConfig> config() const;

	cv::Mat draw(const cv::Mat& img, const QColor& col = QColor()) const;

private:

	QVector<QSharedPointer<GridPixel> > mGridPixel;	// debug only

	QMap<int, QSharedPointer<GridPixel> > computeGrid(const cv::Mat& mag, const cv::Mat& phase, int winSize, double winOverlap) const;
	QVector<QSharedPointer<GridPixel> > merge(const QMap<int, QSharedPointer<GridPixel> >& pixels, const cv::Mat& mag, const cv::Mat& phase) const;

	void edges(const cv::Mat& src, cv::Mat& magnitude, cv::Mat& orientation) const;
	cv::Mat lineMask(const cv::Mat& src) const;
};

/// <summary>
/// Configuration file for local orientation extraction.
/// </summary>
/// <seealso cref="ModuleConfig" />
class DllCoreExport LocalOrientationConfig : public ModuleConfig {

public:
	LocalOrientationConfig();

	virtual QString toString() const override;

	int maxScale() const;
	int minScale() const;
	Vector2D scaleIvl() const;
	int numOrientations() const;
	int histSize() const;

	// changable parameters
	void setNumOrientations(int numOr);
	void setMaxScale(int maxScale);
	void setMinScale(int minScale);

protected:
	int mMaxScale = 256;	// radius (in px) of the maximum scale
	int mMinScale = 128;	// radius (in px) of the minimum scale
	int mNumOr = 32;		// number of orientation histograms
	int mHistSize = 64;		// size of the orientation histogram

	void load(const QSettings& settings) override;
	void save(QSettings& settings) const override;
};

/// <summary>
/// Local orientation estimation (using Il Koo's method).
/// </summary>
/// <seealso cref="Module" />
class DllCoreExport LocalOrientation : public Module {

public:
	LocalOrientation(const PixelSet& set = PixelSet());

	bool isEmpty() const override;
	bool compute() override;

	QString toString() const override;
	QSharedPointer<LocalOrientationConfig> config() const;

	// results - available after compute() is called
	PixelSet set() const;

	cv::Mat draw(const cv::Mat& img, const QString& id, double radius) const;

private:
	
	// input/output
	PixelSet mSet;

	bool checkInput() const override;

	void computeScales(Pixel* pixel, const QVector<Pixel*>& set) const;
	void computeAllOrHists(Pixel* pixel, const QVector<Pixel*>& set, double radius) const;
	void computeOrHist(const Pixel* pixel, 
		const QVector<const Pixel*>& set, 
		const Vector2D& histVec, 
		cv::Mat& orHist,
		float& sparsity) const;

};

}
