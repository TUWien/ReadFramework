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
#include "ImageProcessor.h"
#include "Drawer.h"
#include "Utils.h"

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
	mConfig = QSharedPointer<SuperPixelConfig>::create();
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

	Timer dt;

	cv::Mat img = mSrcImg.clone();
	img = IP::grayscale(img);
	cv::normalize(img, img, 255, 0, cv::NORM_MINMAX);
	//img = IP::invert(img);

	cv::Ptr<cv::MSER> mser = cv::MSER::create();

	std::vector<std::vector<cv::Point> > elements;
	std::vector<cv::Rect> bboxes;
	mser->detectRegions(img, elements, bboxes);

	assert(elements.size() == bboxes.size());
	
	qDebug() << "MSER found" << elements.size() << "regions in" << dt;

	for (int idx = 0; idx < elements.size(); idx++) {
		MserBlob cb(elements[idx], Converter::cvRectToQt(bboxes[idx]));
		mBlobs << cb;
	}

	// compute delauney triangulation
	mTriangles = connect(mBlobs);

	// draw to dst img
	QPixmap pm = Image::instance().mat2QPixmap(img);
	QPainter p(&pm);


	p.setPen(ColorManager::instance().colors()[0]);

	for (auto t : mTriangles)
		t.draw(p);

	//for (auto cb : mBlobs)
	//	cb.draw(p);

	mDstImg = Image::instance().qPixmap2Mat(pm);

	mDebug << "computed in" << dt;

	return true;
}

QVector<Triangle> SuperPixel::connect(const QVector<MserBlob>& blobs) const {

	cv::Rect rect(cv::Point(), mSrcImg.size());

	// Create an instance of Subdiv2D
	cv::Subdiv2D subdiv(rect);

	for (const MserBlob& b : blobs)
		subdiv.insert(b.center().toCvPointF());

	std::vector<cv::Vec6f> triangles;
	subdiv.getTriangleList(triangles);

	// convert to our datatype
	QVector<Triangle> rTriangles;
	for (const cv::Vec6f& t : triangles)
		rTriangles << t;

	return rTriangles;
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

// MserBlob --------------------------------------------------------------------
MserBlob::MserBlob(const std::vector<cv::Point>& pts, const QRectF & bbox) {
	mPts = pts;
	mBBox = bbox;
}

int64 MserBlob::area() const {
	return mPts.size();
}

Vector2D MserBlob::center() const {
	return bbox().center();
}

QRectF MserBlob::bbox() const {
	// TODO: if needed compute it here
	return mBBox;
}

void MserBlob::draw(QPainter & p) {
	
	QColor col = ColorManager::instance().getRandomColor();
	col.setAlpha(30);
	Drawer::instance().setColor(col);
	Drawer::instance().drawPoints(p, mPts);

	// draw bounding box
	col.setAlpha(255);
	Drawer::instance().setStrokeWidth(1);
	Drawer::instance().setColor(col);
	Drawer::instance().drawRect(p, bbox());

	// draw center
	Drawer::instance().setStrokeWidth(3);
	Drawer::instance().drawPoint(p, bbox().center());
}

}