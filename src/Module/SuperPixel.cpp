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

QVector<QSharedPointer<MserBlob> > SuperPixel::getBlobs(const cv::Mat & img, int kernelSize) const {

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

QVector<QSharedPointer<MserBlob> > SuperPixel::mser(const cv::Mat & img) const {
	
	cv::Ptr<cv::MSER> mser = cv::MSER::create();
	mser->setMinArea(config()->mserMinArea());
	mser->setMaxArea(config()->mserMaxArea());

	std::vector<std::vector<cv::Point> > pixels;
	std::vector<cv::Rect> boxes;
	mser->detectRegions(img, pixels, boxes);

	assert(pixels.size() == boxes.size());

	Timer dtf;
	int nF = filterAspectRatio(pixels, boxes);
	qDebug() << nF << "filtered (aspect ratio) in " << dtf;

	// collect
	QVector<QSharedPointer<MserBlob> > blobs;
	for (int idx = 0; idx < pixels.size(); idx++) {
		QSharedPointer<MserBlob> cb(new MserBlob(pixels[idx], Converter::cvRectToQt(boxes[idx])));
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

//int SuperPixel::filterUnique(QVector<MserBlob>& blobs, double areaRatio) const {
//
//	QVector<MserBlob> blobsClean;
//
//	for (const MserBlob& b : blobs) {
//
//		double ua = b.uniqueArea(blobs);
//		if (ua / b.area() > areaRatio)
//			blobsClean << b;
//	}
//
//	int numRemoved = blobs.size() - blobsClean.size();
//
//	blobs = blobsClean;
//
//	return numRemoved;
//}

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
		QVector<QSharedPointer<MserBlob> > b = getBlobs(img, idx);
		mBlobs.append(b);
		qDebug() << b.size() << "/" << mBlobs.size() << "collected with kernel size" << idx << "in" << dti;
	}

	Timer dtf;
	int nF = MserBlob::filterDuplicates(mBlobs);
	qDebug() << nF << "filtered (duplicates) in" << dtf;

	//dtf.start();
	//nF = filterUnique(mBlobs, 0.001);
	//qDebug() << nF << "filtered (unique area) in" << dtf;

	dtf.start();
	// convert to pixels
	for (const QSharedPointer<MserBlob>& b : mBlobs)
		mPixels << b->toPixel();
	qDebug() << "conversion to pixel takes" << dtf;

	//// compute delauney triangulation
	//dtf.start();
	//mTriangles = connect(mBlobs);
	//qDebug() << "delauney computed in " << dtf;

	mDebug << mBlobs.size() << "regions computed in" << dt;

	return true;
}

QString SuperPixel::toString() const {

	QString msg = debugName();

	return msg;
}

QVector<QSharedPointer<Pixel> > SuperPixel::getSuperPixels() const {
	return mPixels;
}

QVector<QSharedPointer<MserBlob>> SuperPixel::getMserBlobs() const {
	return mBlobs;
}

QSharedPointer<SuperPixelConfig> SuperPixel::config() const {
	return qSharedPointerDynamicCast<SuperPixelConfig>(mConfig);
}

cv::Mat SuperPixel::drawSuperPixels(const cv::Mat & img) const {

	// draw super pixels
	Timer dtf;
	QPixmap pm = Image::instance().mat2QPixmap(img);
	QPainter p(&pm);

	for (int idx = 0; idx < mBlobs.size(); idx++) {
		Drawer::instance().setColor(ColorManager::instance().getRandomColor());
		p.setPen(Drawer::instance().pen());

		//// uncomment if you want to see MSER & SuperPixel at the same time
		//mBlobs[idx].draw(p);
		mPixels[idx]->ellipse().draw(p, 0.3);
		//qDebug() << mPixels[idx].ellipse();
	}

	qDebug() << "drawing takes" << dtf;
	return Image::instance().qPixmap2Mat(pm);
}

cv::Mat SuperPixel::drawMserBlobs(const cv::Mat & img) const {

	// draw mser blobs
	Timer dtf;
	QPixmap pm = Image::instance().mat2QPixmap(img);
	QPainter p(&pm);

	for (auto b : mBlobs) {
		Drawer::instance().setColor(ColorManager::instance().getRandomColor());
		p.setPen(Drawer::instance().pen());

		b->draw(p);
	}

	qDebug() << "drawing takes" << dtf;
	
	return Image::instance().qPixmap2Mat(pm);
}

// SuperPixelConfig --------------------------------------------------------------------
SuperPixelConfig::SuperPixelConfig() : ModuleConfig("Super Pixel") {
}

QString SuperPixelConfig::toString() const {
	return ModuleConfig::toString();
}

int SuperPixelConfig::mserMinArea() const {
	return mMserMinArea;
}

int SuperPixelConfig::mserMaxArea() const {
	return mMserMaxArea;
}

void SuperPixelConfig::load(const QSettings & settings) {

	// add parameters
	mMserMinArea = settings.value("MserMinArea", mMserMinArea).toInt();
	mMserMaxArea = settings.value("MserMaxArea", mMserMaxArea).toInt();

}

void SuperPixelConfig::save(QSettings & settings) const {

	// add parameters
	settings.setValue("MserMinArea", mMserMinArea);
	settings.setValue("MserMaxArea", mMserMaxArea);
}

}