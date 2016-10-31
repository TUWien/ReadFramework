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

	QString GradientVectorConfig::toString() const {
		QString msg;
		msg += "  mSigma: " + QString::number(mSigma);

		return msg;
	}

	void GradientVectorConfig::load(const QSettings & settings) {
		mSigma = settings.value("sigma", mSigma).toDouble();
	}

	void GradientVectorConfig::save(QSettings & settings) const {
		settings.setValue("sigma", mSigma);
	}

	GradientVector::GradientVector(const cv::Mat & img, const cv::Mat & mask)	{
		mSrcImg = img;
		mMask = mask;

		if (mMask.empty()) {
			mMask = cv::Mat(mSrcImg.size(), CV_8UC1, cv::Scalar(255));
		}

		mConfig = QSharedPointer<GradientVectorConfig>::create();
	}

	cv::Mat GradientVector::getDebugGaussImg()	{
		return cv::Mat();
	}

	cv::Mat GradientVector::getMagImg(bool normGrad) 	{
		if (normGrad)
			return cv::Mat();
		else
			return cv::Mat();
	}

	cv::Mat GradientVector::getRadImg()	{
		return cv::Mat();
	}

	bool GradientVector::isEmpty() const	{
		if (mSrcImg.empty()) return true;

		return false;
	}

	bool GradientVector::compute()
	{
		return false;
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

};
