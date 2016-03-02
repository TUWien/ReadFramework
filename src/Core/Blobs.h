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

#pragma warning(push, 0)	// no warnings from includes
// Qt Includes
#include <QPolygon>
#include <QVector>
#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp" 
#pragma warning(pop)

#ifndef DllCoreExport
#ifdef DLL_CORE_EXPORT
#define DllCoreExport Q_DECL_EXPORT
#else
#define DllCoreExport Q_DECL_IMPORT
#endif
#endif

#pragma warning(disable: 4251)
// Qt defines

namespace rdf {

	/// <summary>
	/// A class that defines a single blob within an image.
	/// </summary>
	class DllCoreExport Blob {

public:
	/// <summary>
	/// Initializes a new instance of the <see cref="Blob"/> class.
	/// </summary>
	Blob() {};

	/// <summary>
	/// Initializes a new instance of the <see cref="Blob"/> class.
	/// </summary>
	/// <param name="outerC">The outer contour of the blob.</param>
	/// <param name="innerC">The inner contours of the blob (can be more than one).</param>
	Blob(const QVector<cv::Point>& outerC, const QVector<QVector<cv::Point> >& innerC);

	/// <summary>
	/// Determines whether this instance is empty.
	/// </summary>
	/// <returns>True if emtpy.</returns>
	bool isEmpty() const;

	//void read(const QString& pointList);
	//QString write() const;

	//int size() const;

/// <summary>
/// Sets the blob.
/// </summary>
/// <param name="outerC">The outer contour.</param>
/// <param name="innerC">The inner contours.</param>
	void setBlob(const QVector<cv::Point>& outerC, const QVector<QVector<cv::Point> >& innerC);

	/// <summary>
	/// Returns the outer contour.
	/// </summary>
	/// <returns>The outer contour.</returns>
	QVector<cv::Point> outerContour() const;

	/// <summary>
	/// Returns the inner contours.
	/// </summary>
	/// <returns>The inner contours.</returns>
	QVector<QVector<cv::Point> > innerContours() const;
	
	/// <summary>
	/// Calculates the hierarchy for the blob such that it can be used with openCV functions.
	/// </summary>
	/// <returns>The OpenCV hierarchy.</returns>
	QVector<cv::Vec4i> hierarchy() const;

	/// <summary>
	/// The blobs orientation based on moments.
	/// </summary>
	/// <returns>The orientation of the blob in rad.</returns>
	float blobOrientation() const;

	/// <summary>
	/// Draws the Blob.
	/// </summary>
	/// <param name="imgSrc">The img source where the blob is drawn.</param>
	/// <param name="color">The color of the blob.</param>
	/// <returns>True if the blob is drawn.</returns>
	bool drawBlob(cv::Mat& imgSrc, cv::Scalar color = cv::Scalar(255, 255, 255), int maxLevel = 1) const;

protected:

	QVector<cv::Point> mOuterContour;
	QVector<QVector<cv::Point> > mInnerContours;
	
private:
	//QVector<cv::Vec4i> mHierarchy;
};


	/// <summary>
	/// Blobs class - holds a vector of Blobs and the corresponding image.
	/// </summary>
	class DllCoreExport Blobs {

public:
	/// <summary>
	/// Initializes a new instance of the <see cref="Blobs"/> class.
	/// </summary>
	Blobs();

	/// <summary>
	/// Determines whether this instance is empty (no blobs).
	/// </summary>
	/// <returns>True if the instance is empty.</returns>
	bool isEmpty() const;

	/// <summary>
	/// Sets the image.
	/// </summary>
	/// <param name="bWImg">The binary img CV_8UC1.</param>
	/// <returns>True if the image could be set.</returns>
	bool setImage(const cv::Mat& bWImg);

	/// <summary>
	/// Deletes the blobs.
	/// </summary>
	void deleteBlobs();

	/// <summary>
	/// Return all blobs.
	/// </summary>
	/// <returns>The blobs.</returns>
	QVector<Blob> blobs() const { return mBlobs; };

	/// <summary>
	/// Sets the blobs.
	/// </summary>
	/// <param name="blobs">The blobs.</param>
	void setBlobs(const QVector<Blob>& blobs) { mBlobs.clear(); mBlobs = blobs; }

	/// <summary>
	/// Returns the size of the image.
	/// </summary>
	/// <returns>The size of the image.</returns>
	cv::Size size() const { return mSize; };

	/// <summary>
	/// Computes all blobs of the binary image.
	/// </summary>
	/// <returns>True if the blobs could be computed.</returns>
	bool compute();

	//void read(const QString& pointList);
	//QString write() const;

	//int size() const;
	//bool drawBlob(cv::Mat imgSrc, cv::Scalar color = cv::Scalar(255, 255, 255)) const;

private:
	QVector<Blob> mBlobs;
	cv::Mat mBwImg;
	int mApproxMethod = CV_CHAIN_APPROX_SIMPLE;
	cv::Size mSize;

	bool checkInput() const;
};


	/// <summary>
	/// Allows to manipulate the Blobs class (filter, etc.)
	/// </summary>
	class DllCoreExport BlobManager {

public:
	/// <summary>
	/// Creates an instance or return the instance.
	/// </summary>
	/// <returns>The instance of BlobManager.</returns>
	static BlobManager& instance();

	/// <summary>
	/// Filters blobs according to a minimal area.
	/// Removes all blobs with an area smaller than area.
	/// </summary>
	/// <param name="area">The area threshold.</param>
	/// <param name="blobs">The blobs.</param>
	/// <returns>The filtered blob vector</returns>
	QVector<Blob> filterArea(int area, const Blobs& blobs) const;

	/// <summary>
	/// Filters blobs according to the size and the aspect ratio.
	/// </summary>
	/// <param name="maxAspectRatio">The maximum aspect ratio.</param>
	/// <param name="minWidth">The minimum width.</param>
	/// <param name="blobs">The blobs.</param>
	/// <returns>The filtered blob vector.</returns>
	QVector<Blob> filterMar(float maxAspectRatio, int minWidth, const Blobs& blobs) const;

	/// <summary>
	/// Filters blobs according to their orientation.
	/// </summary>
	/// <param name="angle">The specified orientation angle in rad.</param>
	/// <param name="maxAngleDiff">The maximum angle difference in degree.</param>
	/// <param name="blobs">The blobs.</param>
	/// <returns>The filtered blob vector</returns>
	QVector<Blob> filterAngle(float angle, float maxAngleDiff, const Blobs& blobs) const;

	/// <summary>
	/// Draws the blobs and returns a CV_8UC1 image.
	/// </summary>
	/// <param name="blobs">The blobs.</param>
	/// <param name="color">The color of the blobs.</param>
	/// <returns>A CV_8UC1 image containing all blobs.</returns>
	cv::Mat drawBlobs(const Blobs& blobs, cv::Scalar color = cv::Scalar(255, 255, 255)) const;

	/// <summary>
	/// Assumes all blobs as lines and fit a Line based on the minimum area rectangle.
	/// </summary>
	/// <param name="blobs">The blobs.</param>
	/// <returns>The line vector.</returns>
	QVector<Line> lines(const Blobs& blobs) const;

	/// <summary>
	/// Gets the biggest BLOB.
	/// </summary>
	/// <param name="blobs">The blobs.</param>
	/// <returns>The biggest blob.</returns>
	Blob getBiggestBlob(const Blobs& blobs) const;

private:
	BlobManager();
	BlobManager(const BlobManager&);
};

};