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

#include "ScaleFactory.h"

#include "BaseImageElement.h"

#pragma warning(push, 0)	// no warnings from includes
#include <QSettings>

#include <opencv2/imgproc.hpp>
#pragma warning(pop)

namespace rdf {

// -------------------------------------------------------------------- ScaleFactoryConfig 
ScaleFactoryConfig::ScaleFactoryConfig() : ModuleConfig("Scale Factory") {
}

QString ScaleFactoryConfig::toString() const {
	return ModuleConfig::toString();
}

void ScaleFactoryConfig::setMaxImageSide(int maxSide) {
	mMaxImageSide = maxSide;
}

int ScaleFactoryConfig::maxImageSide() const {
	return ModuleConfig::checkParam(mMaxImageSide, 500, 10000, "maxImageSide");
}

void ScaleFactoryConfig::setScaleMode(const ScaleFactoryConfig::ScaleSideMode & mode) {
	mScaleMode = mode;
}

ScaleFactoryConfig::ScaleSideMode ScaleFactoryConfig::scaleMode() const {
	return mScaleMode;
}

int ScaleFactoryConfig::dpi() const {
	return ModuleConfig::checkParam(mDpi, 1, 3000, "dpi");
}

void ScaleFactoryConfig::load(const QSettings & settings) {

	mScaleMode = (ScaleFactoryConfig::ScaleSideMode)settings.value("scaleMode", scaleMode()).toInt();
	mMaxImageSide = settings.value("maxImageSide", maxImageSide()).toInt();
	mDpi = settings.value("dpi", dpi()).toInt();
}

void ScaleFactoryConfig::save(QSettings & settings) const {

	settings.setValue("scaleMode", scaleMode());
	settings.setValue("maxImageSide", maxImageSide());
	settings.setValue("dpi", dpi());
}

// --------------------------------------------------------------------  ScaleFactory
ScaleFactory::ScaleFactory(const Vector2D& imgSize) {

	mConfig = QSharedPointer<ScaleFactoryConfig>::create();
	mConfig->loadSettings();

	mImgSize = imgSize;
	mScaleFactor = scaleFactor(mImgSize, config()->maxImageSide(), config()->scaleMode());
}

double ScaleFactory::scaleFactor() {


	if (mImgSize.isNull()) {
		qWarning() << "querying scaleFactor() of uninitialized ScaleFactory...";
		return 1.0;
	}

	return mScaleFactor;
}

double ScaleFactory::scaleFactorDpi() {

	// clear dpi changes (parameters are tuned for 300dpi)
	return scaleFactor() * (double)config()->dpi() / 300.0;
}

QSharedPointer<ScaleFactoryConfig> ScaleFactory::config() const {
	return mConfig;
}

void ScaleFactory::setConfig(QSharedPointer<ScaleFactoryConfig> c) {

	mConfig = c;
	mScaleFactor = scaleFactor(mImgSize, config()->maxImageSide(), config()->scaleMode());
}

cv::Mat ScaleFactory::scaled(cv::Mat & img) {

	cv::Mat sImg = img;

	double sf = ScaleFactory::scaleFactor();

	// resize if necessary
	if (sf != 1.0) {
		cv::resize(sImg, sImg, cv::Size(), sf, sf, CV_INTER_LINEAR);
		qInfo() << "image resized, new dimension" << sImg.cols << "x" << sImg.rows << "scale factor:" << sf;
	}

	return sImg;
}

void ScaleFactory::scale(BaseElement & el) {
	el.scale(ScaleFactory::scaleFactor());
}

void ScaleFactory::scaleInv(BaseElement & el) {
	el.scale(1.0 / ScaleFactory::scaleFactor());
}

Vector2D ScaleFactory::imgSize() {
	return mImgSize;
}

double ScaleFactory::scaleFactor(const Vector2D& size, int maxImageSize, const ScaleFactoryConfig::ScaleSideMode& mode) const {


	if (maxImageSize > 0) {

		if (maxImageSize < 500) {
			qWarning() << "you chose the maximal image side to be" << maxImageSize << "px - this is pretty low";
		}

		// find the image side
		double side = 0;
		if (mode == ScaleFactoryConfig::scale_max_side)
			side = qMax(size.x(), size.y());
		else
			side = size.y();

		double sf = (double)maxImageSize / side;

		// do not rescale if the factor is close to 1
		if (sf <= 0.95)
			return sf;

		// inform user that we do not resize if the scale factor is close to 1
		if (sf < 1.0)
			qInfo() << "I won't resize the image since the scale factor is" << sf;
	}

	return 1.0;
}


}