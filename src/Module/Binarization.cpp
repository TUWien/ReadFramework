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

#include "Binarization.h"
#include "Algorithms.h"
#include "Image.h"

#pragma warning(push, 0)	// no warnings from includes
#include <QDebug>
#include <QSettings>
#include <qmath.h>
#include "opencv2/imgproc/imgproc.hpp"
#pragma warning(pop)

namespace rdf {

// SimpleBinarization --------------------------------------------------------------------
/// <summary>
/// Initializes a new instance of the <see cref="SimpleBinarization"/> class.
/// </summary>
/// <param name="img">The source img CV_8U.</param>
SimpleBinarization::SimpleBinarization(const cv::Mat& srcImg) {
	mSrcImg = srcImg;
	
	mModuleName = "SimpleBinarization";
	//loadSettings();
}

bool SimpleBinarization::checkInput() const {
	
	if (mSrcImg.depth() != CV_8U) {
		mWarning << "illegal image depth: " << mSrcImg.depth();
		return false;
	}

	return true;
}

/// <summary>
/// Determines whether this instance is empty.
/// </summary>
/// <returns>True if the instance is empty.</returns>
bool SimpleBinarization::isEmpty() const {
	return mSrcImg.empty();
}

void SimpleBinarization::load(const QSettings& settings) {
	
	mThresh = settings.value("thresh", mThresh).toInt();
}

void SimpleBinarization::save(QSettings& settings) const {

	settings.setValue("thresh", mThresh);
}

/// <summary>
/// Returns the binary image.
/// </summary>
/// <returns>The binary image CV_8UC1.</returns>
cv::Mat SimpleBinarization::binaryImage() const {
	return mBwImg;
}

//void SimpleBinarization::setThresh(int thresh) {
//	mThresh = thresh;
//}
//
//int SimpleBinarization::thresh() const {
//	return mThresh;
//}

/// <summary>
/// Computes the binary image.
/// </summary>
/// <returns>True if the image could be computed.</returns>
bool SimpleBinarization::compute() {

	if (!checkInput())
		return false;

	mBwImg = mSrcImg > mThresh;

	// I guess here is a good point to save the settings
	saveSettings();
	mDebug << " computed...";
	mWarning << "a warning...";
	mInfo << "an info...";

	return true;
}

/// <summary>
/// Summary of the class.
/// </summary>
/// <returns>A summary string</returns>
QString SimpleBinarization::toString() const {
	
	QString msg = debugName();
	msg += " thresh: " + QString::number(mThresh);

	return msg;
}

// BaseBinarizationSu --------------------------------------------------------------------
/// <summary>
/// Initializes a new instance of the <see cref="BaseBinarizationSu"/> class.
/// </summary>
/// <param name="img">The source img CV_8U.</param>
/// <param name="mask">The optional mask image CV_8UC1.</param>
BaseBinarizationSu::BaseBinarizationSu(const cv::Mat& img, const cv::Mat& mask) {
	mSrcImg = img;
	mMask = mask;

	if (mMask.empty()) {
		mMask = cv::Mat(mSrcImg.size(), CV_8UC1, cv::Scalar(255));
	}

	mModuleName = "BaseBinarizationSu";
	//loadSettings();
}

bool BaseBinarizationSu::checkInput() const {

	if (mSrcImg.depth() != CV_8U) {
		mWarning << "illegal image depth: " << mSrcImg.depth();
		return false;
	}

	if (mMask.depth() != CV_8U && mMask.channels() != 1) {
		mWarning << "illegal image depth or channel for mask: " << mMask.depth();
		return false;
	}

	return true;
}

/// <summary>
/// Determines whether this instance is empty.
/// </summary>
/// <returns>True if the source image is not set.</returns>
bool BaseBinarizationSu::isEmpty() const {
	return mSrcImg.empty() && mMask.empty();
}

void BaseBinarizationSu::load(const QSettings& settings) {

	mErodeMaskSize = settings.value("erodeMaskSize", mErodeMaskSize).toInt();
}

void BaseBinarizationSu::save(QSettings& settings) const {

	settings.setValue("erodeMaskSize", mErodeMaskSize);
}

/// <summary>
/// Returns the binary image.
/// </summary>
/// <returns>The binary image CV_8UC1.</returns>
cv::Mat BaseBinarizationSu::binaryImage() const {
	return mBwImg;
}

/// <summary>
/// Computes the thresholded image.
/// </summary>
/// <returns>True if the binary image could be computed.</returns>
bool BaseBinarizationSu::compute() {

	if (!checkInput())
		return false;

	cv::Mat erodedMask = Algorithms::instance().erodeImage(mMask, cvRound(mErodeMaskSize), Algorithms::SQUARE, 0);

	//Image::instance().imageInfo(erodedMask, "erodedMAsk");
	//qDebug() << "mErodedMaskSize " << cvRound(mErodeMaskSize);
	//Image::instance().save(erodedMask, "D:\\tmp\\maskTest.tif");
	//Image::instance().save(mSrcImg, "D:\\tmp\\mSrcImg.tif");

	cv::Mat contrastImg = compContrastImg(mSrcImg, erodedMask);
	cv::Mat binContrastImg = compBinContrastImg(contrastImg);

	//Image::instance().save(contrastImg, "D:\\tmp\\contrastImgBase.tif");
	//Image::instance().save(binContrastImg, "D:\\tmp\\bincontrastBase.tif");

	cv::Mat maskedContrastImg;
	binContrastImg.convertTo(maskedContrastImg, CV_32F, 1.0f / 255.0f);
	maskedContrastImg = contrastImg.mul(maskedContrastImg);
	mStrokeW = strokeWidth(maskedContrastImg);
	maskedContrastImg.release();

	//qDebug() << "Estimated Strokewidth: " << mStrokeW;
	//Image::instance().save(contrastImg, "D:\\tmp\\contrastImg.tif");
	//Image::instance().save(binContrastImg, "D:\\tmp\\bincontrastImg.tif");


	// now we need a 32F image
	cv::Mat srcGray = mSrcImg.clone();
	if (srcGray.channels() != 1) cv::cvtColor(mSrcImg, srcGray, CV_RGB2GRAY);
	if (srcGray.depth() == CV_8U) srcGray.convertTo(srcGray, CV_32F, 1.0f/255.0f);


	cv::Mat thrImg, resultSegImg;
	computeThrImg(srcGray, binContrastImg, thrImg, resultSegImg);					//compute threshold image

	//Image::instance().save(thrImg, "D:\\tmp\\thrImg.tif");
	//Image::instance().save(resultSegImg, "D:\\tmp\\resultSegImg.tif");

	cv::bitwise_and(resultSegImg, srcGray <= (thrImg), resultSegImg);		//combine with Nmin condition

	if (mPreFilter)
		mBwImg = Algorithms::instance().preFilterArea(mBwImg, mPreFilterSize);

	mBwImg = resultSegImg.clone();


	// I guess here is a good point to save the settings
	//saveSettings();
	mDebug << " computed...";
	//mWarning << "a warning...";
	//mInfo << "an info...";

	return true;
}

/// <summary>
/// Sets the preFiltering. All blobs smaller than preFilterSize are removed if preFilter is true.
/// </summary>
/// <param name="preFilter">if set to <c>true</c> [prefilter] is applied and all blobs smaller then preFilterSize are removed.</param>
/// <param name="preFilterSize">Size of the prefilter.</param>
void BaseBinarizationSu::setPreFiltering(bool preFilter, int preFilterSize) {
	mPreFilter = preFilter; 
	mPreFilterSize = preFilterSize;
}

cv::Mat BaseBinarizationSu::compContrastImg(const cv::Mat& srcImg, const cv::Mat& mask) const {
	
	if (srcImg.depth() != CV_8U) {
		qWarning() << "8U is required";
		return cv::Mat();
	}

	cv::Mat contrastImg(srcImg.size(), CV_32FC1);
	cv::Mat srcGray;
	if (srcImg.channels() != 1)
		cv::cvtColor(srcImg, srcGray, CV_RGB2GRAY);
	else
		srcGray = srcImg;

	//Image::instance().save(srcGray, "D:\\tmp\\srcGray.tif");
	cv::Mat maxImg = Algorithms::instance().dilateImage(srcGray, 3, Algorithms::SQUARE);
	cv::Mat minImg = Algorithms::instance().erodeImage(srcGray, 3, Algorithms::SQUARE);
	//Image::instance().save(maxImg, "D:\\tmp\\maxImgAdapted.tif");
	//Image::instance().save(minImg, "D:\\tmp\\minImgAdapted.tif");
	
	//speed up version of opencv style
	for (int i = 0; i < maxImg.rows; i++)
	{
		float *ptrCon = contrastImg.ptr<float>(i);
		unsigned char *ptrMin = minImg.ptr<unsigned char>(i);
		unsigned char *ptrMax = maxImg.ptr<unsigned char>(i);
		unsigned const char *ptrMask = mask.ptr<unsigned char>(i);

		for (int j = 0; j < maxImg.cols; j++, ptrCon++, ptrMin++, ptrMax++, ptrMask++) {
			*ptrCon = (*ptrMask > 0) ? contrastVal(ptrMax, ptrMin) : 0.0f;
		}
	}

	return contrastImg;
}

inline float BaseBinarizationSu::contrastVal(const unsigned char* maxVal, const unsigned char * minVal) const {

	return (float)(*maxVal - *minVal) / ((float)(*maxVal) + (float)(*minVal) + FLT_MIN);
}

cv::Mat BaseBinarizationSu::compBinContrastImg(const cv::Mat& contrastImg) const {

	cv::Mat contrastImgThr;
	//rdf::Image::instance().imageInfo(contrastImg, "contrastImg");
	contrastImg.convertTo(contrastImgThr, CV_8U, 255, 0); //(8U is needed for OTSU)
	cv::threshold(contrastImgThr, contrastImgThr, 0, 255, CV_THRESH_BINARY | CV_THRESH_OTSU);
	//contrastImgThr.convertTo(contrastImgThr, CV_8U, 255, 0);

	return contrastImgThr;
}


float BaseBinarizationSu::strokeWidth(const cv::Mat& contrastImg) const {

	int height = contrastImg.rows;
	int dy = 1;
	//	int cnt=0;
	float strokeWidth = 0;
	QList<int> diffs;
	QList<float> locInt;
	diffs.clear();
	cv::Mat vec;

	if (height > 100) dy = height / 4;
	dy = 1;

	for (int i = 0; i < height; i += dy) {
		cv::Mat row = contrastImg.row(i);
		computeDistHist(row, &diffs, &locInt, 0.0f);
	}

	vec.create(1, 40, CV_32FC1);
	vec = 0.0f;
	QList<int>::iterator iter = diffs.begin();
	
	float *ptr = vec.ptr<float>(0);
	int idx;
	while (iter != diffs.end()) {
		idx = (*iter);
		if (idx < vec.cols)
			ptr[idx]++;//=(*iter2);

		iter++;
	}
	diffs.clear();

	cv::Mat sHist = vec;

	double sMin, sMax;
	cv::Point pMinIdx, pMaxIdx;
	int minIdx, maxIdx;
	cv::minMaxLoc(sHist, &sMin, &sMax, &pMinIdx, &pMaxIdx);
	minIdx = pMinIdx.x;
	maxIdx = pMaxIdx.x;

	strokeWidth = 1.0f + (float)maxIdx;  //offset since idx starts with 0

	if (strokeWidth < 3)
		return 3.0f;
	else
		return strokeWidth;
}

void BaseBinarizationSu::computeDistHist(const cv::Mat& src, QList<int> *maxDiffList, QList<float> *localIntensity, float gSigma) const {

	QList<int> localMaxList;
	QList<int>::iterator localMaxIter;

	cv::Mat sHist;
	if (gSigma > 0)
		sHist = Algorithms::instance().convolveSymmetric(src, Algorithms::instance().get1DGauss(gSigma));
	else
		sHist = src;

	localMaxList.clear();

	float *sHistPtr = sHist.ptr<float>();

	for (int prevIdx = 0, currIdx = 1, nextIdx = 2; prevIdx < sHist.cols; prevIdx++, nextIdx++, currIdx++) {

		currIdx %= sHist.cols;
		nextIdx %= sHist.cols;

		if ((sHistPtr[prevIdx] <= sHistPtr[currIdx]) && (sHistPtr[currIdx] > sHistPtr[nextIdx])) {
			localMaxList.push_back(currIdx);
			localIntensity->push_back(sHistPtr[currIdx]);
			//localIntensity->push_back(255.0f);
		}
	}
	if (localIntensity->size() >= 1)
		localIntensity->pop_back();

	if (localMaxList.empty() || localMaxList.size() <= 1)
		return;	// at least two local maxima present?

	localMaxIter = localMaxList.begin();
	int cIdx = *localMaxIter;
	++localMaxIter;	// skip the first element

					// compute the distance between peaks
	int lIdx = -1;
	int idx = 1;
	while (localMaxIter != localMaxList.end()) {

		lIdx = cIdx;
		cIdx = *localMaxIter;

		//if (idx%2 == 0)
		maxDiffList->push_back(abs(cIdx - lIdx));
		idx++;
		//printf("distance: %i\n", cIdx-lIdx);
		++localMaxIter;
	}
}

void BaseBinarizationSu::computeThrImg(const cv::Mat& grayImg32F, const cv::Mat& binContrast, cv::Mat& thresholdImg, cv::Mat& thresholdContrastPxImg) {
	int filtersize, Nmin;

	calcFilterParams(filtersize, Nmin);
	//filtersize = cvRound(strokeW);
	//filtersize = (filtersize % 2) != 1 ? filtersize+1 : filtersize;
	//Nmin = filtersize;
	//qDebug() << "kernelsize: " << filtersize << " nMin: " << Nmin;

	cv::Mat contrastBin32F;
	binContrast.convertTo(contrastBin32F, CV_32FC1, 1.0f / 255.0f);
	//DkIP::imwrite("contrastBin343.png", binContrast);
	// compute the mean image
	//rdf::Image::instance().imageInfo(grayImg32F, "grayImg32F");
	//rdf::Image::instance().imageInfo(contrastBin32F, "contrastBin32F");
	cv::Mat meanImg = grayImg32F.mul(contrastBin32F);	// do not overwrite the gray image

														// 1-dimensional since the filter is symmetric
	//cv::Mat sumKernel = cv::Mat(3, 1, CV_32FC1);
	//sumKernel = 1.0/3.0;
	//cv::filter2D(meanImg, meanImg, CV_32FC1, sumKernel);

	cv::Mat stdImg = meanImg.mul(meanImg);
	//Image::instance().save(meanImg, "D:\\tmp\\meanImgAdapted.tif");

	cv::Mat intContrastBinary = contrastBin32F;

	// save RAM for small filter sizes
	if (filtersize <= 7) {

		qDebug() << "calling the new mean....";
		// 1-dimensional since the filter is symmetric
		cv::Mat sumKernel = cv::Mat(filtersize, 1, CV_32FC1);
		sumKernel = 1.0;

		// filter y-coordinates
		cv::filter2D(meanImg, meanImg, CV_32FC1, sumKernel);
		cv::filter2D(stdImg, stdImg, CV_32FC1, sumKernel);
		cv::filter2D(intContrastBinary, intContrastBinary, CV_32FC1, sumKernel);

		// filter x-coordinates
		sumKernel = sumKernel.t();
		cv::filter2D(meanImg, meanImg, CV_32FC1, sumKernel);
		cv::filter2D(stdImg, stdImg, CV_32FC1, sumKernel);
		cv::filter2D(intContrastBinary, intContrastBinary, CV_32FC1, sumKernel);

	}
	else {

		// is way faster than the filter2D function for kernels > 7
		cv::Mat intImg;
		integral(meanImg, intImg);
		meanImg = Algorithms::instance().convolveIntegralImage(intImg, filtersize, 0, Algorithms::BORDER_ZERO);
		//meanImg = Algorithms::instance().convolveIntegralImage(intImg, filtersize, 0, Algorithms::BORDER_FLIP);

		// compute the standard deviation image
		integral(stdImg, intImg);
		stdImg = Algorithms::instance().convolveIntegralImage(intImg, filtersize, 0, Algorithms::BORDER_ZERO);
		//stdImg = Algorithms::instance().convolveIntegralImage(intImg, filtersize, 0, Algorithms::BORDER_FLIP);
		intImg.release(); // early release

		integral(contrastBin32F, intContrastBinary);
		contrastBin32F.release();
		intContrastBinary = Algorithms::instance().convolveIntegralImage(intContrastBinary, filtersize, 0, Algorithms::BORDER_ZERO);
		//intContrastBinary = Algorithms::instance().convolveIntegralImage(intContrastBinary, filtersize, 0, Algorithms::BORDER_FLIP);
	}
	//DkIP::imwrite("meanImg343.png", meanImg, true);
	//Image::instance().save(meanImg, "D:\\tmp\\meanImg2Adapted.tif");
	meanImg /= intContrastBinary;

	//FK new filtering because of jpg artefacts - otherwise horizontal and vertical lines appear
	cv::Mat sumKernel = cv::Mat(11, 11, CV_32FC1);
	sumKernel = 1.0/(11.0*11.0);
	cv::filter2D(meanImg, meanImg, CV_32FC1, sumKernel);
	//Image::instance().save(meanImg, "D:\\tmp\\meanImg3Adapted.tif");

	float *mPtr, *cPtr;
	float *stdPtr;

	for (int rIdx = 0; rIdx < stdImg.rows; rIdx++) {

		mPtr = meanImg.ptr<float>(rIdx);
		stdPtr = stdImg.ptr<float>(rIdx);
		cPtr = intContrastBinary.ptr<float>(rIdx);

		for (int cIdx = 0; cIdx < stdImg.cols; cIdx++, mPtr++, stdPtr++, cPtr++) {

			*stdPtr = (*cPtr != 0) ? *stdPtr / (*cPtr) - (*mPtr * *mPtr) : 0.0f;	// same as OpenCV 0 division
			if (*stdPtr < 0.0f) *stdPtr = 0.0f;		// sqrt throws floating point exception if stdPtr < 0

		}
	}

	sqrt(stdImg, stdImg);	// produces a floating point exception if < 0...

							//DkIP::imwrite("stdImg371.png", stdImg);

	cv::Mat thrImgTmp = cv::Mat(grayImg32F.size(), CV_32FC1);
	cv::Mat segImgTmp = cv::Mat(grayImg32F.size(), CV_8UC1);

	float *ptrStd, *ptrThr, *ptrSumContrast, *ptrMean;
	unsigned char *ptrSeg;

	for (int rIdx = 0; rIdx < stdImg.rows; rIdx++) {
		ptrThr = thrImgTmp.ptr<float>(rIdx);
		ptrMean = meanImg.ptr<float>(rIdx);
		ptrStd = stdImg.ptr<float>(rIdx);
		ptrSeg = segImgTmp.ptr<unsigned char>(rIdx);
		ptrSumContrast = intContrastBinary.ptr<float>(rIdx);

		for (int cIdx = 0; cIdx < stdImg.cols; cIdx++, ptrThr++, ptrMean++, ptrStd++, ptrSeg++, ptrSumContrast++) {
			*ptrThr = thresholdVal(ptrMean, ptrStd);
			*ptrSeg = *ptrSumContrast > Nmin ? 255 : 0;
		}
	}

	//thresholdContrastPxImg = binContrast;
	thresholdContrastPxImg = segImgTmp;
	thresholdImg = thrImgTmp;
}

inline void BaseBinarizationSu::calcFilterParams(int &filterS, int &Nm) {
	filterS = cvRound(mStrokeW);
	filterS = (filterS % 2) != 1 ? filterS + 1 : filterS;
	Nm = filterS;
}

inline float BaseBinarizationSu::thresholdVal(float *mean, float *std) const {
	//qDebug() << "Mean" << *mean << " std " << *std;
	return (*mean + *std / 2);
	//return *mean/1.05;
}

/// <summary>
/// Summary of the method.
/// </summary>
/// <returns>The summary string.</returns>
QString BaseBinarizationSu::toString() const {

	QString msg = debugName();
	//msg += "strokeW: " + QString::number(mStrokeW);
	msg += "  erodedMasksize: " + QString::number(mErodeMaskSize);

	return msg;
}

/// <summary>
/// Initializes a new instance of the <see cref="BinarizationSuAdapted"/> class.
/// </summary>
/// <param name="img">The source img CV_8U.</param>
/// <param name="mask">The optional mask CV_8UC1.</param>
BinarizationSuAdapted::BinarizationSuAdapted(const cv::Mat & img, const cv::Mat & mask) : BaseBinarizationSu(img, mask) {
	mModuleName = "BaseBinarizationAdapted";
}

/// <summary>
/// Computes the binary image using the adapted method of Su.
/// </summary>
/// <returns>True if the thresholded image could be computed.</returns>
bool BinarizationSuAdapted::compute() {

	
	if (!checkInput())
		return false;

	cv::Mat erodedMask = Algorithms::instance().erodeImage(mMask, cvRound(mErodeMaskSize), Algorithms::SQUARE);

	mContrastImg = compContrastImg(mSrcImg, erodedMask);
	//Image::instance().save(mContrastImg, "D:\\tmp\\contrastImgAdapted.tif");
	mBinContrastImg = compBinContrastImg(mContrastImg);

	//mStrokeW = 4; // = default value
	//Image::instance().imageInfo(binContrastImg, "binConrtastImage");
	//Image::instance().save(mBinContrastImg, "D:\\tmp\\binContrastAdapted.tif");
	
	// now we need a 32F image
	cv::Mat srcGray = mSrcImg.clone();
	if (mSrcImg.channels() != 1) cv::cvtColor(mSrcImg, srcGray, CV_RGB2GRAY);
	if (srcGray.depth() == CV_8U) srcGray.convertTo(srcGray, CV_32F, 1.0f / 255.0f);

	//Image::instance().save(srcGray, "D:\\tmp\\srcGrayAdapted.tif");
	//Image::instance().save(mBinContrastImg, "D:\\tmp\\binContrastAdapted.tif");

	cv::Mat resultSegImg;

	//FK new: inserted morphological opening to reduce jpg artefacts - must be checked in overall.
	cv::Mat tmp = srcGray.clone();
	cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5,5));
	cv::erode(tmp, tmp, kernel);
	cv::dilate(tmp, tmp, kernel);

	computeThrImg(tmp, mBinContrastImg, mThrImg, resultSegImg);					//compute threshold image
	//computeThrImg(srcGray, mBinContrastImg, mThrImg, resultSegImg);					//compute threshold image
	//Image::instance().save(mThrImg, "D:\\tmp\\thrImgAdapted.tif");
	//Image::instance().save(resultSegImg, "D:\\tmp\\resultSegAdapted.tif");
	//Image::instance().save(srcGray <= (mThrImg), "D:\\tmp\\leqAdapted.tif");

	cv::bitwise_and(resultSegImg, srcGray <= (mThrImg), resultSegImg);		//combine with Nmin condition
	mBwImg = resultSegImg.clone();
	//Image::instance().save(resultSegImg, "D:\\tmp\\mBwimg.tif");

	if (mPreFilter)
		mBwImg = Algorithms::instance().preFilterArea(mBwImg, mPreFilterSize);

	//if (mMedianFilter)
	//	cv::medianBlur(mBwImg, mBwImg, 3);

	// I guess here is a good point to save the settings
	//or not because it is not thread saFe!
	//saveSettings();
	mDebug << " computed...";

	return true;

	//if (medianFilter)
	//	medianBlur(segImg, segImg, 3);
}

float BinarizationSuAdapted::contrastVal(const unsigned char* maxVal, const unsigned char * minVal) const {

	return 2.0f*(float)(*maxVal - *minVal) / ((float)(*maxVal) + (float)(*minVal) + 255.0f + FLT_MIN);
}

inline float BinarizationSuAdapted::thresholdVal(float *mean, float *std) const {
	//qDebug() << "Mean" << *mean << " std " << *std;
	//return *std < 0.1 ? (*mean + *std / 2) : *mean;
	return (*mean + *std / 2);
	//return *mean/1.05;
}


void BinarizationSuAdapted::calcFilterParams(int &filterS, int &Nm) {

	//if (mStrokeW >= 4.5) mStrokeW = 3.0;		//eventually strokeW should be set to 3.0 as initial value!!!
	//mStrokeW = 3.0f;
	filterS = cvRound(mStrokeW * 10);
	if ((filterS % 2) != 1) filterS += 1;
	Nm = cvFloor(mStrokeW * 10);
}

/// <summary>
/// If set, a median filter of size 3 is applied at the end.
/// </summary>
/// <param name="medianFilter">if set to <c>true</c> [median filter] with size 3 is applied.</param>
void BinarizationSuAdapted::setFiltering(bool medianFilter) {
	mMedianFilter = medianFilter;
}

float BinarizationSuAdapted::setStrokeWidth(float strokeW) {
	return mStrokeW = strokeW;
}

/// <summary>
/// Summary of parameters as String.
/// </summary>
/// <returns>Summary of parameters.</returns>
QString BinarizationSuAdapted::toString() const {

	QString msg = debugName();
	msg += " computed... ";
	msg += " strokeW: " + QString::number(mStrokeW);
	msg += " erodedMasksize: " + QString::number(mErodeMaskSize);

	return msg;
}


/// <summary>
/// Initializes a new instance of the <see cref="BinarizationSuFgdWeight"/> class.
/// </summary>
/// <param name="img">The img.</param>
/// <param name="mask">The mask.</param>
BinarizationSuFgdWeight::BinarizationSuFgdWeight(const cv::Mat & img, const cv::Mat & mask) : BinarizationSuAdapted(img, mask) {
	mModuleName = "BinarizationSuFgdWeight";
}

/// <summary>
/// Computes the foreground weighted image.
/// </summary>
/// <returns>True if the image could be computed.</returns>
bool BinarizationSuFgdWeight::compute() {

	bool stat = BinarizationSuAdapted::compute();
	//result is binarized image mBwImg
	
	mMeanContrast = computeConfidence();

	//Image::instance().save(mBwImg, "D:\\tmp\\bwimgTest.tif");
	//qDebug() << "meanContrast: " << mMeanContrast[0];

	cv::Mat srcGray = mSrcImg.clone();
	if (srcGray.channels() != 1) cv::cvtColor(mSrcImg, srcGray, CV_RGB2GRAY);
	if (srcGray.depth() == CV_8U) srcGray.convertTo(srcGray, CV_32F, 1.0f / 255.0f);

	//Image::instance().save(mThrImg, "D:\\tmp\\thrFgdInput.tif");
	cv::Mat erodedMask = Algorithms::instance().erodeImage(mMask, cvRound(mErodeMaskSize), Algorithms::SQUARE);
	weightFunction(srcGray, mThrImg, erodedMask);

	cv::bitwise_and(mBwImg, srcGray <= (mThrImg), mBwImg);		//combine with Nmin condition

	mDebug << " computed...";

	return stat;
}

cv::Scalar BinarizationSuFgdWeight::computeConfidence() const {
	
	cv::Scalar m = cv::Scalar(-1.0f, -1.0f, -1.0f, -1.0f);
	m[1] = mMeanContrast[1];
	m[2] = mMeanContrast[2];

	if (mMeanContrast[0] == -1.0f) {
		int n = cv::countNonZero(mBinContrastImg);
		//TODO: prove if contrastImg in normalize and statmomentMat must be set to tmp?
		//Mat tmp = mContrastImg.clone();
		cv::normalize(mContrastImg, mContrastImg, 1, 0, cv::NORM_MINMAX, -1, mBinContrastImg);
		if (n > 2000)
			m[0] = rdf::Algorithms::instance().statMomentMat(mContrastImg, mBinContrastImg, 0.5f, 5000);
		else
			m[0] = 0.0f;
	}

	return m;
}

void BinarizationSuFgdWeight::weightFunction(cv::Mat& grayImg, cv::Mat& tImg, const cv::Mat& mask) {

	mFgdEstImg = computeMeanFgdEst(grayImg, mask);					//compute foreground estimation
	//Image::instance().save(mFgdEstImg, "D:\\tmp\\thrFgdEst.tif");

	cv::Mat tmpMask;
	
	//FK changes to old version:
	//instead of threshold 0.0 -> 50/255 is used
	//due to the filtering, values are not 0 any more...
	cv::threshold(tImg, tmpMask, 50.0/255.0, 1.0, CV_THRESH_BINARY);
	//cv::threshold(tImg, tmpMask, 0, 255, CV_THRESH_BINARY_INV | CV_THRESH_OTSU);
	cv::Mat histogram = rdf::Algorithms::instance().computeHist(tImg, tmpMask);		//weight gray values with sigmoid function according

	//Image::instance().save(tImg, "D:\\tmp\\tImg.tif");
	//Image::instance().save(tmpMask, "D:\\tmp\\tmpMask.tif");
	//Image::instance().save(histogram, "D:\\tmp\\tmpHist.tif");
	//Image::instance().imageInfo(histogram, "histogram");
	//qDebug().noquote() << Image::instance().printImage(histogram, "histogram");

	tmpMask.release();

	double l = rdf::Algorithms::instance().getThreshOtsu(histogram) / 255.0f;		//sigmoid slope, centered at l according text estimation
	float sigmaSlopeTmp = mSigmSlope / 255.0f;

	//qDebug() << "otsu: " << l << " sigmaSlope: " << sigmaSlopeTmp;

	double fm[256];
	for (int i = 0; i < 256; i++)
		fm[i] = 1.0 / (1.0 + qExp(((i / 255.0) - l) * (-1.0 / (sigmaSlopeTmp))));

	for (int i = 0; i < grayImg.rows; i++) {
		float *ptrGray = grayImg.ptr<float>(i);
		float *ptrThr = tImg.ptr<float>(i);
		float *ptrFgdEst = mFgdEstImg.ptr<float>(i);
		unsigned char const *ptrMask = mask.ptr<unsigned char>(i);

		for (int j = 0; j < grayImg.cols; j++, ptrGray++, ptrThr++, ptrFgdEst++, ptrMask++) {
			*ptrGray = (float)fm[cvRound(*ptrGray*255.0f)];
			*ptrThr = (*ptrMask != 0) ? *ptrThr * (*ptrFgdEst) : 0.0f;
		}
	}

}

cv::Mat BinarizationSuFgdWeight::computeMeanFgdEst(const cv::Mat& grayImg32F, const cv::Mat& mask) const {
	cv::Mat tmp;

	if (mFgdEstFilterSize < 3) {
		tmp = cv::Mat(grayImg32F.size(), CV_32FC1);
		tmp.setTo(1.0f);
	}
	else {
		cv::Mat fgdEstImgInt = cv::Mat(grayImg32F.rows + 1, grayImg32F.cols + 1, CV_64FC1);
		integral(grayImg32F, fgdEstImgInt);
		tmp = rdf::Algorithms::instance().convolveIntegralImage(fgdEstImgInt, mFgdEstFilterSize, 0, Algorithms::BORDER_ZERO);
		fgdEstImgInt.release(); // early release

								//DkIP::mulMask(fgdEstImg, mask);	// diem: otherwise values outside the mask are mutual
		cv::normalize(tmp, tmp, 1.0f, 0, cv::NORM_MINMAX, -1, mask);  // note: values outside the mask remain outside [0 1]
		rdf::Algorithms::instance().invertImg(tmp);
		
	}
	tmp = tmp.clone();

	return tmp;
}

cv::Mat BinarizationSuFgdWeight::computeFgd() const {

	return cv::Mat();

}


}