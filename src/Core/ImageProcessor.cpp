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

}