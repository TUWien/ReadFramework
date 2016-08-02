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

#pragma warning(push, 0)	// no warnings from includes
#include <QRectF>
#include <QVector>
#include <QPainter>
#pragma warning(pop)

#ifndef DllModuleExport
#ifdef DLL_MODULE_EXPORT
#define DllModuleExport Q_DECL_EXPORT
#else
#define DllModuleExport Q_DECL_IMPORT
#endif
#endif

// Qt defines

namespace rdf {

// read defines

class DllModuleExport MserBlob {

public:
	MserBlob(const std::vector<cv::Point>& pts = std::vector<cv::Point>(),
		const QRectF& bbox = QRectF());

	double area() const;
	double uniqueArea(const QVector<MserBlob>& blobs) const;
	Vector2D center() const;
	Rect bbox() const;

	std::vector<cv::Point> pts() const;
	std::vector<cv::Point> relativePts(const Vector2D& origin) const;

	void draw(QPainter& p);

	Vector2D getAxis() const;
	Pixel toPixel() const;

protected:
	Vector2D mCenter;
	Rect mBBox;
	std::vector<cv::Point> mPts;
};

class DllModuleExport SuperPixelConfig : public ModuleConfig {

public:
	SuperPixelConfig();

	virtual QString toString() const override;

protected:

	void load(const QSettings& settings) override;
	void save(QSettings& settings) const override;
};

class DllModuleExport SuperPixel : public Module {

public:
	SuperPixel(const cv::Mat& img);

	bool isEmpty() const override;
	bool compute() override;
	QVector<Triangle> connect(const QVector<MserBlob>& blobs) const;

	cv::Mat binaryImage() const;

	QString toString() const override;

	QVector<Pixel> getSuperPixels() const;

private:
	cv::Mat mSrcImg;
	cv::Mat mDstImg;

	QVector<MserBlob> mBlobs;
	QVector<Pixel> mPixels;
	QVector<Triangle> mTriangles;	// TODO: remove

	// parameters
	int mMinArea = 100;

	bool checkInput() const override;

	QVector<MserBlob> getBlobs(const cv::Mat& img, int kernelSize) const;
	QVector<MserBlob> mser(const cv::Mat& img) const;
	int filterAspectRatio(std::vector<std::vector<cv::Point> >& elements, std::vector<cv::Rect>& boxes, double aRatio = 0.4) const;
	int filterDuplicates(QVector<MserBlob>& blobs) const;
	int filterUnique(QVector<MserBlob>& blobs, double areaRatio = 0.7) const;
};

};