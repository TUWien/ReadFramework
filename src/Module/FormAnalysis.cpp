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
#include <QSettings>

#include "opencv2/imgproc/imgproc.hpp"
#pragma warning(pop)

namespace rdf {
	FormFeatures::FormFeatures(){
		mConfig = QSharedPointer<FormFeaturesConfig>::create();
	}
	FormFeatures::FormFeatures(const cv::Mat & img, const cv::Mat & mask)
	{
		mSrcImg = img;
		mMask = mask;

		mSizeSrc = mSrcImg.size();

		mConfig = QSharedPointer<FormFeaturesConfig>::create();
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

		cv::Mat lineTempl(mSizeSrc, CV_32FC1);
		LineTrace::generateLineImage(horLinesTemp, verLinesTemp, lineTempl);
		cv::distanceTransform(lineTempl, lineTempl, CV_DIST_L1, CV_DIST_MASK_3); //cityblock

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

		float ratioHor = hLen < hLenTemp ? hLen / hLenTemp : hLenTemp / hLen;
		float ratioVer = vLen < vLenTemp ? vLen / vLenTemp : vLenTemp / vLen;

		//at least mThreshLineLenRatio (default: 60%) of the lines must be detected in the current document
		if (ratioHor < config()->threshLineLenRation() || ratioVer < config()->threshLineLenRation()) {
			qDebug() << "form rejected: less lines as specified in the threshold (threshLineLenRatio)";
			return false;
		}

		QVector<int> offsetsX;
		QVector<int> offsetsY;
		//mSrcImg.rows
		int sizeDiffY = std::abs(mSizeSrc.height - fTempl.sizeImg().height);
		int sizeDiffX = std::abs(mSizeSrc.width - fTempl.sizeImg().width);

		findOffsets(horLinesTemp, verLinesTemp, offsetsX, offsetsY);

		float horizontalError = 0;
		float verticalError = 0;
		cv::Point offSet(0, 0);
		float minError = std::numeric_limits<float>::max();

		for (int iX = 0; iX <= offsetsX.size(); iX++) {
			for (int iY = 0; iY <= offsetsY.size(); iY++) {
				//for for maximal translation
				if (offsetsX[iX] <= sizeDiffX && offsetsY[iY] <= sizeDiffY) {

					for (int i = 0; i < mHorLines.size(); i++) {
						float tmp = errLine(lineTempl, mHorLines[i], cv::Point(offsetsX[iX], offsetsY[iY]));
						horizontalError += tmp < std::numeric_limits<float>::max() ? tmp : 0;
					}
					for (int i = 0; i < mVerLines.size(); i++) {
						float tmp = errLine(lineTempl, mVerLines[i], cv::Point(offsetsX[iX], offsetsY[iY]));
						verticalError += tmp < std::numeric_limits<float>::max() ? tmp : 0;
					}
					float error = horizontalError + verticalError;
					if (error <= minError) {
						minError = error;
						offSet.x = offsetsX[iX];
						offSet.y = offsetsY[iY];
					}
				}

			}
		}

		qDebug() << "current Error: " << minError;
		qDebug() << "current offSet: " << offSet.x << " " << offSet.y;

		return true;
	}

	cv::Size FormFeatures::sizeImg() const
	{
		return mSizeSrc;
	}

	QVector<rdf::Line> FormFeatures::horLines() const
	{
		return mHorLines;
	}

	QVector<rdf::Line> FormFeatures::verLines() const
	{
		return mVerLines;
	}

	QSharedPointer<FormFeaturesConfig> FormFeatures::config() const	{
		return qSharedPointerDynamicCast<FormFeaturesConfig>(mConfig);
	}

	cv::Mat FormFeatures::binaryImage() const
	{
		return mBwImg;
	}

	void FormFeatures::setEstimateSkew(bool s)
	{
		mEstimateSkew = s;
	}

	//void FormFeatures::setThreshLineLenRatio(float l)
	//{
	//	mThreshLineLenRatio = l;
	//}

	QString FormFeatures::toString() const
	{
		return QString("Form Features class calculates line and layout features for form classification");
	}

	float FormFeatures::errLine(const cv::Mat & distImg, const rdf::Line l, cv::Point offset)
	{

		cv::LineIterator it(mSrcImg, l.startPointCV(), l.endPointCV());
		float distance = 0;
		float outsidePixel = 0;
		float max = 0;

		for (int i = 0; i < it.count; i++, ++it) {

			cv::Point pos = it.pos();
			pos = pos + offset;

			if (pos.x < 0 || pos.y < 0 || pos.x >= distImg.cols || pos.y >= distImg.rows) {
				//we are outside the image
				outsidePixel++;
			}
			else {
				float dist = distImg.at<float>(pos.y, pos.x);
				distance += dist;
				max = dist > max ? dist : max;
			}
		}

		distance += (max*outsidePixel);
		if (distance == 0)
			distance = std::numeric_limits<float>::max();

		return distance;
	}

	void FormFeatures::findOffsets(const QVector<Line>& hT, const QVector<Line>& vT, QVector<int>& offX, QVector<int>& offY) const	{

		offY.clear();
		offY.push_back(0);
		offX.clear();
		offX.push_back(0);

		//find vertical offsets
		if (!hT.empty() && !mHorLines.empty()) {
			//use Y difference of horizontal lines if template and current Image contains horizontal lines
			for (int i = 0; i < hT.size(); i++) {
				int yLineTemp = hT[i].startPointCV().y;
				for (int j = 0; j < mHorLines.size(); j++) {
					int diffYLine = yLineTemp - mHorLines[j].startPointCV().y;
					offY.push_back(diffYLine);
				}
			}
		}
		else {
			//use Y difference of starting point of vertical lines if template or current image contains no horizontal lines
			for (int i = 0; i < vT.size(); i++) {
				int yLineTemp = vT[i].startPointCV().y;
				for (int j = 0; j < mVerLines.size(); j++) {
					int diffYLine = yLineTemp - mVerLines[j].startPointCV().y;
					offY.push_back(diffYLine);
				}
			}
		}
		


		//find horizontal offsets
		if (!vT.empty() && !mVerLines.empty()) {
			//use Y difference of horizontal lines if template and current Image contains horizontal lines
			for (int i = 0; i < vT.size(); i++) {
				int xLineTemp = vT[i].startPointCV().x;
				for (int j = 0; j < mVerLines.size(); j++) {
					int diffXLine = xLineTemp - mVerLines[j].startPointCV().x;
					offY.push_back(diffXLine);
				}
			}
		}
		else {
			//use Y difference of starting point of vertical lines if template or current image contains no horizontal lines
			for (int i = 0; i < hT.size(); i++) {
				int xLineTemp = hT[i].startPointCV().x;
				for (int j = 0; j < mHorLines.size(); j++) {
					int diffXLine = xLineTemp - mHorLines[j].startPointCV().x;
					offX.push_back(diffXLine);
				}
			}
		}

	}

	bool FormFeatures::checkInput() const	{
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
	FormFeaturesConfig::FormFeaturesConfig() {
		mModuleName = "FormFeatures";
	}
	float FormFeaturesConfig::threshLineLenRation() const	{
		return mThreshLineLenRatio;
	}
	void FormFeaturesConfig::setThreshLineLenRation(float s)	{
		mThreshLineLenRatio = s;
	}
	QString FormFeaturesConfig::toString() const	{
		QString msg;
		msg += "  mThreshLineLenRatio: " + QString::number(mThreshLineLenRatio);

		return msg;
	}
	void FormFeaturesConfig::load(const QSettings & settings)	{
		mThreshLineLenRatio = settings.value("threshLineLenRatio", mThreshLineLenRatio).toFloat();
	}
	void FormFeaturesConfig::save(QSettings & settings) const	{
		settings.setValue("threshLineLenRatio", mThreshLineLenRatio);
	}
}