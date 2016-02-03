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


Algorithms& Algorithms::instance() {

	static QSharedPointer<Algorithms> inst;
	if (!inst)
		inst = QSharedPointer<Algorithms>(new Algorithms());
	return *inst;
}

cv::Mat Algorithms::dilateImage(const cv::Mat& bwImg, int seSize, MorphShape shape) const {

	// nothing to do in here
	if (seSize < 3) return bwImg.clone();
	if (seSize % 2 == 0) seSize += 1;

	if (bwImg.channels() > 1) {
		qWarning() << "Gray-scale image is required";
		return cv::Mat();
	}

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

cv::Mat Algorithms::erodeImage(const cv::Mat& bwImg, int seSize, MorphShape shape) const {

	// nothing to do in here
	if (seSize < 3) return bwImg.clone();
	if (seSize % 2 == 0) seSize += 1;

	if (bwImg.channels() > 1) {
		qWarning() << "Gray-scale image is required";
		return cv::Mat();
	}

	cv::Mat eImg;
	cv::Mat imgU = bwImg;

	// TODO: bug if 32F & DK_DISK
	// erode is much faster (and correct) with CV_8U
	if (bwImg.depth() != CV_8U)
		bwImg.convertTo(imgU, CV_8U, 255);

	cv::Mat se = createStructuringElement(seSize, shape);
	cv::erode(imgU, eImg, se, cv::Point(-1, -1), 1, cv::BORDER_CONSTANT, 255);

	imgU = eImg;
	if (bwImg.depth() != CV_8U)
		imgU.convertTo(eImg, bwImg.depth(), 1.0f / 255.0f);

	return eImg;
}

cv::Mat Algorithms::createStructuringElement(int seSize, int shape) const {

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

cv::Mat Algorithms::threshOtsu(const cv::Mat& srcImg) const {

	if (srcImg.depth() !=  CV_8U) {
		qWarning() << "8U is required";
		return cv::Mat();
	}

	cv::Mat srcGray = srcImg;
	if (srcImg.channels() != 1) cv::cvtColor(srcImg, srcGray, CV_RGB2GRAY);

	cv::Mat binImg;
	cv::threshold(srcGray, binImg, 0, 255, CV_THRESH_BINARY | CV_THRESH_OTSU);

	return binImg;

}


cv::Mat Algorithms::convolveSymmetric(const cv::Mat& hist, const cv::Mat& kernel) const {

	if (hist.channels() > 1) {
		qWarning() << "the histogram needs to have 1 channel";
		return cv::Mat();
	}

	if (kernel.channels() > 1) {
		qWarning() << "the kernel needs to have 1 channel";
		return cv::Mat();
	}

	if (hist.type() != CV_32FC1) {
		qWarning() << "the histogram needs to be CV_32FC1";
		return cv::Mat();
	}

	if (kernel.type() != CV_32FC1) {
		qWarning() << "the kernel needs to be CV_32FC1";
		return cv::Mat();
	}

	int hs = hist.rows * hist.cols;			// histogram size
	int ks = kernel.rows * kernel.cols;		// kernel size
	int halfKs = cvFloor(ks / 2);				// half kernel size
	int cs = hs + ks - 1;						// histogram size + kernel size

	cv::Mat symHistSmooth;
	cv::Mat symHist = cv::Mat(1, cs, CV_32F);

	const float* histPtr = hist.ptr<float>();
	float* symHistPtr = symHist.ptr<float>();

	for (int nIdx = 0, oIdx = -halfKs; nIdx < cs; nIdx++, oIdx++) {

		oIdx += hs;
		oIdx %= hs;

		symHistPtr[nIdx] = histPtr[oIdx];
	}

	filter2D(symHist, symHist, -1, kernel);
	symHistSmooth = symHist.colRange(halfKs, cs - halfKs);

	return symHistSmooth.clone();	// delete values outside the range

}

cv::Mat Algorithms::get1DGauss(double sigma) const {

	// correct -> checked with matlab reference
	int kernelsize = cvRound(cvCeil(sigma * 3) * 2) + 1;
	if (kernelsize < 3) kernelsize = 3;
	if ((kernelsize % 2) != 1) kernelsize += 1;

	cv::Mat gKernel = cv::Mat(1, kernelsize, CV_32F);
	float* kernelPtr = gKernel.ptr<float>();

	for (int idx = 0, x = -cvFloor(kernelsize / 2); idx < kernelsize; idx++, x++) {

		kernelPtr[idx] = (float)(exp(-(x*x) / (2 * sigma*sigma)));	// 1/(sqrt(2pi)*sigma) -> discrete normalization
	}


	if (sum(gKernel).val[0] == 0) {
		qWarning() << "The kernel sum is zero";
		return cv::Mat();
	} else
		gKernel *= 1.0f / sum(gKernel).val[0];

	return gKernel;
}

cv::Mat Algorithms::convolveIntegralImage(const cv::Mat& src, const int kernelSizeX, const int kernelSizeY, MorphBorder norm) const {

	if (src.channels() > 1) {
		qWarning() << "the image needs to have 1 channel";
		return cv::Mat();
	}

	if (src.type() != CV_64FC1) {
		qWarning() << "the image needs to be CV_64FC1";
		return cv::Mat();
	}

	int ksY = (kernelSizeY != 0) ? kernelSizeY : kernelSizeX;	// make squared kernel

	cv::Mat dst = cv::Mat(src.rows - 1, src.cols - 1, CV_32FC1);

	int halfKRows = (ksY < dst.rows) ? cvFloor((float)ksY*0.5) + 1 : cvFloor((float)(dst.rows - 1)*0.5) - 1;
	int halfKCols = (kernelSizeX < dst.cols) ? cvFloor((float)kernelSizeX*0.5) + 1 : cvFloor((float)(dst.cols - 1)*0.5) - 1;

	// if the image dimension (rows, cols) <= 2
	if (halfKRows <= 0 || halfKCols <= 0) {
		dst.setTo(0);
		return dst;
	}

	// pointer for all corners
	const double* llc = src.ptr<double>();
	const double* lrc = src.ptr<double>();
	const double* ulc = src.ptr<double>();
	const double* urc = src.ptr<double>();
	const double* origin = src.ptr<double>();
	const double* lastRow = src.ptr<double>();

	float *dstPtr = dst.ptr<float>();

	// initial positions
	lrc += halfKCols;
	ulc += halfKRows*src.cols;
	urc += halfKRows*src.cols + halfKCols;
	lastRow += (src.rows - 1)*src.cols;

	// area computation
	float rs = (float)halfKRows;
	float cs = (float)halfKCols;
	float area = rs*cs;

	for (int row = 0; row < dst.rows; row++) {

		for (int col = 0; col < dst.cols; col++) {

			// filter operation
			if (norm == BORDER_ZERO)
				*dstPtr = (float)(*urc - *ulc - *lrc + *llc);
			else if (norm == BORDER_FLIP) {
				*dstPtr = (float)((*urc - *ulc - *lrc + *llc) / area);
			}

			// do not change the left corners if we are near the left border
			if (col >= halfKCols - 1) {
				llc++; ulc++;
			}
			// but recompute the filter area near the border
			else if (norm == BORDER_FLIP) {
				cs++;
				area = rs*cs;
			}

			// do not change the right corners if we are near the right border
			if (col < dst.cols - halfKCols) {
				lrc++; urc++;
			}
			else if (norm == BORDER_FLIP && col != dst.cols - 1) {
				cs--;
				area = rs*cs;
			}

			dstPtr++;
		}

		// ok, flip to next row
		llc = ++lrc;
		ulc = ++urc;
		lrc += halfKCols;
		urc += halfKCols;

		if (row < halfKRows - 1) {
			llc = origin;
			lrc = origin + halfKCols;
			if (norm == BORDER_FLIP) {
				rs++;
				area = rs*cs;
			}
		}

		if (row >= dst.rows - halfKRows) {
			ulc = lastRow;
			urc = lastRow + halfKCols;
			if (norm == BORDER_FLIP) {
				rs--;
				area = rs*cs;
			}
		}
	}

	//normalize(dst, dst, 1, 0, NORM_MINMAX);
	return dst;
}


}