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
#include "Image.h"
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

cv::Mat Algorithms::dilateImage(const cv::Mat& bwImg, int seSize, MorphShape shape, int borderValue) const {

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
	cv::dilate(imgU, dImg, se, cv::Point(-1, -1), 1, cv::BORDER_CONSTANT, borderValue);

	return dImg;
}

cv::Mat Algorithms::erodeImage(const cv::Mat& bwImg, int seSize, MorphShape shape, int borderValue) const {

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
	cv::erode(imgU, eImg, se, cv::Point(-1, -1), 1, cv::BORDER_CONSTANT, borderValue);

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
	//qDebug() << "convertedImg has " << srcGray.channels() << " channels";

	cv::Mat binImg;
	cv::threshold(srcGray, binImg, 0, 255, CV_THRESH_BINARY_INV | CV_THRESH_OTSU);

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

void Algorithms::setBorderConst(cv::Mat &src, float val) const {

	// do nothing if the mask is empty
	if (!src.empty()) {
		if (src.depth() == CV_32F)
			setBorderConstIntern<float>(src, val);
		else if (src.depth() == CV_8U)
			setBorderConstIntern<unsigned char>(src, (unsigned char)val);
		else {
			qWarning() << "The source image and the mask must be [CV_8U or CV_32F]";
		}
	}

}

float Algorithms::statMomentMat(const cv::Mat src, cv::Mat mask, float momentValue, int maxSamples, int area) const {

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

	return (float)rdf::statMoment(samples, momentValue);
}

void Algorithms::invertImg(cv::Mat& srcImg, cv::Mat mask) {
	
	if (srcImg.depth() == CV_32F) {

		int rows = srcImg.rows;
		int cols = srcImg.cols;
		for (int i = 0; i < rows; i++) {
			float *ptrImg = srcImg.ptr<float>(i);
			for (int j = 0; j < cols; j++) {
				ptrImg[j] = ptrImg[j] * -1.0f + 1.0f;
			}
		}
		Algorithms::mulMask(srcImg, mask);

	}
	else if (srcImg.depth() == CV_8U) {

		bitwise_not(srcImg, srcImg);
		Algorithms::mulMask(srcImg, mask);

	}
	else {
		qDebug() << "[DkIP::invertImg] the input image's depth must be 32F or 8U\n";
	}
}

void Algorithms::mulMask(cv::Mat& src, cv::Mat mask) {
	// do nothing if the mask is empty
	if (!mask.empty()) {

		if (src.depth() == CV_32F && mask.depth() == CV_32F)
			rdf::mulMaskIntern<float, float>(src, mask);
		else if (src.depth() == CV_32F && mask.depth() == CV_8U)
			rdf::mulMaskIntern<float, unsigned char>(src, mask);
		else if (src.depth() == CV_8U && mask.depth() == CV_32F)
			rdf::mulMaskIntern<unsigned char, float>(src, mask);
		else if (src.depth() == CV_8U && mask.depth() == CV_8U)
			rdf::mulMaskIntern<unsigned char, unsigned char>(src, mask);
		else if (src.depth() == CV_32S && mask.depth() == CV_8U)
			rdf::mulMaskIntern<int, unsigned char>(src, mask);
		else {
			qDebug() << "The source image and the mask must be [CV_8U or CV_32F]";
		}
	}
}

cv::Mat Algorithms::preFilterArea(const cv::Mat& img, int minArea, int maxArea) const {

	cv::Mat bwImgTmp = img.clone();
	cv::Mat filteredImage = img.clone();

	setBorderConst(bwImgTmp);
	int w = bwImgTmp.cols;
	int h = bwImgTmp.rows;
	unsigned char *si = bwImgTmp.data + w + 1;
	unsigned char *sie = si + w * h - 2 * w - 2;

	unsigned char **ppFGList = new unsigned char*[w * h];	// w*h/4 -> bug when images have large blobs [6.10.2011 markus]
	int fgListPos;
	int cur_fgListPos;
	unsigned char *pCurImgPos;

#define _PUSH_FG_LIST(a) (ppFGList[fgListPos++] = (a))
#define _PUSH_FG_LIST_CLR(a) (ppFGList[fgListPos++] = (*a = 0, a))
#define _POP_FG_LIST (ppFGList[cur_fgListPos++])

	while (si < sie)
	{
		if (*si)
		{
			fgListPos = 0;
			cur_fgListPos = 0;
			_PUSH_FG_LIST_CLR(si);

			// 3 2 1
			// 4 X 8
			// 5 6 7

			while (cur_fgListPos < fgListPos)
			{
				pCurImgPos = _POP_FG_LIST - 1 - w;

				if (*pCurImgPos)
					// 3
					_PUSH_FG_LIST_CLR(pCurImgPos);

				pCurImgPos++;
				if (*pCurImgPos)
					// 2
					_PUSH_FG_LIST_CLR(pCurImgPos);

				pCurImgPos++;
				if (*pCurImgPos)
					// 1
					_PUSH_FG_LIST_CLR(pCurImgPos);

				pCurImgPos += w;
				if (*pCurImgPos)
					// 8
					_PUSH_FG_LIST_CLR(pCurImgPos);

				pCurImgPos -= 2;
				if (*pCurImgPos)
					// 4
					_PUSH_FG_LIST_CLR(pCurImgPos);

				pCurImgPos += w;
				if (*pCurImgPos)
					// 5
					_PUSH_FG_LIST_CLR(pCurImgPos);

				pCurImgPos++;
				if (*pCurImgPos)
					// 6
					_PUSH_FG_LIST_CLR(pCurImgPos);

				pCurImgPos++;
				if (*pCurImgPos)
					// 7
					_PUSH_FG_LIST_CLR(pCurImgPos);
			}

			if (fgListPos <= minArea || (maxArea != -1 && fgListPos >= maxArea))
			{
				cur_fgListPos = 0;

				while (cur_fgListPos < fgListPos)
				{
					pCurImgPos = _POP_FG_LIST;

					*(filteredImage.data + ((size_t)pCurImgPos - (size_t)bwImgTmp.data)) = 0;
				}
			}
		}

		si++;
	}

	delete[] ppFGList;

	return filteredImage;

}

cv::Mat Algorithms::computeHist(const cv::Mat img, const cv::Mat mask) const {
	if (img.channels() > 1) {
		qDebug() << "the image needs to have 1 channel";
		return cv::Mat();
	}

	if (!mask.empty() && mask.channels() > 1) {
		qDebug() << "the mask needs to have 1 channel";
		return cv::Mat();
	}

	if (img.type() != CV_32FC1) {
		qDebug() << "the image needs to be CV_32FC1";
		return cv::Mat();
	}

	if (!mask.empty() && !(mask.type() == CV_32FC1 || mask.type() == CV_8UC1)) {
		qDebug() << "the mask needs to be CV_32FC1";
		return cv::Mat();
	}

	// compute gradient magnitude
	int cols = img.cols;
	int rows = img.rows;

	// speed up for accessing elements
	if (img.isContinuous()) {
		cols *= rows;
		rows = 1;
	}

	cv::Mat hist = cv::Mat(1, 256, CV_32FC1);
	hist.setTo(0);

	for (int rIdx = 0; rIdx < rows; rIdx++) {

		const float* imgPtr = img.ptr<float>(rIdx);
		const float* maskPtr32F = 0;
		const unsigned char* maskPtr8U = 0;

		float* histPtr = hist.ptr<float>();

		if (!mask.empty() && mask.depth() == CV_32F) {
			maskPtr32F = mask.ptr<float>(rIdx);
		}
		else if (!mask.empty() && mask.depth() == CV_8U) {
			maskPtr8U = mask.ptr<unsigned char>(rIdx);
		}

		for (int cIdx = 0; cIdx < cols; cIdx++) {

			if (mask.empty() ||
				(mask.depth() == CV_32F && maskPtr32F[cIdx] > 0) ||
				(mask.depth() == CV_8U  && maskPtr8U[cIdx]	> 0)) {
				int hIdx = cvFloor(imgPtr[cIdx] * 255);
				//printf("%.3f, %i\n", imgPtr[cIdx], hIdx);
				if (hIdx >= 0 && hIdx < 256)	// -> bug in normalize!
					histPtr[hIdx] ++;
			}
		}
	}

	return hist;
}

double Algorithms::getThreshOtsu(const cv::Mat& hist, const double otsuThresh) const {
	if (hist.channels() > 1) {
		qDebug() << "the histogram needs to have 1 channel";
		return -1.0;
	}

	if (hist.type() != CV_32FC1) {
		qDebug() << "the histogram needs to be 32FC1";
		return -1.0;
	}

	double max_val = 0;

	int i, count;
	const float* h;
	double sum = 0, mu = 0;
	bool uniform = false;
	double low = 0, high = 0, delta = 0;
	double mu1 = 0, q1 = 0;
	double max_sigma = 0;

	count = hist.cols;
	h = hist.ptr<float>();

	low = 0;
	high = count;

	delta = (high - low) / count;
	low += delta*0.5;
	uniform = true;

	for (i = 0; i < count; i++) {
		sum += h[i];
		mu += (i*delta + low)*h[i];
	}

	sum = fabs(sum) > FLT_EPSILON ? 1. / sum : 0;
	mu *= sum;
	mu1 = 0;
	q1 = 0;

	for (i = 0; i < count; i++) {
		double p_i, q2, mu2, val_i, sigma;
		p_i = h[i] * sum;
		mu1 *= q1;
		q1 += p_i;
		q2 = 1. - q1;

		if (MIN(q1, q2) < FLT_EPSILON || MAX(q1, q2) > 1. - FLT_EPSILON)
			continue;

		val_i = i*delta + low;

		mu1 = (mu1 + val_i*p_i) / q1;
		mu2 = (mu - q1*mu1) / q2;
		sigma = q1*q2*(mu1 - mu2)*(mu1 - mu2);
		if (sigma > max_sigma) {
			max_sigma = sigma;
			max_val = val_i;
		}
	}

	// shift threshold if the contrast is low (no over segmentation)
	double max_val_shift = (max_val - 0.2 > 0) ? max_val - 0.2 : 0;

	return (max_sigma >= otsuThresh) ? max_val : max_val_shift; // 0.0007 (for textRectangles)
																//return max_val;
}


float Algorithms::normAngleRad(float angle, float startIvl, float endIvl) const {
	// this could be a bottleneck
	if (abs(angle) > 1000)
		return angle;

	while (angle <= startIvl)
		angle += endIvl - startIvl;
	while (angle > endIvl)
		angle -= endIvl - startIvl;

	return angle;
}

float Algorithms::euclideanDistance(const QPoint& p1, const QPoint& p2) const {

	return (float)sqrt((p1.x() - p2.x())*(p1.x() - p2.x()) + (p1.y() - p2.y())*(p1.y() - p2.y()));

}


cv::Mat Algorithms::estimateMask(const cv::Mat& src, bool preFilter) const {

	cv::Mat srcGray = src.clone();
	cv::Mat mask;

	if (src.depth() != CV_8U && src.depth() != CV_32F) return cv::Mat();
	if (srcGray.channels() != 1) cv::cvtColor(srcGray, srcGray, CV_RGB2GRAY);

	if (srcGray.depth() == CV_32F) {
		srcGray.convertTo(mask, CV_8U, 255);
	}
	else {
		mask = srcGray.clone();
		srcGray.convertTo(srcGray, CV_32F, 1.0f / 255.0f);
	}
	//now we have a CV_32F srcGray image and a CV_8U mask

	mask = Algorithms::instance().threshOtsu(mask);
	if (preFilter)
		mask = Algorithms::instance().preFilterArea(mask, 10);
	//cv::Mat hist = Algorithms::instance().computeHist(srcGray);
	//double thresh = Algorithms::instance().getThreshOtsu(hist);

	// check the ratio of the border pixels if there is foreground
	int borderPixelCount = 0;
	for (int y = 1; y < mask.rows - 1; y++) {
		const unsigned char* curRow = mask.ptr<unsigned char>(y);
		borderPixelCount += curRow[0] + curRow[mask.cols - 1];
	}
	const unsigned char* firstRow = mask.ptr<unsigned char>(0);
	const unsigned char* lastRow = mask.ptr<unsigned char>(mask.rows - 1);
	for (int x = 0; x < mask.cols; x++) {
		borderPixelCount += firstRow[x] + lastRow[x];
	}

	if (((float)borderPixelCount / 255) / ((mask.rows * 2 + 2 * mask.cols) - 4) > 0.50) {
		qWarning() << "using empty mask in estimateMask";
		mask.setTo(255);
		return mask;
	}

	cv::Mat zeroBorderMask = cv::Mat(mask.rows + 2, mask.cols + 2, mask.type()); // make a zero border image because findContours uses zero borders
	zeroBorderMask.setTo(0);
	cv::Mat smallZeroBorderMask = zeroBorderMask(cv::Rect(1, 1, mask.cols, mask.rows)); // copyTo needs reference
	mask.copyTo(smallZeroBorderMask);


	rdf::Blobs binBlobs;
	binBlobs.setImage(zeroBorderMask);
	binBlobs.compute();

	rdf::Blob bBlob = rdf::BlobManager::instance().getBiggestBlob(binBlobs);

	if (!bBlob.isEmpty()) {
		// calculate mean and standard deviation of the gray values at the border of the image
		cv::Mat border(srcGray.rows, srcGray.cols, CV_8UC1);
		border.setTo(255);
		border(cv::Rect(1, 1, srcGray.cols - 2, srcGray.rows - 2)).setTo(0);
		border = border - mask; // set pixels at the border which are detected by otsu to 0
		cv::Scalar meanBorder;
		cv::Scalar stdDevBorder;
		meanStdDev(srcGray, meanBorder, stdDevBorder, border);

		// if the background is perfectly uniform, the std might get < 0.003
		if (stdDevBorder[0] < 0.051) stdDevBorder[0] = 0.051;

		zeroBorderMask.setTo(0);
		bBlob.drawBlob(zeroBorderMask, 0);
		

		for (int x = 0; x < srcGray.cols; ) {
			cv::Rect rect;
			cv::Point p1(x, 0);
			cv::Point p2(x, srcGray.rows - 1);
			if (srcGray.at<float>(p1) <= meanBorder[0])
				floodFill(srcGray, zeroBorderMask, cv::Point(x, 0), cv::Scalar(2000), &rect, cv::Scalar(5), stdDevBorder, cv::FLOODFILL_MASK_ONLY); // newVal (Scalar(2000)) is ignored when using MASK_ONLY
			if (srcGray.at<float>(p2) <= meanBorder[0])
				floodFill(srcGray, zeroBorderMask, cv::Point(x, srcGray.rows - 1), cv::Scalar(2000), &rect, cv::Scalar(5), stdDevBorder, cv::FLOODFILL_MASK_ONLY); // newVal (Scalar(2000)) is ignored when using MASK_ONLY
			if (srcGray.cols / 20 > 0)
				x += srcGray.cols / 20; // 5 seed points for the first and last row
			else
				x++;
		}
		for (int y = 0; y < srcGray.rows; ) {
			cv::Rect rect;
			cv::Point p1(0, y);
			cv::Point p2(srcGray.cols - 1, y);
			if (srcGray.at<float>(p1) <= meanBorder[0])
				floodFill(srcGray, zeroBorderMask, cv::Point(0, y), cv::Scalar(2000), &rect, cv::Scalar(5), stdDevBorder, cv::FLOODFILL_MASK_ONLY); // newVal (Scalar(2000)) is ignored when using MASK_ONLY
			if (srcGray.at<float>(p2) <= meanBorder[0])
				floodFill(srcGray, zeroBorderMask, cv::Point(srcGray.cols - 1, y), cv::Scalar(2000), &rect, cv::Scalar(5), stdDevBorder, cv::FLOODFILL_MASK_ONLY); // newVal (Scalar(2000)) is ignored when using MASK_ONLY

			if (srcGray.rows / 20 > 0)
				y += srcGray.rows / 20; // 5 seed points for the first and last col
			else
				y++;
		}

		zeroBorderMask = zeroBorderMask != 1;
		mask = zeroBorderMask(cv::Rect(1, 1, mask.cols, mask.rows));

		rdf::Blobs binBlobs2;
		binBlobs2.setImage(zeroBorderMask);
		binBlobs2.compute();

		binBlobs2.setBlobs(rdf::BlobManager::instance().filterArea(mask.rows*mask.cols / 8, binBlobs2));
		zeroBorderMask = rdf::BlobManager::instance().drawBlobs(binBlobs2, cv::Scalar(255));

	} else {
		qWarning() << "The thresholded image seems to be empty -> no mask created";
		mask = 255;
	}

	return mask;
}

}