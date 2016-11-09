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

#include "SuperPixelClassification.h"

#include "Utils.h"
#include "Image.h"
#include "Drawer.h"
#include "ImageProcessor.h"

#include "Elements.h"
#include "ElementsHelper.h"

#pragma warning(push, 0)	// no warnings from includes
#include <QDebug>
#include <QPainter>

#include <opencv2/features2d/features2d.hpp>
#pragma warning(pop)

namespace rdf {


// SuperPixelClassificationConfig --------------------------------------------------------------------
SuperPixelClassificationConfig::SuperPixelClassificationConfig() : ModuleConfig("Super Pixel Classification") {
}

QString SuperPixelClassificationConfig::toString() const {
	return ModuleConfig::toString();
}

int SuperPixelClassificationConfig::maxSide() const {
	return mMaxSide;
}

// SuperPixelClassification --------------------------------------------------------------------
SuperPixelClassification::SuperPixelClassification(const cv::Mat& img, const PixelSet& set) {

	mImg = img;
	mSet = set;
	mConfig = QSharedPointer<SuperPixelClassificationConfig>::create();
}

bool SuperPixelClassification::isEmpty() const {
	return mImg.empty();
}

bool SuperPixelClassification::compute() {

	mWarning << "not implemented yet";
	return true;
}

QSharedPointer<SuperPixelClassificationConfig> SuperPixelClassification::config() const {
	return qSharedPointerDynamicCast<SuperPixelClassificationConfig>(mConfig);
}

cv::Mat SuperPixelClassification::draw(const cv::Mat& img) const {

	// draw mser blobs
	Timer dtf;
	QPixmap pm = Image::mat2QPixmap(img);

	QPainter p(&pm);
	
	return Image::qPixmap2Mat(pm);
}

QString SuperPixelClassification::toString() const {
	return Module::toString();
}

bool SuperPixelClassification::checkInput() const {

	return !mImg.empty();
}

// SuperPixelFeatureConfig --------------------------------------------------------------------
SuperPixelFeatureConfig::SuperPixelFeatureConfig() :  ModuleConfig("Super Pixel Feature") {
}

QString SuperPixelFeatureConfig::toString() const{
	return ModuleConfig::toString();
}

// SuperPixelFeature --------------------------------------------------------------------
SuperPixelFeature::SuperPixelFeature(const cv::Mat & img, const PixelSet & set) {
	mImg = img;
	mSet = set;
	mConfig = QSharedPointer<SuperPixelFeatureConfig>::create();
}

bool SuperPixelFeature::isEmpty() const {
	return mImg.empty();
}

bool SuperPixelFeature::compute() {

	if (!checkInput())
		return false;

	Timer dt;
	cv::Mat cImg;
	cImg = IP::grayscale(mImg);

	assert(cImg.type() == CV_8UC1);

	std::vector<cv::KeyPoint> keypoints;
	for (const QSharedPointer<const Pixel>& px : mSet.pixels()) {
		assert(px);
		keypoints.push_back(px->toKeyPoint());
	}

	mInfo << "# keypoints before ORB" << keypoints.size();


	std::vector<cv::KeyPoint> kptsIn(keypoints.begin(), keypoints.end());

	cv::Ptr<cv::ORB> features = cv::ORB::create();
	features->compute(cImg, keypoints, mDescriptors);

	QVector<cv::KeyPoint> kptsOut = QVector<cv::KeyPoint>::fromStdVector(keypoints);


	for (int idx = 0; idx < kptsIn.size(); idx++) {
		
		int kIdx = kptsOut.indexOf(kptsIn[idx]);

		if (kIdx == -1)
			mSet.remove(mSet.pixels()[idx]);
	}

	//qDebug() << "in vs. out" << kptsIn.size() << "o" << keypoints.size();

	//for (const QSharedPointer<Pixel>& px : mSet.pixels()) {

	//	bool exists = false;

	//	for (const cv::KeyPoint& kp : keypoints) {

	//		if (px == kp) {
	//			exists = true;
	//			break;
	//		}
	//	}

	//	if (!exists)
	//		mSet.remove(px);
	//}

	mInfo << "# keypoints after ORB" << keypoints.size() << "set size" << mSet.size();

	//mSet.remove();

	mInfo << mDescriptors.rows << "features computed in" << dt;

	return true;
}

QSharedPointer<SuperPixelFeatureConfig> SuperPixelFeature::config() const {
	return qSharedPointerDynamicCast<SuperPixelFeatureConfig>(mConfig);
}

cv::Mat SuperPixelFeature::draw(const cv::Mat & img) const {
	
	qDebug() << "not implemented...";

	cv::Mat avg;
	cv::reduce(mDescriptors, avg, 0, CV_REDUCE_SUM, CV_32FC1);

	// draw mser blobs
	Timer dtf;
	QPixmap pm = Image::mat2QPixmap(img);

	QPainter p(&pm);

	// draw labeled pixels
	mSet.draw(p);

	Drawer::instance().setColor(ColorManager::getColor(0, 0.3));
	p.setPen(Drawer::instance().pen());

	Histogram hist(avg);
	hist.draw(p, Rect(10, 10, mDescriptors.cols*3, 100));

	return Image::qPixmap2Mat(pm);//Image::qImage2Mat(createLabelImage(Rect(img)));

}

QString SuperPixelFeature::toString() const {
	
	QString str;
	str += config()->name() + " no info to display";
	return str;
}

cv::Mat SuperPixelFeature::features() const {
	return mDescriptors;
}

PixelSet SuperPixelFeature::set() const {
	return mSet;
}

bool SuperPixelFeature::checkInput() const {
	return !mImg.empty();
}

}