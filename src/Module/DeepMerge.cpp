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

#include "DeepMerge.h"

#include "Utils.h"
#include "Image.h"
#include "Drawer.h"
#include "GraphCut.h"
#include "ImageProcessor.h"

#pragma warning(push, 0)	// no warnings from includes
#include <QPainter>
#include <opencv2/opencv.hpp>
#pragma warning(pop)

namespace rdf {


// LayoutAnalysisConfig --------------------------------------------------------------------
DeepMergeConfig::DeepMergeConfig() : ModuleConfig("Deep Merge Module") {
}

QString DeepMergeConfig::toString() const {
	return ModuleConfig::toString();
}

QString DeepMergeConfig::labelConfigPath() const {
	return mLabelConfigPath;
}

void DeepMergeConfig::load(const QSettings & settings) {
	
	mLabelConfigPath = settings.value("LabelConfigPath", mLabelConfigPath).toString();
}

void DeepMergeConfig::save(QSettings & settings) const {
	
	settings.setValue("LabelConfigPath", mLabelConfigPath);
}

// LayoutAnalysis --------------------------------------------------------------------
DeepMerge::DeepMerge(const cv::Mat& img) {

	mImg = img;

	mConfig = QSharedPointer<DeepMergeConfig>::create();
	mConfig->loadSettings();
}

bool DeepMerge::isEmpty() const {
	return mImg.empty();
}

bool DeepMerge::compute() {

	if (!checkInput())
		return false;

	mManager = LabelManager::read(config()->labelConfigPath());

	Timer dt;

	cv::Mat img = mImg.clone();

	// norm the image
	cv::cvtColor(img, img, cv::COLOR_RGBA2RGB);
	img.convertTo(img, CV_32F, 1.0 / 255.0);

	cv::Mat mi = maxChannels<float>(img);

	// split the images for input
	std::vector<cv::Mat> ch;
	cv::split(img, ch);

	QVector<cv::Mat> channels = QVector<cv::Mat>::fromStdVector(ch);
	channels.push_front(1.0 - mi);

	DeepCut dc(channels);
	if (!dc.compute())
		qWarning() << "could not compute DeepCut!";

	mLabelImg = dc.image();

	// convert to polygons
	
	if (mManager.labelInfos().size() != channels.size()) {
		qWarning() << "the labels loaded from" << config()->labelConfigPath() << "do not fit the number of labels we have in DeepMerge:" << channels.size();
	}

	for (int idx = 0; idx < channels.size(); idx++) {

		cv::Mat cImg = mLabelImg == idx;

		auto poly = IP::maskToPoly(cImg, mScaleFactor);
		LabelInfo l = mManager.find(idx);

		mRegions << DMRegion(poly, l);
	}

	mInfo << "computed in" << dt;

	return true;
}

cv::Mat DeepMerge::thresh(const cv::Mat& src, double thr) const {

	cv::Mat rImg = src.clone();

	if (rImg.channels() == 4)
		cv::cvtColor(rImg, rImg, cv::COLOR_RGBA2RGB);

	std::vector<cv::Mat> channels;
	cv::split(rImg, channels);

	cv::Mat mxImg = maxChannels<unsigned char>(rImg);

	for (cv::Mat& ch : channels) {
		ch = ch > thr;// &ch == mxImg;
	}

	cv::merge(channels, rImg);

	cv::cvtColor(rImg, rImg, cv::COLOR_RGB2BGR);

	return rImg;
}

QSharedPointer<DeepMergeConfig> DeepMerge::config() const {
	return qSharedPointerDynamicCast<DeepMergeConfig>(mConfig);
}

cv::Mat DeepMerge::draw(const cv::Mat & img, const QColor& col) const {

	QImage qImg = Image::mat2QImage(img, true);
	
	QPainter p(&qImg);
	p.setPen(ColorManager::blue());

	for (auto r : mRegions) {
		
		// don't draw the background
		if (r.label().id() == LabelInfo::label_unknown)
			continue;

		r.draw(p);
	}

	mManager.draw(p);

	return Image::qImage2Mat(qImg);
}

QString DeepMerge::toString() const {
	return Module::toString();
}

cv::Mat DeepMerge::image() const {
	return mLabelImg;
}

void DeepMerge::setScaleFactor(double sf) {
	mScaleFactor = sf;
}

bool DeepMerge::checkInput() const {

	return !isEmpty();
}

// -------------------------------------------------------------------- DMRegion 
DMRegion::DMRegion(const QVector<Polygon>& poly, const LabelInfo& l) {

	mPoly = poly;
	mLabel = l;
}

void DMRegion::setRegions(const QVector<Polygon>& poly) {
	mPoly = poly;
}

QVector<Polygon> DMRegion::regions() const {
	return mPoly;
}

LabelInfo DMRegion::label() const {
	return mLabel;
}

void DMRegion::draw(QPainter & p) {

	QPen pen(mLabel.visColor());
	
	p.setPen(pen);
	p.setBrush(ColorManager::alpha(mLabel.visColor(), 0.3));
	
	for (const Polygon& py : mPoly) {
		py.draw(p);
	}
}

}