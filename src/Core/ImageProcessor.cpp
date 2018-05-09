/*******************************************************************************************************
 ReadFramework is the basis for modules developed at CVL/TU Wien for the EU project READ. 
  
 Copyright (C) 2016 Markus Diem <diem@cvl.tuwien.ac.at>
 Copyright (C) 2016 Stefan Fiel <fiel@cvl.tuwien.ac.at>
 Copyright (C) 2016 Florian Kleber <kleber@cvl.tuwien.ac.at>

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
 [1] http://www.cvl.tuwien.ac.at/cvl/
 [2] https://transkribus.eu/Transkribus/
 [3] https://github.com/TUWien/
 [4] http://nomacs.org
 *******************************************************************************************************/

#include "ImageProcessor.h"
#include "Algorithms.h"
#include "Image.h"

#include "Blobs.h"	// needed for estimate mask

#pragma warning(push, 0)	// no warnings from includes
#include <QColor>
#include <QDebug>
#pragma warning(pop)

namespace cv {
	bool operator==(const cv::KeyPoint& kpl, const cv::KeyPoint& kpr) {
		return kpl.pt == kpr.pt && kpl.size == kpr.size && kpl.angle == kpr.angle;
	}
}

namespace rdf {

cv::Mat IP::invert(const cv::Mat & src) {
	
	assert(src.depth() == CV_8U || src.depth() == CV_32S || src.depth() == CV_32F || src.depth() == CV_64F);

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
				if ((mask.depth() == CV_32FC1 && ((float*)maskPtr)[cIdx] != 0.0f) ||
					(mask.depth() == CV_8U && ((uchar*)maskPtr)[cIdx] != 0)) {

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

/// <summary>
/// Dilates the image bwImg with a given structuring element.
/// </summary>
/// <param name="bwImg">The bwImg: a grayscale image CV_8U (or CV_32F [0 1] but slower).</param>
/// <param name="seSize">The structuring element's size.</param>
/// <param name="shape">The shape (either Square or Disk).</param>
/// <param name="borderValue">The border value.</param>
/// <returns>An dilated image (CV_8U or CV_32F).</returns>
cv::Mat IP::dilateImage(const cv::Mat& bwImg, int seSize, MorphShape shape, int borderValue) {

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

/// <summary>
/// Erodes the image bwimg with a given structuring element.
/// </summary>
/// <param name="bwImg">The bwimg: a grayscale image CV_8U (or CV_32F [0 1] but slower).</param>
/// <param name="seSize">The structuring element's size.</param>
/// <param name="shape">The shape (either Square or Disk).</param>
/// <param name="borderValue">The border value.</param>
/// <returns>An eroded image (CV_8U or CV_32F).</returns>
cv::Mat IP::erodeImage(const cv::Mat& bwImg, int seSize, MorphShape shape, int borderValue) {

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

/// <summary>
/// Creates the structuring element for morphological operations.
/// </summary>
/// <param name="seSize">Size of the structuring element.</param>
/// <param name="shape">The shape (either Square or Disk).</param>
/// <returns>A cvMat containing the structuring element (CV_8UC1).</returns>
cv::Mat IP::createStructuringElement(int seSize, int shape) {

	cv::Mat se = cv::Mat(seSize, seSize, CV_8U);

	switch (shape) {

	case IP::morph_square:
		se = 1;
		break;
	case IP::morph_disk:
	{
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
	default:;
		// do nothing
	}

	return se;

}

/// <summary>
/// Threshold an image using Otsu as threshold.
/// </summary>
/// <param name="srcImg">The source img CV_8UC1 or CV_8UC3.</param>
/// <returns>A binary image CV_8UC1</returns>
cv::Mat IP::threshOtsu(const cv::Mat& srcImg, int thType) {

	if (srcImg.depth() != CV_8U) {
		qWarning() << "8U is required";
		return cv::Mat();
	}

	cv::Mat srcGray = srcImg;
	if (srcImg.channels() != 1) cv::cvtColor(srcImg, srcGray, CV_RGB2GRAY);
	//qDebug() << "convertedImg has " << srcGray.channels() << " channels";

	cv::Mat binImg;
	cv::threshold(srcGray, binImg, 0, 255, thType | CV_THRESH_OTSU);

	return binImg;

}

///// <summary>
///// Convolves a histogram symmetrically.
///// Symmetric convolution means that the convolution
///// is flipped around at the histograms borders. This is
///// specifically useful for orientation histograms (since
///// 0° corresponds to 360°
///// </summary>
///// <param name="hist">The histogram CV_32FC1.</param>
///// <param name="kernel">The convolution kernel CV_32FC1.</param>
///// <returns>The convolved Histogram CV_32FC1.</returns>
//cv::Mat IP::convolveSymmetric(const cv::Mat& hist, const cv::Mat& kernel) {
//
//	if (hist.channels() > 1) {
//		qWarning() << "the histogram needs to have 1 channel";
//		return cv::Mat();
//	}
//
//	if (kernel.channels() > 1) {
//		qWarning() << "the kernel needs to have 1 channel";
//		return cv::Mat();
//	}
//
//	if (hist.type() != CV_32FC1) {
//		qWarning() << "the histogram needs to be CV_32FC1";
//		return cv::Mat();
//	}
//
//	if (kernel.type() != CV_32FC1) {
//		qWarning() << "the kernel needs to be CV_32FC1";
//		return cv::Mat();
//	}
//
//	int hs = hist.rows * hist.cols;			// histogram size
//	int ks = kernel.rows * kernel.cols;		// kernel size
//	int halfKs = cvFloor(ks / 2);				// half kernel size
//	int cs = hs + ks - 1;						// histogram size + kernel size
//
//	cv::Mat symHistSmooth;
//	cv::Mat symHist = cv::Mat(1, cs, CV_32F);
//
//	const float* histPtr = hist.ptr<float>();
//	float* symHistPtr = symHist.ptr<float>();
//
//	for (int nIdx = 0, oIdx = -halfKs; nIdx < cs; nIdx++, oIdx++) {
//
//		oIdx += hs;
//		oIdx %= hs;
//
//		symHistPtr[nIdx] = histPtr[oIdx];
//	}
//
//	filter2D(symHist, symHist, -1, kernel);
//	symHistSmooth = symHist.colRange(halfKs, cs - halfKs);
//
//	return symHistSmooth.clone();	// delete values outside the range
//
//}

/// <summary>
/// Computes a 1D Gaussian filter kernel.
/// The kernel's size is adjusted to the standard deviation.
/// </summary>
/// <param name="sigma">The standard deviation of the Gaussian.</param>
/// <returns>The Gaussian kernel CV_32FC1</returns>
cv::Mat IP::get1DGauss(double sigma, int kernelsize) {

	// correct -> checked with matlab reference
	if (kernelsize == -1) {
		kernelsize = cvRound(cvCeil(sigma * 3) * 2) + 1;
		if (kernelsize < 3) kernelsize = 3;
		if ((kernelsize % 2) != 1) kernelsize += 1;
	}

	cv::Mat gKernel = cv::Mat(1, kernelsize, CV_32F);
	float* kernelPtr = gKernel.ptr<float>();

	for (int idx = 0, x = -cvFloor(kernelsize / 2); idx < kernelsize; idx++, x++) {

		kernelPtr[idx] = (float)(exp(-(x*x) / (2 * sigma*sigma)));	// 1/(sqrt(2pi)*sigma) -> discrete normalization
	}

	if (sum(gKernel).val[0] == 0) {
		qWarning() << "The kernel sum is zero";
		return cv::Mat();
	}
	else
		gKernel *= 1.0f / sum(gKernel).val[0];

	return gKernel;
}

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
cv::Mat IP::convolveIntegralImage(const cv::Mat& src, const int kernelSizeX, const int kernelSizeY, MorphBorder norm) {

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
			if (norm == IP::border_zero)
				*dstPtr = (float)(*urc - *ulc - *lrc + *llc);
			else if (norm == IP::border_flip) {
				*dstPtr = (float)((*urc - *ulc - *lrc + *llc) / area);
			}

			// do not change the left corners if we are near the left border
			if (col >= halfKCols - 1) {
				llc++; ulc++;
			}
			// but recompute the filter area near the border
			else if (norm == IP::border_flip) {
				cs++;
				area = rs*cs;
			}

			// do not change the right corners if we are near the right border
			if (col < dst.cols - halfKCols) {
				lrc++; urc++;
			}
			else if (norm == IP::border_flip && col != dst.cols - 1) {
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
			if (norm == IP::border_flip) {
				rs++;
				area = rs*cs;
			}
		}

		if (row >= dst.rows - halfKRows) {
			ulc = lastRow;
			urc = lastRow + halfKCols;
			if (norm == IP::border_flip) {
				rs--;
				area = rs*cs;
			}
		}
	}

	//normalize(dst, dst, 1, 0, NORM_MINMAX);
	return dst;
}

/// <summary>
/// Sets the border to a constant value (1 pixel width).
/// </summary>
/// <param name="src">The source image CV_32F or CV_8U.</param>
/// <param name="val">The border value.</param>
void IP::setBorderConst(cv::Mat &src, float val) {

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

/// <summary>
/// Inverts the img.
/// </summary>
/// <param name="srcImg">The source img CV_32FC1 [0 1] or CV_8UC1.</param>
/// <param name="mask">The mask.</param>
void IP::invertImg(cv::Mat& srcImg, cv::Mat mask) {

	if (srcImg.depth() == CV_32F) {

		int rows = srcImg.rows;
		int cols = srcImg.cols;
		for (int i = 0; i < rows; i++) {
			float *ptrImg = srcImg.ptr<float>(i);
			for (int j = 0; j < cols; j++) {
				ptrImg[j] = ptrImg[j] * -1.0f + 1.0f;
			}
		}
		IP::mulMask(srcImg, mask);

	}
	else if (srcImg.depth() == CV_8U) {

		bitwise_not(srcImg, srcImg);
		IP::mulMask(srcImg, mask);

	}
	else {
		qDebug() << "[DkIP::invertImg] the input image's depth must be 32F or 8U\n";
	}
}

/// <summary>
/// Applies a mask to the image.
/// </summary>
/// <param name="src">The source img CV_8U or CV_32F.</param>
/// <param name="mask">The optional mask CV_8U or CV_32F.</param>
void IP::mulMask(cv::Mat& src, cv::Mat mask) {
	// do nothing if the mask is empty
	if (!mask.empty()) {

		if (src.depth() == CV_64F && mask.depth() == CV_8U)
			mulMaskIntern<double, unsigned char>(src, mask);
		else if (src.depth() == CV_32F && mask.depth() == CV_32F)
			mulMaskIntern<float, float>(src, mask);
		else if (src.depth() == CV_32F && mask.depth() == CV_8U)
			mulMaskIntern<float, unsigned char>(src, mask);
		else if (src.depth() == CV_8U && mask.depth() == CV_32F)
			mulMaskIntern<unsigned char, float>(src, mask);
		else if (src.depth() == CV_8U && mask.depth() == CV_8U)
			mulMaskIntern<unsigned char, unsigned char>(src, mask);
		else if (src.depth() == CV_32S && mask.depth() == CV_8U)
			mulMaskIntern<int, unsigned char>(src, mask);
		else {
			qDebug() << "The source image and the mask must be [CV_8U or CV_32F]";
		}
	}
}

/// <summary>
/// Prefilters an binary image according to the blob size.
/// Should be done to remove small blobs and to reduce the runtime of cvFindContours.
/// </summary>
/// <param name="img">The source img CV_8UC1.</param>
/// <param name="minArea">The blob size threshold in pixel.</param>
/// <param name="maxArea">The maximum area.</param>
/// <returns>A CV_8UC1 binary image with all blobs smaller than minArea removed.</returns>
cv::Mat IP::preFilterArea(const cv::Mat& img, int minArea, int maxArea) {

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

	while (si < sie) {
		if (*si) {
			fgListPos = 0;
			cur_fgListPos = 0;
			_PUSH_FG_LIST_CLR(si);

			// 3 2 1
			// 4 X 8
			// 5 6 7

			while (cur_fgListPos < fgListPos) {
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

			if (fgListPos <= minArea || (maxArea != -1 && fgListPos >= maxArea)) {
				cur_fgListPos = 0;

				while (cur_fgListPos < fgListPos) {
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

/// <summary>
/// Computes the histogram of an image.
/// </summary>
/// <param name="img">The source img CV_32FC1.</param>
/// <param name="mask">The mask CV_8UC1 or CV_32FC1.</param>
/// <returns>The histogram of the img as cv::mat CV_32FC1.</returns>
cv::Mat IP::computeHist(const cv::Mat img, const cv::Mat mask) {

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

/// <summary>
/// Gets the Otsu threshold based on a certain histogram.
/// </summary>
/// <param name="hist">The histogram CV_32FC1.</param>
/// <param name="otsuThresh">The otsu threshold - deprecated.</param>
/// <returns>The computed threshold.</returns>
double IP::getThreshOtsu(const cv::Mat& hist, const double otsuThresh) {
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
	double low = 0, high = 0, delta = 0;
	double mu1 = 0, q1 = 0;
	double max_sigma = 0;

	count = hist.cols;
	h = hist.ptr<float>();

	low = 0;
	high = count;

	delta = (high - low) / count;
	low += delta*0.5;

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

/// <summary>
/// Estimates the mask.
/// </summary>
/// <param name="src">The source image.</param>
/// <returns>The estimated mask.</returns>
cv::Mat IP::estimateMask(const cv::Mat& src, bool preFilter) {

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

	mask = IP::threshOtsu(mask, CV_THRESH_BINARY);
	if (preFilter)
		mask = IP::preFilterArea(mask, 10);
	//cv::Mat hist = IP::computeHist(srcGray);
	//double thresh = IP::getThreshOtsu(hist);

	//Image::save(mask, "D:\\tmp\\mask1.tif");

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

	//Image::save(zeroBorderMask, "D:\\tmp\\zeroBorderMask1.tif");
	//Image::save(smallZeroBorderMask, "D:\\tmp\\smallzeroBorderMask1.tif");

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

		//Image::save(border, "D:\\tmp\\border1.tif");

		// if the background is perfectly uniform, the std might get < 0.003
		if (stdDevBorder[0] < 0.051) stdDevBorder[0] = 0.051;

		zeroBorderMask.setTo(0);
		bBlob.drawBlob(zeroBorderMask, 255, 0);

		//Image::save(zeroBorderMask, "D:\\tmp\\blob1.tif");
		//Image::save(srcGray, "D:\\tmp\\srcGray1.tif");
		//Image::imageInfo(srcGray, "srcGray");
		//Image::imageInfo(zeroBorderMask, "zeroBorderMask");

		//WARNING: original code just uses cv::FLOODFILL_MASK_ONLY only
		//due to a bug in floodfill, see https://github.com/Itseez/opencv/issues/5123 ,
		// cv::FLOODFILL_MASK_ONLY | 4 | 1 << 8 is used.
		//check if that effects the estimate mask method
		for (int x = 0; x < srcGray.cols; ) {
			cv::Rect rect;
			cv::Point p1(x, 0);
			cv::Point p2(x, srcGray.rows - 1);
			if (srcGray.at<float>(p1) <= meanBorder[0])
				cv::floodFill(srcGray, zeroBorderMask, cv::Point(x, 0), cv::Scalar(2000), &rect, cv::Scalar(5), stdDevBorder, cv::FLOODFILL_MASK_ONLY | 4 | 1 << 8); // newVal (Scalar(2000)) is ignored when using MASK_ONLY
			if (srcGray.at<float>(p2) <= meanBorder[0])
				cv::floodFill(srcGray, zeroBorderMask, cv::Point(x, srcGray.rows - 1), cv::Scalar(2000), &rect, cv::Scalar(5), stdDevBorder, cv::FLOODFILL_MASK_ONLY | 4 | 1 << 8); // newVal (Scalar(2000)) is ignored when using MASK_ONLY
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
				cv::floodFill(srcGray, zeroBorderMask, cv::Point(0, y), cv::Scalar(2000), &rect, cv::Scalar(5), stdDevBorder, cv::FLOODFILL_MASK_ONLY | 4 | 1 << 8); // newVal (Scalar(2000)) is ignored when using MASK_ONLY
			if (srcGray.at<float>(p2) <= meanBorder[0])
				cv::floodFill(srcGray, zeroBorderMask, cv::Point(srcGray.cols - 1, y), cv::Scalar(2000), &rect, cv::Scalar(5), stdDevBorder, cv::FLOODFILL_MASK_ONLY | 4 | 1 << 8); // newVal (Scalar(2000)) is ignored when using MASK_ONLY

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

	}
	else {
		qWarning() << "The thresholded image seems to be empty -> no mask created";
		mask = 255;
	}

	return mask;
}

/// <summary>
/// Rotates an image according to the angle obtained.
/// The new image bounds are minimized with respect to
/// the angle obtained.
/// positive angle values mean counterclockwise rotation
/// </summary>
/// <param name="src">The source image.</param>
/// <param name="angleRad">The angle in RAD.</param>
/// <param name="interpolation">The interpolation.</param>
/// <param name="borderValue">The border value.</param>
/// <returns>The rotated image.</returns>
cv::Mat IP::rotateImage(const cv::Mat & src, double angleRad, int interpolation, cv::Scalar borderValue) {

	// check inputs
	if (src.empty()) {
		qWarning() << "I cannot rotate an empty image";
		return cv::Mat();
	}

	if (angleRad == 0.0f)
		return src.clone();
	//if (angleRad == CV_PI*0.5f)
	//	return src.clone().t();

	QPointF srcSize((float)src.cols, (float)src.rows);
	QPointF nSl = calcRotationSize(angleRad, srcSize);

	QPointF rotCenter = nSl * 0.5;

	cv::Mat rotMat = cv::getRotationMatrix2D(cv::Point((int)rotCenter.x(), (int)rotCenter.y()), angleRad *  180.0 / CV_PI, 1.0);

	QPointF cDiff = rotCenter - srcSize*0.5;
	QTransform tf;
	tf.rotateRadians(-angleRad);
	cDiff = tf.map(cDiff);

	double *transl = rotMat.ptr<double>();
	transl[2] += (double)cDiff.x();
	transl[5] += (double)cDiff.y();

	//img in wrapAffine must not be overwritten
	cv::Mat rImg = cv::Mat(cv::Size(cvCeil(nSl.x()), cvCeil(nSl.y())), src.type());
	warpAffine(src, rImg, rotMat, rImg.size(), interpolation, cv::BORDER_CONSTANT, borderValue);

	return rImg;
}

/// <summary>
/// Calculates the image size of the rotated image.
/// </summary>
/// <param name="angleRad">The angle in radians.</param>
/// <param name="srcSize">Size of the source image.</param>
/// <returns>The Size of the rotated image.</returns>
QPointF IP::calcRotationSize(double angleRad, const QPointF& srcSize) {
	QPointF nSl = srcSize;
	QPointF nSr(srcSize.y(), srcSize.x());

	QTransform tf;

	tf.rotateRadians(-angleRad);
	nSl = tf.map(nSl);
	//absolute value
	nSl.setX(qAbs(nSl.x()));
	nSl.setY(qAbs(nSl.y()));

	nSr = tf.map(nSr);
	//absolute value
	nSr.setX(qAbs(nSr.x()));
	nSr.setY(qAbs(nSr.y()));

	QPointF resultSize(qMax(nSl.x(), nSr.y()), qMax(nSl.y(), nSr.x()));

	return resultSize;
}

}