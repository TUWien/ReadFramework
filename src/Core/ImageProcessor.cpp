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

#include "ImageProcessor.h"
#include "Algorithms.h"
#include "Image.h"

#pragma warning(push, 0)	// no warnings from includes
#include <QColor>
#include <QDebug>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#pragma warning(pop)

namespace cv {
	bool operator==(const cv::KeyPoint& kpl, const cv::KeyPoint& kpr) {
		return kpl.pt == kpr.pt && kpl.size == kpr.size && kpl.angle == kpr.angle;
	}
}

namespace rdf {

cv::Mat IP::invert(const cv::Mat & src) {
	
	assert(src.depth() == CV_8U || src.depth() == CV_32F || src.depth() == CV_64F);

	double sMin, sMax;
	cv::minMaxLoc(src, &sMin, &sMax);

	cv::Mat dst = cv::Scalar::all(sMax) - src + cv::Scalar::all(sMin);

	return dst;
}

/// <summary>
/// Returns the Luminance channel of Luv which is better than the RGB2Gray.
/// </summary>
/// <param name="src">The source.</param>
/// <returns></returns>
cv::Mat IP::grayscale(const cv::Mat & src) {
	
	// is already grayscale?
	if (src.channels() == 1)
		return src;

	assert(src.channels() == 3 || src.channels() == 4);

	cv::Mat dst;
	cv::cvtColor(src, dst, CV_RGB2Luv);
	
	std::vector<cv::Mat> channels;
	cv::split(dst, channels);
	
	assert(!channels.empty());
	return channels[0];
}

/// <summary>
/// Computes the histogram of data.
/// Data can be any type (it will be reduced to CV_32F).
/// </summary>
/// <param name="data">The data.</param>
/// <param name="width">The length of the resulting histogram.</param>
/// <param name="numElements">The number elements if data has more elements, it will be downsampled accordingly.</param>
/// <param name="maxBin">The maximal bin of the resulting histogram.</param>
/// <returns>A 1xN CV_32F. Each element represents a bin computed from data.</returns>
cv::Mat IP::computeHist(const cv::Mat & data, int width, int numElements, double * maxBin) {
	
	cv::Mat reducedData = data;

	// convert data
	if (data.depth() != CV_32F)
		reducedData.convertTo(reducedData, CV_32F);

	reducedData = reducedData.reshape(1, 1);

	// downsample data
	if (numElements > 0 && numElements < data.cols)
		cv::resize(data, reducedData, cv::Size(numElements, data.rows), 0.0, 0.0, CV_INTER_NN);

	double minV, maxV;
	cv::minMaxLoc(reducedData, &minV, &maxV);

	float hranges[] = {(float)minV, (float)maxV};
	const float* phranges = hranges;

	cv::Mat hist;
	cv::calcHist(&reducedData, 1, 0, cv::Mat(), hist, 1, &width, &phranges);

	if (maxBin)
		cv::minMaxLoc(hist, 0, maxBin);

	hist = hist.t();
	cv::normalize(hist, hist, 0, 1.0f, cv::NORM_MINMAX);

	return hist;
}

void IP::draw(const std::vector<cv::Point>& pts, cv::Mat & img, unsigned char val) {

	// we support 8UC1 for now
	assert(img.depth() == CV_8UC1);

	for (const cv::Point& p : pts) {

		assert(p.x >= 0 && p.x < img.cols);
		assert(p.y >= 0 && p.y < img.rows);

		img.at<unsigned char>(p) = val;
	}
}

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
double IP::statMomentMat(const cv::Mat& src, const cv::Mat& mask, double momentValue, int maxSamples, int area) {

	// check input
	if (src.type() != CV_32FC1) {
		qWarning() << "Mat must be CV_32FC1";
		return -1;
	}

	if (!mask.empty()) {
		if (src.rows != mask.rows || src.cols != mask.cols) {
			qWarning() << "Matrix dimension mismatch";
			return -1;
		}

		if (mask.depth() != CV_32F && mask.depth() != CV_8U) {
			qWarning() << "The mask obtained is neither of type CV_32F nor CV_8U";
			return -1;
		}
	}

	// init output list
	QList<float> samples = QList<float>();

	// assign the step size
	if (mask.empty()) {
		int step = cvRound((src.cols*src.rows) / (float)maxSamples);
		if (step <= 0) step = 1;

		for (int rIdx = 0; rIdx < src.rows; rIdx += step) {

			const float* srcPtr = src.ptr<float>(rIdx);

			for (int cIdx = 0; cIdx < src.cols; cIdx += step) {
				samples.push_back(srcPtr[cIdx]);
			}
		}
	}
	else {

		if (area == -1)
			area = countNonZero(mask);

		int step = cvRound((float)area / (float)maxSamples);
		int cStep = 0;

		const void *maskPtr;

		if (step <= 0) step = 1;

		for (int rIdx = 0; rIdx < src.rows; rIdx++) {

			const float* srcPtr = src.ptr<float>(rIdx);
			if (mask.depth() == CV_32F)
				maskPtr = mask.ptr<float>(rIdx);
			else
				maskPtr = mask.ptr<uchar>(rIdx);
			//maskPtr = (mask.depth() == CV_32F) ? mask.ptr<float>(rIdx) : maskPtr = mask.ptr<uchar>(rIdx);

			for (int cIdx = 0; cIdx < src.cols; cIdx++) {

				// skip mask pixel
				if (mask.depth() == CV_32FC1 && ((float*)maskPtr)[cIdx] != 0.0f ||
					mask.depth() == CV_8U && ((uchar*)maskPtr)[cIdx] != 0) {

					if (cStep >= step) {
						samples.push_back(srcPtr[cIdx]);
						cStep = 0;
					}
					else
						cStep++;
				}
			}
		}
	}

	return Algorithms::statMoment(samples, momentValue);
}

/// <summary>
/// C.
/// </summary>
/// <param name="src">The source.</param>
/// <returns></returns>
QColor IP::statMomentColor(const cv::Mat & src, const cv::Mat& mask, double momentValue) {

	assert(src.type() == CV_8UC3 || src.type() == CV_8UC4);

	cv::Mat srcF;
	src.convertTo(srcF, CV_32F);

	std::vector<cv::Mat> channels;
	cv::split(srcF, channels);

	assert(channels.size() == 3 || channels.size() == 4);

	QColor col;
	col.setRed(qRound(statMomentMat(channels[0], mask, momentValue)));
	col.setGreen(qRound(statMomentMat(channels[1], mask, momentValue)));
	col.setBlue(qRound(statMomentMat(channels[2], mask, momentValue)));

	if (channels.size() == 4)
		col.setAlpha(qRound(statMomentMat(channels[3], mask, momentValue)));

	return col;
}

void IP::normalize(cv::Mat & src) {

	double maxV, minV;
	cv::minMaxLoc(src, &minV, &maxV);

	// normalize
	if (minV != maxV) {
		double dr = 1.0 / (maxV - minV);
		src.convertTo(src, CV_32F, dr, -dr * minV);
	}
	else {
		qWarning() << "[IP::normalize] I seem to get weird values here: ";
		src.convertTo(src, CV_32F);
		Image::imageInfo(src, "src");
	}
}

}