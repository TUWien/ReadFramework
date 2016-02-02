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

#include "Algorithms.h"

#pragma warning(push, 0)	// no warnings from includes
#include <QApplication>
#include <QDebug>

#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#pragma warning(pop)

namespace rdf {

// Algorithms --------------------------------------------------------------------
Algorithms::Algorithms() {
}

cv::Mat Algorithms::dilateImage(const cv::Mat &bwImg, int seSize, int shape = Algorithms::SQUARE) {

	// nothing to do in here
	if (seSize < 3) return bwImg.clone();
	if (seSize % 2 == 0) seSize += 1;

	//if (bwImg.channels() > 1)
	//	throw DkMatException("Gray-scale image is required.", __LINE__, __FILE__);

	cv::Mat dImg;
	// TODO: bug if 32F & DK_DISK
	// dilate is much faster (and correct) with CV_8U
	cv::Mat imgU = bwImg;
	if (bwImg.depth() != CV_8U)
		bwImg.convertTo(imgU, CV_8U, 255);

	cv::Mat se = createStructuringElement(seSize, shape);
	cv::dilate(imgU, dImg, se, cv::Point(-1, -1), 1, cv::BORDER_CONSTANT, 0);

	return dImg;
}

cv::Mat Algorithms::erodeImage(const cv::Mat &bwImg, int seSize, int shape = Algorithms::SQUARE) {

	// nothing to do in here
	if (seSize < 3) return bwImg.clone();
	if (seSize % 2 == 0) seSize += 1;

	//if (bwImg.channels() > 1)
	//	throw DkMatException("Gray-scale image is required.", __LINE__, __FILE__);

	cv::Mat eImg;
	cv::Mat imgU = bwImg;

	// TODO: bug if 32F & DK_DISK
	// erode is much faster (and correct) with CV_8U
	if (bwImg.depth() != CV_8U)
		bwImg.convertTo(imgU, CV_8U, 255);

	cv::Mat se = createStructuringElement(seSize, shape);
	erode(imgU, eImg, se, cv::Point(-1, -1), 1, cv::BORDER_CONSTANT, 255);

	imgU = eImg;
	if (bwImg.depth() != CV_8U)
		imgU.convertTo(eImg, bwImg.depth(), 1.0f / 255.0f);

	return eImg;
}

cv::Mat Algorithms::createStructuringElement(int seSize, int shape) {

	cv::Mat se = cv::Mat(seSize, seSize, CV_8U);

	switch (shape) {

	case Algorithms::SQUARE:
		se = 1;
		break;
	case Algorithms::DISK:

		se.setTo(0);

		int c = seSize << 1;   //DkMath::halfInt(seSize);	// radius
		int r = c*c;						// center

		for (int rIdx = 0; rIdx < se.rows; rIdx++) {

			unsigned char* sePtr = se.ptr<unsigned char>(rIdx);

			for (int cIdx = 0; cIdx < se.cols; cIdx++) {

				// squared pixel distance to center
				int dist = (rIdx - c)*(rIdx - c) + (cIdx - c)*(cIdx - c);

				//printf("distance: %i, radius: %i\n", dist, r);
				if (dist < r)
					sePtr[cIdx] = 1;
			}
		}
		break;
	}

	return se;

}

Algorithms& Algorithms::instance() {

	static QSharedPointer<Algorithms> inst;
	if (!inst)
		inst = QSharedPointer<Algorithms>(new Algorithms());
	return *inst;
}


}