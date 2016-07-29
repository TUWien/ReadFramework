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

#include "SuperPixel.h"

#include "Image.h"
#include "Drawer.h"

#pragma warning(push, 0)	// no warnings from includes
#include <QDebug>

//#include <opencv2/stitching/detail/seam_finders.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d/features2d.hpp>
#pragma warning(pop)

namespace rdf {


// SuperPixel --------------------------------------------------------------------
SuperPixel::SuperPixel(const cv::Mat& srcImg) {
	mSrcImg = srcImg;
}

bool SuperPixel::checkInput() const {

	if (mSrcImg.empty()) {
		mWarning << "the source image must not be empty...";
		return false;
	}

	return true;
}

bool SuperPixel::isEmpty() const {
	return mSrcImg.empty();
}

bool SuperPixel::compute() {

	if (!checkInput())
		return false;

	cv::Ptr<cv::MSER> mser = cv::MSER::create();

	std::vector<std::vector<cv::Point> > centers;
	std::vector<cv::Rect> bboxes;
	mser->detectRegions(mSrcImg, centers, bboxes);

	mDstImg = mSrcImg.clone();

	// TODO: put into a drawer class...
	for (auto set : centers) {
		Drawer::instance().setColor(Drawer::instance().getRandomColor());
		mDstImg = Drawer::instance().drawPoints(mDstImg, set);
	}

	//Drawer::instance().setColor(Drawer::instance().getRandomColor());
	//mDstImg = Drawer::instance().drawRects(mDstImg, bboxes);

	//cv::detail::GraphCutSeamFinder seamFinder;

	//std::vector<cv::Point> corners;
	//corners.push_back(cv::Point(0, 0));
	//corners.push_back(cv::Point(100, 100));

	//cv::UMat src;
	//cv::cvtColor(mSrcImg, src, CV_RGBA2RGB);

	//std::vector<cv::UMat> mats;
	//mats.push_back(src);
	//mats.push_back(src);

	//mMask.push_back(cv::UMat(src.size(), CV_8UC1, cv::Scalar(255)));
	//mMask.push_back(cv::UMat(src.size(), CV_8UC1, cv::Scalar(255)));

	//qDebug() << "src channels: " << mSrcImg.channels();

	//seamFinder.find(mats, corners, mMask);

	//qDebug() << "mMask size: " << mMask.size();
	mDebug << "computed...";

	return true;
}

cv::Mat SuperPixel::binaryImage() const {
	
	return mDstImg;
}

QString SuperPixel::toString() const {

	QString msg = debugName();

	return msg;
}

// SuperPixelConfig --------------------------------------------------------------------
SuperPixelConfig::SuperPixelConfig() : ModuleConfig("Super Pixel") {
}

QString SuperPixelConfig::toString() const {
	return ModuleConfig::toString();
}

void SuperPixelConfig::load(const QSettings & /*settings*/) {

	// add parameters
	//mThresh = settings.value("thresh", mThresh).toInt();

}

void SuperPixelConfig::save(QSettings & /*settings*/) const {

	// add parameters
	//settings.setValue("thresh", mThresh);
}

}