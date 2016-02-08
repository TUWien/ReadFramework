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


class DllCoreExport Algorithms {

public:
	enum MorphShape { SQUARE = 0, DISK };
	enum MorphBorder { BORDER_ZERO = 0, BORDER_FLIP };

	static Algorithms& instance();
	cv::Mat dilateImage(const cv::Mat& bwImg, int seSize, MorphShape shape = Algorithms::SQUARE, int borderValue = 0) const;
 	cv::Mat erodeImage(const cv::Mat& bwImg, int seSize, MorphShape shape = Algorithms::SQUARE, int borderValue = 255) const;
	cv::Mat createStructuringElement(int seSize, int shape) const;
	cv::Mat convolveSymmetric(const cv::Mat& hist, const cv::Mat& kernel) const;
	cv::Mat get1DGauss(double sigma) const;
	cv::Mat threshOtsu(const cv::Mat& srcImg) const;
	cv::Mat convolveIntegralImage(const cv::Mat& src, const int kernelSizeX, const int kernelSizeY = 0, MorphBorder norm = BORDER_ZERO) const;
	float statMomentMat(const cv::Mat src, cv::Mat mask = cv::Mat(), float momentValue = 0.5f, int maxSamples = 10000, int area = -1) const;
	void invertImg(cv::Mat& srcImg, cv::Mat mask = cv::Mat());
	void mulMask(cv::Mat& src, cv::Mat mask = cv::Mat());
	cv::Mat computeHist(const cv::Mat img, const cv::Mat mask = cv::Mat()) const;
	double getThreshOtsu(const cv::Mat& hist, const double otsuThresh = 0) const;

private:
	Algorithms();
	Algorithms(const Algorithms&);
};

};