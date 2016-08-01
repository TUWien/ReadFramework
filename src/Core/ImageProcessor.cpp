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

#pragma warning(push, 0)	// no warnings from includes
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#pragma warning(pop)

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
	
	assert(src.channels() == 3);
	
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

void IP::draw(const std::vector<cv::Point>& pts, cv::Mat & img, cv::Scalar val) {

	// we support 8UC1 for now
	assert(img.depth() == CV_8UC1);

	unsigned char* iPtr = img.ptr<unsigned char>();

	for (const cv::Point& p : pts) {

		assert(p.x >= 0 && p.x < img.cols);
		assert(p.y >= 0 && p.y < img.rows);


	}
}

}