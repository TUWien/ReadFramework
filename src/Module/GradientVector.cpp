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

#include "GradientVector.h"
#include "Image.h"
#include "Algorithms.h"

#pragma warning(push, 0)	// no warnings from includes
// Qt Includes
#include <QDebug>
#include <QSettings>
#include <qmath.h>
#include "opencv2/imgproc/imgproc.hpp"
#pragma warning(pop)



namespace rdf {

	GradientVectorConfig::GradientVectorConfig() {
		mModuleName = "GradientVector";
	}

	double GradientVectorConfig::sigma() const {
		return mSigma;
	}

	void GradientVectorConfig::setSigma(double s) {
		mSigma = s;
	}

	bool GradientVectorConfig::normGrad() const	{
		return mNormGrad;
	}

	void GradientVectorConfig::setNormGrad(bool n)	{
		mNormGrad = n;
	}

	bool GradientVectorConfig::perpendAngle() const	{
		return mPerpendAngle;
	}

	void GradientVectorConfig::setPerpendAngle(bool p) 	{
		mPerpendAngle = p;
	}



	QString GradientVectorConfig::toString() const {
		QString msg;
		msg += "  mSigma: " + QString::number(mSigma);
		return msg;
	}

	void GradientVectorConfig::load(const QSettings & settings) {
		mSigma = settings.value("sigma", mSigma).toDouble();
		mNormGrad = settings.value("normGrad", mNormGrad).toBool();
		mPerpendAngle = settings.value("perpendAngle", mPerpendAngle).toBool();
	}

	void GradientVectorConfig::save(QSettings & settings) const {
		settings.setValue("sigma", mSigma);
		settings.setValue("normGrad", mNormGrad);
		settings.setValue("perpendAngle", mPerpendAngle);
	}

	GradientVector::GradientVector(const cv::Mat & img, const cv::Mat & mask)	{
		mSrcImg = img;
		mMask = mask;

		if (mMask.empty()) {
			mMask = cv::Mat(mSrcImg.size(), CV_8UC1, cv::Scalar(255));
			//rdf::Image::imageInfo(mMask, "mMask");
		}

		mConfig = QSharedPointer<GradientVectorConfig>::create();
	}

	cv::Mat GradientVector::debugGaussImg()	{
		return mGaussImg;
	}

	cv::Mat GradientVector::magImg() 	{
		return mMagImg;
	}

	cv::Mat GradientVector::radImg()	{
		return mRadImg;
	}

	cv::Mat GradientVector::mask()	{
		return mMask;
	}

	void GradientVector::setAnchor(cv::Point a)	{
		mAnchor = a;
	}

	cv::Point GradientVector::anchor() const	{
		return mAnchor;
	}

	void GradientVector::setDxKernel(const cv::Mat & m)	{
		mDxKernel = m;
	}

	void GradientVector::setDyKernel(const cv::Mat & m)	{
		mDyKernel = m;
	}

	double GradientVector::minVal() const	{
		return mMinVal;
	}

	double GradientVector::maxVal() const	{
		return mMaxVal;
	}

	bool GradientVector::isEmpty() const	{
		if (mSrcImg.empty()) return true;

		return false;
	}

	bool GradientVector::compute()
	{
		if (!checkInput())
			return false;

		computeGradients();

		//denormalize gradient image?
		computeGradMag(config()->normGrad());
		//rad img
		computeGradAngle();

		return true;
	}

	QSharedPointer<GradientVectorConfig> GradientVector::config() const	{
		return qSharedPointerDynamicCast<GradientVectorConfig>(mConfig);
	}

	QString GradientVector::toString() const {
		QString msg = debugName();
		//msg += "number of detected horizontal lines: " + QString::number(hLines.size());
		//msg += "number of detected vertical lines: " + QString::number(vLines.size());
		msg += config()->toString();

		return msg;
	}

	bool GradientVector::checkInput() const {
		if (mSrcImg.empty()) {
			mWarning << "Source Image is empty";
			return false;
		}

		return true;
	}

	void GradientVector::computeGradients()	{

		//commented -> max and min after gauss
		// get the minimum & maximum for de-normalization
		minMaxLoc(mSrcImg, &mMinVal, &mMaxVal, 0, 0, mMask);

		// compute gaussian image
		if (mGaussImg.empty() && config()->sigma() > 1 / 6.0f) {

			if (mSrcImg.depth() == CV_8U)
				mSrcImg.convertTo(mGaussImg, CV_64F, 1 / 255.0);
			else if (mSrcImg.depth() == CV_32F) {
				mSrcImg.convertTo(mGaussImg, CV_64F, 1);
			}
			else
				mGaussImg = mSrcImg.clone();

			cv::normalize(mGaussImg, mGaussImg, 1, 0, cv::NORM_MINMAX);

			int kSize = cvRound(cvCeil(config()->sigma() * 3) * 2 + 1);
			GaussianBlur(mGaussImg, mGaussImg, cvSize(kSize, kSize), config()->sigma());
		} else if (mSrcImg.depth() == CV_8U) {
			mSrcImg.convertTo(mGaussImg, CV_64F, 1 / 255.0);
			cv::normalize(mGaussImg, mGaussImg, 1, 0, cv::NORM_MINMAX);
		} else if (mSrcImg.depth() == CV_32F) {
			mSrcImg.convertTo(mGaussImg, CV_64F, 1);
			cv::normalize(mGaussImg, mGaussImg, 1, 0, cv::NORM_MINMAX);
		} else {
			mGaussImg = mSrcImg.clone();
			cv::normalize(mGaussImg, mGaussImg, 1, 0, cv::NORM_MINMAX);
		}

		//rdf::Image::imageInfo(mGaussImg, "mGaussImg");

		//double diffData[] = { 1, 0, -1 };
		//cv::Mat dxKernel = cv::Mat(1, 3, CV_64F, &diffData);
		//cv::Mat dyKernel = cv::Mat(3, 1, CV_64F, &diffData);
		double diffData[] = { 1, 0, -1 };
		if (mDxKernel.empty())
			mDxKernel = cv::Mat(1, 3, CV_64F, &diffData);
		if (mDyKernel.empty())
			mDyKernel = cv::Mat(3, 1, CV_64F, &diffData);

		filter2D(mGaussImg, mDxImg, -1, mDxKernel, mAnchor);	// -1: dst.depth == src.depth
		filter2D(mGaussImg, mDyImg, -1, mDyKernel, mAnchor);

		
		//rdf::Image::imageInfo(mDxKernel, "mDxKernel");
		//rdf::Image::imageInfo(mDyKernel, "mDyKernel");

		//rdf::Image::imageInfo(mDxImg, "mDxImg");
		//rdf::Image::imageInfo(mDyImg, "mDyImg");

		////TODO: implement old mask functions?
		// remove borders if the mask was assigned
		if (!mMask.empty()) {
			cv::Mat maskEr;
			//	//if (imgs == 0)
			//	//	maskEr = DkIP::fastErodeImage(mask, cvRound(sigma*12.0f));
			//	//else
			//	//	maskEr = mask;
			maskEr = rdf::Algorithms::erodeImage(mMask, (int)(config()->sigma()*12.0f),rdf::Algorithms::SQUARE, 0);
			//mDxImg = mDxImg.mul(mDxImg);
			//mDyImg = mDyImg.mul(mDyImg);
			rdf::Algorithms::mulMask(mDxImg, maskEr);
			rdf::Algorithms::mulMask(mDyImg, maskEr);
			mMask = maskEr;
		}
	}

	void GradientVector::computeGradMag(bool norm) 	{

		//denormalize gradient image
		if (!norm) {
			mDxImg = mDxImg*(double)(mMaxVal - mMinVal);
			mDyImg = mDyImg*(double)(mMaxVal - mMinVal);
		}

		//rdf::Image::imageInfo(mDxImg, "mDxImg");
		//rdf::Image::imageInfo(mDyImg, "mDyImg");

		// compute gradient magnitude
		mMagImg = cv::Mat(mDxImg.size(), mDxImg.type());
		int cols = mMagImg.cols;
		int rows = mMagImg.rows;

		// speed up for accessing elements
		if (mMagImg.isContinuous()) {
			cols *= rows;
			rows = 1;
		}

		for (int rIdx = 0; rIdx < rows; rIdx++) {

			const double* dxPtr = mDxImg.ptr<double>(rIdx);
			const double* dyPtr = mDyImg.ptr<double>(rIdx);
			double* magPtr = mMagImg.ptr<double>(rIdx);

			for (int cIdx = 0; cIdx < cols; cIdx++) {

				magPtr[cIdx] = dxPtr[cIdx] * dxPtr[cIdx] + dyPtr[cIdx] * dyPtr[cIdx];		// gradient magnitude: sqrt(dx^2 + dy^2)
																							//magPtr[cIdx] = sqrt(dxPtr[cIdx]*dxPtr[cIdx] + dyPtr[cIdx]*dyPtr[cIdx]);			// gradient magnitude: sqrt(dx^2 + dy^2)
																							//magPtr[cIdx] = DkMath::fastSqrt(dxPtr[cIdx]*dxPtr[cIdx] + dyPtr[cIdx]*dyPtr[cIdx]);		// gradient magnitude: sqrt(dx^2 + dy^2)
			}
		}

		cv::sqrt(mMagImg, mMagImg);
		
		//cv::normalize(mDxImg, mDxImg, 255, 0, cv::NORM_MINMAX);
		//mDxImg.convertTo(mDxImg, CV_8U);
		//Image::save(mDxImg, "D:\\tmp\\dbg.png");
	}

	void GradientVector::computeGradAngle()  {
		// compute gradient orientation image
		mRadImg = cv::Mat(mDxImg.size(), mDxImg.type());
		int cols =mRadImg.cols;
		int rows = mRadImg.rows;
		double angle = 0;

		// speed up for accessing elements
		if (mRadImg.isContinuous()) {
			cols *= rows;
			rows = 1;
		}

		if (config()->perpendAngle()) qDebug() << "WARNING: gradient orientation is perpendicular according setting...";

		for (int rIdx = 0; rIdx < rows; rIdx++) {

			const double* dxPtr = mDxImg.ptr<double>(rIdx);
			const double* dyPtr = mDyImg.ptr<double>(rIdx);
			double* radPtr = mRadImg.ptr<double>(rIdx);

			for (int cIdx = 0; cIdx < cols; cIdx++) {

				if (config()->perpendAngle()) {
					angle = atan2(dxPtr[cIdx],-dyPtr[cIdx]);
				} else {
					angle = atan2(dyPtr[cIdx], dxPtr[cIdx]);
				}
				
				//angle = atan2(dxPtr[cIdx], -dyPtr[cIdx]);
				//result of atan2 is -pi to +pi
				//normangleRad -> 0 to 2pi
				radPtr[cIdx] = rdf::Algorithms::normAngleRad(angle);
				//radPtr[cIdx] = angle;
			}
		}
	}

};
