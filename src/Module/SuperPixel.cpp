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

QVector<MserBlob> SuperPixel::getBlobs(const cv::Mat & img, int kernelSize) const {

	if (kernelSize > 0) {
		cv::Size kSize(kernelSize, kernelSize);
		cv::Mat k = cv::getStructuringElement(cv::MORPH_ELLIPSE,
			cv::Size(2 * kSize.width + 1, 2 * kSize.height + 1),
			cv::Point(kSize.width, kSize.height));

		// NOTE: dilate, erode is conter-intuitive because ink is usually dark
		cv::dilate(img, img, k);
		cv::erode(img, img, k);
	}

	return mser(img);
}

QVector<MserBlob> SuperPixel::mser(const cv::Mat & img) const {
	
	cv::Ptr<cv::MSER> mser = cv::MSER::create();
	mser->setMinArea(mMinArea);

	std::vector<std::vector<cv::Point> > pixels;
	std::vector<cv::Rect> boxes;
	mser->detectRegions(img, pixels, boxes);

	assert(pixels.size() == boxes.size());

	int nF = filterAspectRatio(pixels, boxes);
	qDebug() << nF << "filtered (aspect ratio)";

	// collect
	QVector<MserBlob> blobs;
	for (int idx = 0; idx < pixels.size(); idx++) {
		MserBlob cb(pixels[idx], Converter::cvRectToQt(boxes[idx]));
		blobs << cb;
	}

	return blobs;
}

int SuperPixel::filterAspectRatio(std::vector<std::vector<cv::Point>>& pixels, std::vector<cv::Rect>& boxes, double aRatio) const {

	assert(pixels.size() == boxes.size());

	// filter w.r.t aspect ratio
	std::vector<std::vector<cv::Point> > elementsClean;
	std::vector<cv::Rect> boxesClean;

	for (int idx = 0; idx < pixels.size(); idx++) {

		cv::Rect b = boxes[idx];
		double cARatio = (double)qMin(b.width, b.height) / qMax(b.width, b.height);

		if (cARatio > aRatio) {
			boxesClean.push_back(b);
			elementsClean.push_back(pixels[idx]);
		}
	}

	int numRemoved = (int)(pixels.size() - elementsClean.size());

	pixels = elementsClean;
	boxes = boxesClean;

	return numRemoved;
}

int SuperPixel::filterDuplicates(QVector<MserBlob>& blobs) const {

	QVector<MserBlob> blobsClean;

	for (const MserBlob& b : blobs) {

		bool isNew = true;
		for (const MserBlob& cb : blobsClean) {
						
			if (b.center() == cb.center()) {
				isNew = false;
				break;
			}
		}

		if (isNew)
			blobsClean << b;

	}
	
	int numRemoved = blobs.size() - blobsClean.size();

	blobs = blobsClean;

	return numRemoved;
}

int SuperPixel::filterUnique(QVector<MserBlob>& blobs, double areaRatio) const {

	QVector<MserBlob> blobsClean;

	for (const MserBlob& b : blobs) {

		double ua = b.uniqueArea(blobs);
		if (ua / b.area() > areaRatio)
			blobsClean << b;
	}

	int numRemoved = blobs.size() - blobsClean.size();

	blobs = blobsClean;

	return numRemoved;
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

	for (int idx = 0; idx < 5; idx += 2) {

		Timer dti;
		QVector<MserBlob> b = getBlobs(img, idx);
		mBlobs.append(b);
		qDebug() << b.size() << "/" << mBlobs.size() << "collected with kernel size" << idx << "in" << dti;
	}

	Timer dtf;
	int nF = filterDuplicates(mBlobs);
	qDebug() << nF << "filtered (duplicates) in" << dtf;

	//dtf.start();
	//nF = filterUnique(mBlobs, 0.001);
	//qDebug() << nF << "filtered (unique area) in" << dtf;

	// compute delauney triangulation
	dtf.start();
	mTriangles = connect(mBlobs);
	qDebug() << "delauney computed in " << dtf;

	// draw to dst img
	dtf.start();
	QPixmap pm = Image::instance().mat2QPixmap(img);
	QPainter p(&pm);

	p.setPen(ColorManager::instance().colors()[0]);

	for (auto t : mTriangles)
		t.draw(p);

	//for (auto cb : mBlobs)
	//	cb.draw(p);

	mDstImg = Image::instance().qPixmap2Mat(pm);
	qDebug() << "drawing takes" << dtf;

	mDebug << mBlobs.size() << "regions computed in" << dt;

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
	mCenter = bbox.center();	// cache center
}

double MserBlob::area() const {
	return (double)mPts.size();
}

double MserBlob::uniqueArea(const QVector<MserBlob>& blobs) const {
	
	cv::Mat m(mBBox.size().toCvSize(), CV_8UC1);
	IP::draw(relativePts(bbox().topLeft()), m, cv::Scalar::all(1));

	for (const MserBlob& b : blobs) {

		// remove pixels
		if (bbox().contains(b.bbox())) {
			IP::draw(b.relativePts(bbox().topLeft()), m, cv::Scalar::all(0));
		}
	}

	return cv::sum(m)[0];
}

Vector2D MserBlob::center() const {
	return mCenter;
}

Rect MserBlob::bbox() const {
	// TODO: if needed compute it here
	return mBBox;
}

std::vector<cv::Point> MserBlob::pts() const {
	return mPts;
}

std::vector<cv::Point> MserBlob::relativePts(const Vector2D & origin) const {
	
	std::vector<cv::Point> rPts;

	for (const cv::Point& p : mPts) {

		Vector2D rp = p - origin;
		rPts.push_back(rp.toCvPoint());
	}
	
	return rPts;
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
	Drawer::instance().drawRect(p, bbox().toQRectF());

	// draw center
	Drawer::instance().setStrokeWidth(3);
	Drawer::instance().drawPoint(p, bbox().center().toQPointF());
}

}