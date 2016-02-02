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

	compContrastImg();
	compBinContrastImg();


	// I guess here is a good point to save the settings
	saveSettings();
	mDebug << " computed...";
	mWarning << "a warning...";
	mInfo << "an info...";

	return true;
}

void BaseBinarizationSu::compContrastImg() {
	
	cv::Mat tmp = cv::Mat(mSrcImg.size(), CV_32FC1);
	cv::Mat srcGray = mSrcImg;
	if (mSrcImg.channels() != 1) cv::cvtColor(mSrcImg, srcGray, CV_RGB2GRAY);
	


}

void BaseBinarizationSu::compBinContrastImg() {

}

QString BaseBinarizationSu::toString() const {

	QString msg = debugName();
	msg += "strokeW: " + QString::number(mStrokeW);

	return msg;
}




}