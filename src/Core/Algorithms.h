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
#include "Blobs.h"

#ifndef DllCoreExport
#ifdef DLL_CORE_EXPORT
#define DllCoreExport Q_DECL_EXPORT
#else
#define DllCoreExport Q_DECL_IMPORT
#endif
#endif

// Qt defines
class QSettings;

namespace rdf {

// read defines

/// <summary>
/// Contains basic algorithms to manipulate images.
/// </summary>
class DllCoreExport Algorithms {

public:
	enum MorphShape { SQUARE = 0, DISK };
	enum MorphBorder { BORDER_ZERO = 0, BORDER_FLIP };

	// image processing
	static cv::Mat dilateImage(const cv::Mat& bwImg, int seSize, MorphShape shape = Algorithms::SQUARE, int borderValue = 0);
	static cv::Mat erodeImage(const cv::Mat& bwImg, int seSize, MorphShape shape = Algorithms::SQUARE, int borderValue = 255);
	static cv::Mat createStructuringElement(int seSize, int shape);
	static cv::Mat convolveSymmetric(const cv::Mat& hist, const cv::Mat& kernel);
	static cv::Mat get1DGauss(double sigma);
	static cv::Mat threshOtsu(const cv::Mat& srcImg, int thType = CV_THRESH_BINARY_INV);
	static cv::Mat convolveIntegralImage(const cv::Mat& src, const int kernelSizeX, const int kernelSizeY = 0, MorphBorder norm = BORDER_ZERO);
	static void setBorderConst(cv::Mat &src, float val = 0.0f);
	static void invertImg(cv::Mat& srcImg, cv::Mat mask = cv::Mat());
	static void mulMask(cv::Mat& src, cv::Mat mask = cv::Mat());
	static cv::Mat preFilterArea(const cv::Mat& img, int minArea, int maxArea = -1);
	static cv::Mat computeHist(const cv::Mat img, const cv::Mat mask = cv::Mat());
	static double getThreshOtsu(const cv::Mat& hist, const double otsuThresh = 0);
	static double normAngleRad(double angle, double startIvl = 0.0, double endIvl = 2*CV_PI);
	static double angleDist(double angle1, double angle2, double maxAngle = 2*CV_PI);
	static cv::Mat estimateMask(const cv::Mat& src, bool preFilter=true);
	static cv::Mat rotateImage(const cv::Mat& src, double angleRad, int interpolation = cv::INTER_CUBIC, cv::Scalar borderValue = cv::Scalar(0));
	static double statMomentMat(const cv::Mat src, cv::Mat mask = cv::Mat(), float momentValue = 0.5f, int maxSamples = 10000, int area = -1);
	//Computes the natural logarithm of the absolute value of	the gamma function of x using the Lanczos approximation.
	static double log_gamma_lanczos(double x);


	// convenience functions
	static QPointF calcRotationSize(double angleRad, const QPointF& srcSize);
	static double min(const QVector<double>& vec);
	static double max(const QVector<double>& vec);

	// template functions --------------------------------------------------------------------
	/// <summary>
	/// Computes robust statistical moments (quantiles).
	/// </summary>
	/// <param name="valuesIn">The statistical set (samples).</param>
	/// <param name="momentValue">The statistical moment value (0.5 = median, 0.25 and 0.75 = quartiles).</param>
	/// <param name="interpolated">A flag if the value should be interpolated if the length of the list is even.</param>
	/// <returns>The statistical moment.</returns>
	template <typename numFmt>
	static double statMoment(const QList<numFmt>& valuesIn, double momentValue, int interpolated = 1) {

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

};

/// <summary>
/// Implements robust line fitting algorithms.
/// </summary>
class DllCoreExport LineFitting {

public:
	LineFitting(const QVector<Vector2D>& pts);

	Line fitLineLMS() const;

protected:
	// parameters:
	int mNumSets = 1000;	// # random sets generated
	int mSetSize = 2;		// if 2, lines are directly returned
	double mEps = 0.1;		// if LMS is smaller than that, we break
	double mMinLength = 2;	// minimum line length

	QVector<Vector2D> mPts;

	void sample(const QVector<Vector2D>& pts, QVector<Vector2D>& set, int setSize = 2) const;
	double medianResiduals(const QVector<Vector2D>& pts, const Line& line) const;
};

};