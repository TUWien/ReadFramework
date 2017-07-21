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
#include <QObject>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#pragma warning(pop)

#ifndef DllCoreExport
#ifdef DLL_CORE_EXPORT
#define DllCoreExport Q_DECL_EXPORT
#else
#define DllCoreExport Q_DECL_IMPORT
#endif
#endif

// Qt defines

namespace cv {
	// compare operator for keypoints
	DllCoreExport bool operator==(const cv::KeyPoint& kpl, const cv::KeyPoint& kpr);
}

namespace rdf {

// read defines
class DllCoreExport IP {	// basically a namespace for now

public:
	enum MorphShape { morph_square = 0, morph_disk };
	enum MorphBorder { border_zero = 0, border_flip };

	// image processing
	static cv::Mat createStructuringElement(int seSize, int shape);
	static cv::Mat dilateImage(const cv::Mat& bwImg, int seSize, MorphShape shape = IP::morph_square, int borderValue = 0);
	static cv::Mat erodeImage(const cv::Mat& bwImg, int seSize, MorphShape shape = IP::morph_square, int borderValue = 255);

	static cv::Mat convolveSymmetric(const cv::Mat& hist, const cv::Mat& kernel);
	static cv::Mat convolveIntegralImage(const cv::Mat& src, const int kernelSizeX, const int kernelSizeY = 0, MorphBorder norm = IP::border_zero);
	static cv::Mat get1DGauss(double sigma, int kernelsize = -1);

	static cv::Mat threshOtsu(const cv::Mat& srcImg, int thType = CV_THRESH_BINARY_INV);
	static double getThreshOtsu(const cv::Mat& hist, const double otsuThresh = 0);

	static void setBorderConst(cv::Mat &src, float val = 0.0f);
	static void invertImg(cv::Mat& srcImg, cv::Mat mask = cv::Mat());
	static cv::Mat preFilterArea(const cv::Mat& img, int minArea, int maxArea = -1);
	static cv::Mat computeHist(const cv::Mat img, const cv::Mat mask = cv::Mat());

	static cv::Mat estimateMask(const cv::Mat& src, bool preFilter = true);
	static void mulMask(cv::Mat& src, cv::Mat mask = cv::Mat());
	static QPointF calcRotationSize(double angleRad, const QPointF& srcSize);
	static cv::Mat rotateImage(const cv::Mat& src, double angleRad, int interpolation = cv::INTER_CUBIC, cv::Scalar borderValue = cv::Scalar(0));

	static cv::Mat invert(const cv::Mat& src);
	static cv::Mat grayscale(const cv::Mat& src);

	static cv::Mat computeHist(const cv::Mat& data, int width, int numElements = -1, double* maxBin = 0);
	static void draw(const std::vector<cv::Point>& pts, cv::Mat& img, unsigned char val = 255);
	
	static double statMomentMat(const cv::Mat& src, const cv::Mat& mask = cv::Mat(), double momentValue = 0.5, int maxSamples = 10000, int area = -1);
	static QColor statMomentColor(const cv::Mat& src, const cv::Mat& mask = cv::Mat(), double momentValue = 0.5);

	static void normalize(cv::Mat& src);

private:
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
	}

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
	}
};

}
