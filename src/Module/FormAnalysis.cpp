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

#include "FormAnalysis.h"
#include "Binarization.h"
#include "SkewEstimation.h"


#pragma warning(push, 0)	// no warnings from includes
#include <QDebug>

#include "opencv2/imgproc/imgproc.hpp"
#pragma warning(pop)

namespace rdf {
	FormFeatures::FormFeatures()
	{
	}
	FormFeatures::FormFeatures(const cv::Mat & img, const cv::Mat & mask)
	{
		mSrcImg = img;
		mMask = mask;
	}
	void FormFeatures::setInputImg(const cv::Mat & img)
	{
		mSrcImg = img;
	}
	void FormFeatures::setMask(const cv::Mat & mask)
	{
		mMask = mask;
	}
	bool FormFeatures::isEmpty() const
	{
		return mSrcImg.empty();
	}
	bool FormFeatures::compute()
	{
		if (!checkInput())
			return false;

		if (mBwImg.empty()) {
			if (!computeBinaryInput())
				return false;

			mWarning << "binary image was not set - was calculated";
		}

		if (mEstimateSkew) {
			BaseSkewEstimation skewE(mSrcImg, mMask);
			skewE.compute();
			mPageAngle = skewE.getAngle();
		}

		//compute Lines
		LineTrace lt(mBwImg, mMask);
		if (mEstimateSkew) {
			lt.setAngle(mPageAngle);
		}
		lt.compute();

		mHorLines = lt.getHLines();
		mVerLines = lt.getVLines();

		return true;
	}

	bool FormFeatures::computeBinaryInput()
	{
		if (mSrcImg.empty() || mSrcImg.depth() != CV_8U) {
			mWarning << "image is empty or illegal image depth: " << mSrcImg.depth();
			return false;
		}

		if (!mMask.empty() && mMask.depth() != CV_8U && mMask.channels() != 1) {
			mWarning << "illegal image depth or channel for mask: " << mMask.depth();
			return false;
		}

		BinarizationSuAdapted binarizeImg(mSrcImg, mMask);
		binarizeImg.compute();
		mBwImg = binarizeImg.binaryImage();

		return true;
	}

	bool FormFeatures::compareWithTemplate(const FormFeatures & fTempl)
	{
		std::sort(mHorLines.begin(), mHorLines.end(), rdf::Line::leqY1);
		std::sort(mVerLines.begin(), mVerLines.end(), rdf::Line::leqX1);

		QVector<rdf::Line> horLinesTemp = fTempl.horLines();
		QVector<rdf::Line> verLinesTemp = fTempl.verLines();

		std::sort(horLinesTemp.begin(), horLinesTemp.end(), rdf::Line::leqY1);
		std::sort(verLinesTemp.begin(), verLinesTemp.end(), rdf::Line::leqX1);

		float hLen = 0, hLenTemp = 0;
		float vLen = 0, vLenTemp = 0;
		
		//calculate entire horizontal and vertical line lengths of the detected lines
		//of the current document
		for (auto i : mHorLines) {
			hLen += i.length();
		}
		for (auto i : mVerLines) {
			vLen += i.length();
		}
		//calculate entire horizontal and vertical line lengths of the template
		for (auto i : horLinesTemp) {
			hLenTemp += i.length();
		}
		for (auto i : verLinesTemp) {
			vLenTemp += i.length();
		}

		//float ratioHor = hLen < hLenTemp ? hLen / hLenTemp : hLenTemp / hLen;
		//float ratioVer = vLen < vLenTemp ? vLen / vLenTemp : vLenTemp / vLen;

		////at least mThreshLineLenRatio (default: 60%) of the lines must be detected in the current document
		//if (ratioHor < mThreshLineLenRatio || ratioVer < mThreshLineLenRatio)
		//	return false;

		//int refY = horLinesTemp[0].startPoint().y();
		//float distance = 0.0f;
		//for (int i = 0; i < mHorLines.size(); i++) {
		//	
		//	int mapDiffY = mHorLines[i].startPoint().y() - refY;
		//	for (int j = 0; j < verLinesTemp.size(); j++) {

		//	}

		//}


		return false;
	}

	QVector<rdf::Line> FormFeatures::horLines() const
	{
		return mHorLines;
	}

	QVector<rdf::Line> FormFeatures::verLines() const
	{
		return mVerLines;
	}

	cv::Mat FormFeatures::binaryImage() const
	{
		return mBwImg;
	}

	void FormFeatures::setEstimateSkew(bool s)
	{
		mEstimateSkew = s;
	}

	void FormFeatures::setThreshLineLenRatio(float l)
	{
		mThreshLineLenRatio = l;
	}

	QString FormFeatures::toString() const
	{
		return QString("Form Features class calculates line and layout features for form classification");
	}

	bool FormFeatures::checkInput() const
	{
		if (mSrcImg.empty())
			return false;

		if (mSrcImg.depth() != CV_8U) {
			mSrcImg.convertTo(mSrcImg, CV_8U, 255);
			mWarning << "Input image was not CV_8U - has been converted";
		}

		if (mSrcImg.channels() != 1) {
			cv::cvtColor(mSrcImg, mSrcImg, CV_RGB2GRAY);

			mWarning << "Input image was a color image - has been converted tpo grayscale";
		}

		return true;
	}
}