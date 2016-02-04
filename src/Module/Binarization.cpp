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
#include "opencv2/imgproc/imgproc.hpp"
#pragma warning(pop)

namespace rdf {

// SimpleBinarization --------------------------------------------------------------------
SimpleBinarization::SimpleBinarization(const cv::Mat& srcImg) {
	mSrcImg = srcImg;
	
	mModuleName = "SimpleBinarization";
	loadSettings();
}

bool SimpleBinarization::checkInput() const {
	
	if (mSrcImg.depth() != CV_8U) {
		mWarning << "illegal image depth: " << mSrcImg.depth();
		return false;
	}

	return true;
}

bool SimpleBinarization::isEmpty() const {
	return mSrcImg.empty();
}

void SimpleBinarization::load(const QSettings& settings) {
	
	mThresh = settings.value("thresh", mThresh).toInt();
}

void SimpleBinarization::save(QSettings& settings) const {

	settings.setValue("thresh", mThresh);
}

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

QString SimpleBinarization::toString() const {
	
	QString msg = debugName();
	msg += " thresh: " + QString::number(mThresh);

	return msg;
}

// BaseBinarizationSu --------------------------------------------------------------------
BaseBinarizationSu::BaseBinarizationSu(const cv::Mat& img, const cv::Mat& mask) {
	mSrcImg = img;
	mMask = mask;

	if (mMask.empty()) {
		mMask = cv::Mat(mSrcImg.size(), CV_8UC1, cv::Scalar(255));
	}

	mModuleName = "BaseBinarizationSu";
	loadSettings();
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

bool BaseBinarizationSu::isEmpty() const {
	return mSrcImg.empty() && mMask.empty();
}

void BaseBinarizationSu::load(const QSettings& settings) {

	mErodeMaskSize = settings.value("erodeMaskSize", mErodeMaskSize).toInt();
}

void BaseBinarizationSu::save(QSettings& settings) const {

	settings.setValue("erodeMaskSize", mErodeMaskSize);
}

cv::Mat BaseBinarizationSu::binaryImage() const {
	return mBwImg;
}

bool BaseBinarizationSu::compute() {

	if (!checkInput())
		return false;

	cv::Mat erodedMask = Algorithms::instance().erodeImage(mMask, cvRound(mErodeMaskSize), Algorithms::SQUARE);

	cv::Mat contrastImg = compContrastImg(mSrcImg, erodedMask);
	cv::Mat binContrastImg = compBinContrastImg(contrastImg);

	cv::Mat maskedContrastImg;
	binContrastImg.convertTo(maskedContrastImg, CV_32F, 1.0f / 255.0f);
	maskedContrastImg = contrastImg.mul(maskedContrastImg);
	mStrokeW = getStrokeWidth(maskedContrastImg);
	maskedContrastImg.release();

	// now we need a 32F image
	cv::Mat srcGray = mSrcImg;
	if (srcGray.channels() != 1) cv::cvtColor(mSrcImg, srcGray, CV_RGB2GRAY);
	if (srcGray.depth() == CV_8U) srcGray.convertTo(srcGray, CV_32F, 1.0f/255.0f);

	cv::Mat thrImg, resultSegImg;
	computeThrImg(srcGray, binContrastImg, thrImg, resultSegImg);					//compute threshold image

	cv::bitwise_and(resultSegImg, srcGray <= (thrImg), resultSegImg);		//combine with Nmin condition
	mBwImg = resultSegImg.clone();


	// I guess here is a good point to save the settings
	saveSettings();
	mDebug << " computed...";
	mWarning << "a warning...";
	mInfo << "an info...";

	return true;
}


cv::Mat BaseBinarizationSu::compContrastImg(const cv::Mat& srcImg, const cv::Mat& mask) const {
	
	if (srcImg.depth() != CV_8U) {
		qWarning() << "8U is required";
		return cv::Mat();
	}

	cv::Mat contrastImg = cv::Mat(srcImg.size(), CV_32FC1);
	cv::Mat srcGray = srcImg;
	if (srcGray.channels() != 1) cv::cvtColor(srcImg, srcGray, CV_RGB2GRAY);
	
	cv::Mat maxImg = Algorithms::instance().dilateImage(srcGray, 3, Algorithms::SQUARE);
	cv::Mat minImg = Algorithms::instance().erodeImage(srcGray, 3, Algorithms::SQUARE);
	
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

inline float BaseBinarizationSu::contrastVal(unsigned char* maxVal, unsigned char * minVal) const {

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


float BaseBinarizationSu::getStrokeWidth(const cv::Mat& contrastImg) const {

	int height = contrastImg.rows;
	int dy = 1;
	//	int cnt=0;
	float strokeWidth = 0;
	std::list<int> diffs;
	std::list<float> locInt;
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
	std::list<int>::iterator iter = diffs.begin();
	
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

void BaseBinarizationSu::computeDistHist(const cv::Mat& src, std::list<int> *maxDiffList, std::list<float> *localIntensity, float gSigma) const {

	std::list<int> localMaxList;
	std::list<int>::iterator localMaxIter;

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

void BaseBinarizationSu::computeThrImg(const cv::Mat& grayImg32F, const cv::Mat& binContrast, cv::Mat& thresholdImg, cv::Mat& thresholdContrastPxImg) const {
	int filtersize, Nmin;

	calcFilterParams(filtersize, Nmin);
	//filtersize = cvRound(strokeW);
	//filtersize = (filtersize % 2) != 1 ? filtersize+1 : filtersize;
	//Nmin = filtersize;
	qDebug() << "kernelsize: " << filtersize << " nMin: " << Nmin;

	cv::Mat contrastBin32F;
	binContrast.convertTo(contrastBin32F, CV_32FC1, 1.0f / 255.0f);
	//DkIP::imwrite("contrastBin343.png", binContrast);
	// compute the mean image
	rdf::Image::instance().imageInfo(grayImg32F, "grayImg32F");
	rdf::Image::instance().imageInfo(contrastBin32F, "contrastBin32F");
	cv::Mat meanImg = grayImg32F.mul(contrastBin32F);	// do not overwrite the gray image
	cv::Mat stdImg = meanImg.mul(meanImg);

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

		// compute the standard deviation image
		integral(stdImg, intImg);
		stdImg = Algorithms::instance().convolveIntegralImage(intImg, filtersize, 0, Algorithms::BORDER_ZERO);
		intImg.release(); // early release

		integral(contrastBin32F, intContrastBinary);
		contrastBin32F.release();
		intContrastBinary = Algorithms::instance().convolveIntegralImage(intContrastBinary, filtersize, 0, Algorithms::BORDER_ZERO);
	}
	//DkIP::imwrite("meanImg343.png", meanImg, true);

	meanImg /= intContrastBinary;

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

inline void BaseBinarizationSu::calcFilterParams(int &filterS, int &Nm) const {
	filterS = cvRound(mStrokeW);
	filterS = (filterS % 2) != 1 ? filterS + 1 : filterS;
	Nm = filterS;
}

inline float BaseBinarizationSu::thresholdVal(float *mean, float *std) const {
	return (*mean + *std / 2);
}


QString BaseBinarizationSu::toString() const {

	QString msg = debugName();
	msg += "strokeW: " + QString::number(mStrokeW);

	return msg;
}


bool BinarizationSuAdapted::compute() {

	//computeSuIpk(segImg);

	//if (medianFilter)
	//	medianBlur(segImg, segImg, 3);


	return false;
}

QString BinarizationSuAdapted::toString() const {

	QString msg = debugName();
	msg += "strokeW: " + QString::number(mStrokeW);

	return msg;
}



}