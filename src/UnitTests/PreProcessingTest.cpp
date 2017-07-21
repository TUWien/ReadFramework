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

#include "PreProcessingTest.h"

#include "Binarization.h"		// tested
#include "SkewEstimation.h"		// tested

#include "Image.h"
#include "Utils.h"
#include "Settings.h"
#include "ImageProcessor.h"

#pragma warning(push, 0)	// no warnings from includes
#include <QFileInfo>
#include <QDir>
#pragma warning(pop)

namespace rdf {
PreProcessingTest::PreProcessingTest(const TestConfig & config) : mConfig(config) {
}

/// <summary>
/// Test different binarization methods.
/// </summary>
/// <returns></returns>
bool PreProcessingTest::binarize() const {

	cv::Mat img;
	if (!load(img))
		return false;

	rdf::BaseBinarizationSu bs(img);
	
	if (!bs.compute())
		return false;

	rdf::BinarizationSuAdapted bsa(img);

	if (!bsa.compute())
		return false;

	rdf::BinarizationSuFgdWeight bsf(img);

	if (!bsf.compute())
		return false;

	rdf::SimpleBinarization bss(img);

	if (!bss.compute())
		return false;

	// -------------------------------------------------------------------- drawing
	QString fn = QFileInfo(mConfig.imagePath()).baseName() + ".jpg";
	QString dstPath = QFileInfo(rdf::Config::global().workingDir(), fn).absoluteFilePath();

	cv::Mat rImg = bs.binaryImage();
	rdf::Image::save(rImg, rdf::Utils::createFilePath(dstPath, "-su"));

	rImg = bsa.binaryImage();
	rdf::Image::save(rImg, rdf::Utils::createFilePath(dstPath, "-su-adapted"));

	rImg = bsf.binaryImage();
	rdf::Image::save(rImg, rdf::Utils::createFilePath(dstPath, "-su-fgd"));

	rImg = bss.binaryImage();
	rdf::Image::save(rImg, rdf::Utils::createFilePath(dstPath, "-bw-simple"));

	qDebug() << "binarization written to" << dstPath;


	return true;
}

/// <summary>
/// Test different skew estimations.
/// </summary>
/// <returns></returns>
bool PreProcessingTest::skew() const {

	cv::Mat img;
	if (!load(img))
		return false;

	// -------------------------------------------------------------------- native skew 
	double nativeAngle = 0;
	if (!nativeSkew(img, nativeAngle)) {
		qWarning() << "could not compute native skew";
		return false;
	}

	// -------------------------------------------------------------------- local orientation based 
	rdf::TextLineSkew tls(img);

	if (!tls.compute()) {
		qWarning() << "could not compute text-line based skew estimation";
		return false;
	}
	
	// -------------------------------------------------------------------- drawing
	QString fn = QFileInfo(mConfig.imagePath()).baseName() + ".jpg";
	QString dstPath = QFileInfo(rdf::Config::global().workingDir(), fn).absoluteFilePath();

	cv::Mat rImg = rdf::IP::rotateImage(img, nativeAngle);
	rdf::Image::save(rImg, rdf::Utils::createFilePath(dstPath, "-native"));

	rImg = tls.rotated(img);
	rdf::Image::save(rImg, rdf::Utils::createFilePath(dstPath, "-tls"));

	rImg = tls.draw(img);
	rdf::Image::save(rImg, rdf::Utils::createFilePath(dstPath, "-tls-debug"));

	qDebug() << "rotated images written to" << dstPath;

	return true;
}

bool PreProcessingTest::load(cv::Mat& img) const {

	QImage qImg = Image::load(mConfig.imagePath());

	if (qImg.isNull()) {
		qWarning() << "could not load image from" << mConfig.imagePath();
		return false;
	}

	// convert image
	img = Image::qImage2Mat(qImg);

	return true;
}

bool PreProcessingTest::nativeSkew(const cv::Mat & img, double & angle) const {

	rdf::BaseSkewEstimation bse;

	bse.setImages(img);
	QSharedPointer<rdf::BaseSkewEstimationConfig> cf = bse.config();

	cf->setWidth(qRound(img.cols / 1430.0*49.0)); //check  (nomacs plugin version)
	cf->setHeight(qRound(img.rows / 700.0*12.0)); //check (nomacs plugin version)
	cf->setDelta(qRound(img.cols / 1430.0*20.0)); //check (nomacs plugin version)
	cf->setMinLineLength(qRound(img.cols / 1430.0 * 20.0)); //check
	cf->setThr(0.1);
	bse.setFixedThr(false);

	if (!bse.compute()) {
		qDebug() << "could not compute skew";
		return false;
	}

	angle = bse.getAngle();
	angle = -angle / 180.0 * CV_PI;

	return true;
}

}