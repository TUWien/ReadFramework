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
#include <QSharedPointer>
#include <QSettings>

#include "opencv2/core/core.hpp"
#pragma warning(pop)

#pragma warning (disable: 4251)	// inlined Qt functions in dll interface

#ifndef DllCoreExport
#ifdef DLL_CORE_EXPORT
#define DllCoreExport Q_DECL_EXPORT
#else
#define DllCoreExport Q_DECL_IMPORT
#endif
#endif
// Qt defines

// Qt defines
class QSettings;

namespace rdf {

// read defines


/// <summary>
/// Computes robust statistical moments (quantiles).
/// </summary>
/// <param name="valuesIn">The statistical set (samples).</param>
/// <param name="momentValue">The statistical moment value (0.5 = median, 0.25 and 0.75 = quartiles).</param>
/// <param name="interpolated">A flag if the value should be interpolated if the length of the list is even.</param>
/// <returns>The statistical moment.</returns>
	template <typename numFmt>
static double statMoment(const QList<numFmt>& valuesIn, float momentValue, int interpolated = 1) {

	QList<numFmt> values = valuesIn;
	qSort(values);

	size_t lSize = values.size();
	double moment = -1;
	unsigned int momIdx = cvCeil(lSize*momentValue);
	unsigned int idx = 1;

	// find the statistical moment
	for (auto val : values) {

		// skip
		if (idx < momIdx) {
			idx++;
			continue;
		}
		if (lSize % 2 == 0 && momIdx < lSize && interpolated == 1)
			// compute mean between this and the next element
			moment = ((double)val + values[idx+1])*0.5;
		else
			moment = (double)val;
		break;
	}

	return moment;
}

template<typename sFmt, typename mFmt>
static void mulMaskIntern(cv::Mat src, const cv::Mat mask) {

	sFmt* srcPtr = (sFmt*)src.data;
	const mFmt* mPtr = (mFmt*)mask.data;

	int srcStep = (int)src.step / sizeof(sFmt);
	int mStep = (int)mask.step / sizeof(mFmt);

	for (int rIdx = 0; rIdx < src.rows; rIdx++, srcPtr += srcStep, mPtr += mStep) {

		for (int cIdx = 0; cIdx < src.cols; cIdx++) {

			if (mPtr[cIdx] == 0) srcPtr[cIdx] = 0;
		}
	}
};

template<typename sFmt>
static void setBorderConstIntern(cv::Mat src, sFmt val) {

	sFmt* srcPtr = (sFmt*)src.data;
	sFmt* srcPtr2 = (sFmt*)src.ptr<sFmt*>(src.rows - 1);
	int srcStep = (int)src.step / sizeof(sFmt);

	for (int cIdx = 0; cIdx < src.cols; cIdx++) {
		srcPtr[cIdx] = val;
		srcPtr2[cIdx] = val;
	}

	srcPtr = (sFmt*)src.data;
	for (int rIdx = 0; rIdx < src.rows; rIdx++, srcPtr += srcStep) {
		srcPtr[0] = val;
		srcPtr[src.cols - 1] = val;
	}
};

class DllCoreExport Algorithms {

public:
	enum MorphShape { SQUARE = 0, DISK };
	enum MorphBorder { BORDER_ZERO = 0, BORDER_FLIP };

	static Algorithms& instance();
	/// <summary>
	/// Dilates the image bwImg with a given structuring element.
	/// </summary>
	/// <param name="bwImg">The bwImg: a grayscale image CV_8U (or CV_32F [0 1] but slower).</param>
	/// <param name="seSize">The structuring element's size.</param>
	/// <param name="shape">The shape (either Square or Disk).</param>
	/// <param name="borderValue">The border value.</param>
	/// <returns>An dilated image (CV_8U or CV_32F).</returns>
	cv::Mat dilateImage(const cv::Mat& bwImg, int seSize, MorphShape shape = Algorithms::SQUARE, int borderValue = 0) const;

	/// <summary>
	/// Erodes the image bwimg with a given structuring element.
	/// </summary>
	/// <param name="bwImg">The bwimg: a grayscale image CV_8U (or CV_32F [0 1] but slower).</param>
	/// <param name="seSize">The structuring element's size.</param>
	/// <param name="shape">The shape (either Square or Disk).</param>
	/// <param name="borderValue">The border value.</param>
	/// <returns>An eroded image (CV_8U or CV_32F).</returns>
	cv::Mat erodeImage(const cv::Mat& bwImg, int seSize, MorphShape shape = Algorithms::SQUARE, int borderValue = 255) const;

	/// <summary>
	/// Creates the structuring element for morphological operations.
	/// </summary>
	/// <param name="seSize">Size of the structuring element.</param>
	/// <param name="shape">The shape (either Square or Disk).</param>
	/// <returns>A cvMat containing the structuring element (CV_8UC1).</returns>
	cv::Mat createStructuringElement(int seSize, int shape) const;

	/// <summary>
	/// Convolves a histogram symmetrically.
	/// Symmetric convolution means that the convolution
	/// is flipped around at the histograms borders. This is
	/// specifically useful for orientation histograms (since
	/// 0° corresponds to 360°
	/// </summary>
	/// <param name="hist">The histogram CV_32FC1.</param>
	/// <param name="kernel">The convolution kernel CV_32FC1.</param>
	/// <returns>The convolved Histogram CV_32FC1.</returns>
	cv::Mat convolveSymmetric(const cv::Mat& hist, const cv::Mat& kernel) const;

	/// <summary>
	/// Computes a 1D Gaussian filter kernel.
	/// The kernel's size is adjusted to the standard deviation.
	/// </summary>
	/// <param name="sigma">The standard deviation of the Gaussian.</param>
	/// <returns>The Gaussian kernel CV_32FC1</returns>
	cv::Mat get1DGauss(double sigma) const;

	/// <summary>
	/// Threshold an image using Otsu as threshold.
	/// </summary>
	/// <param name="srcImg">The source img CV_8UC1 or CV_8UC3.</param>
	/// <returns>A binary image CV_8UC1</returns>
	cv::Mat threshOtsu(const cv::Mat& srcImg) const;

	/// <summary>
	/// Convolves an integral image by means of box filters.
	/// This functions applies box filtering. It is specifically useful for the computation
	/// of image sums, mean filtering and standard deviation with big kernel sizes.
	/// </summary>
	/// <param name="src">The integral image CV_64FC1.</param>
	/// <param name="kernelSizeX">The box filter's size.</param>
	/// <param name="kernelSizeY">The box filter's size.</param>
	/// <param name="norm">If BORDER_ZERO an image sum is computed, if BORDER_FLIP a mean filtering is applied.</param>
	/// <returns>The convolved image CV_32FC1.</returns>
	cv::Mat convolveIntegralImage(const cv::Mat& src, const int kernelSizeX, const int kernelSizeY = 0, MorphBorder norm = BORDER_ZERO) const;

	/// <summary>
	/// Sets the border to a constant value (1 pixel width).
	/// </summary>
	/// <param name="src">The source image CV_32F or CV_8U.</param>
	/// <param name="val">The border value.</param>
	void setBorderConst(cv::Mat &src, float val = 0.0f) const;

	/// <summary>
	/// Computes robust statistical moments of an image.
	/// The quantiles of an image (or median) are computed.
	/// </summary>
	/// <param name="src">The source image CV_32FC1.</param>
	/// <param name="mask">The mask CV_32FC1 or CV_8UC1.</param>
	/// <param name="momentValue">The moment (e.g. 0.5 for median, 0.25 or 0.75 for quartiles).</param>
	/// <param name="maxSamples">The maximum number of samples (speed up).</param>
	/// <param name="area">The mask's area (speed up).</param>
	/// <returns>The statistical moment.</returns>
	float statMomentMat(const cv::Mat src, cv::Mat mask = cv::Mat(), float momentValue = 0.5f, int maxSamples = 10000, int area = -1) const;

	/// <summary>
	/// Inverts the img.
	/// </summary>
	/// <param name="srcImg">The source img CV_32FC1 [0 1] or CV_8UC1.</param>
	/// <param name="mask">The mask.</param>
	void invertImg(cv::Mat& srcImg, cv::Mat mask = cv::Mat());

	/// <summary>
	/// Applies a mask to the image.
	/// </summary>
	/// <param name="src">The source img CV_8U or CV_32F.</param>
	/// <param name="mask">The optional mask CV_8U or CV_32F.</param>
	void mulMask(cv::Mat& src, cv::Mat mask = cv::Mat());

	/// <summary>
	/// Prefilters an binary image according to the blob size.
	/// Should be done to remove small blobs and to reduce the runtime of cvFindContours.
	/// </summary>
	/// <param name="img">The source img CV_8UC1.</param>
	/// <param name="minArea">The blob size threshold in pixel.</param>
	/// <param name="maxArea">The maximum area.</param>
	/// <returns>A CV_8UC1 binary image with all blobs smaller than minArea removed.</returns>
	cv::Mat preFilterArea(const cv::Mat& img, int minArea, int maxArea = -1) const;

	/// <summary>
	/// Computes the histogram of an image.
	/// </summary>
	/// <param name="img">The source img CV_32FC1.</param>
	/// <param name="mask">The mask CV_8UC1 or CV_32FC1.</param>
	/// <returns>The histogram of the img as cv::mat CV_32FC1.</returns>
	cv::Mat computeHist(const cv::Mat img, const cv::Mat mask = cv::Mat()) const;

	/// <summary>
	/// Gets the Otsu threshold based on a certain histogram.
	/// </summary>
	/// <param name="hist">The histogram CV_32FC1.</param>
	/// <param name="otsuThresh">The otsu threshold - deprecated.</param>
	/// <returns>The computed threshold.</returns>
	double getThreshOtsu(const cv::Mat& hist, const double otsuThresh = 0) const;

	/// <summary>
	/// Computes the normalized angle within startIvl and endIvl.
	/// </summary>
	/// <param name="angle">The angle in rad.</param>
	/// <param name="startIvl">The intervals lower bound.</param>
	/// <param name="endIvl">The intervals upper bound.</param>
	/// <returns>The angle within [startIvl endIvl].</returns>
	float normAngleRad(float angle, float startIvl, float endIvl) const;

	/// <summary>
	/// Returns euclidean distance between two vectors
	/// </summary>
	/// <param name="p1">Vector p1.</param>
	/// <param name="p2">Vector p2.</param>
	/// <returns>The Euclidean distance.</returns>
	float euclideanDistance(const QPoint& p1, const QPoint& p2) const;

private:
	Algorithms();
	Algorithms(const Algorithms&);
};

};