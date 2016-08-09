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

QSharedPointer<MserContainer> SuperPixel::getBlobs(const cv::Mat & img, int kernelSize) const {

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

QSharedPointer<MserContainer> SuperPixel::mser(const cv::Mat & img) const {
	
	cv::Ptr<cv::MSER> mser = cv::MSER::create();
	mser->setMinArea(config()->mserMinArea());
	mser->setMaxArea(config()->mserMaxArea());

	QSharedPointer<MserContainer> blobs(new MserContainer());
	mser->detectRegions(img, blobs->pixels, blobs->boxes);
	assert(blobs->pixels.size() == blobs->boxes.size());

	Timer dtf;
	int nF = filterAspectRatio(*blobs);
	qDebug() << "[aspect ratio filter]\tremoves" << nF << "blobs in" << dtf;

	dtf.start();
	nF = filterDuplicates(*blobs);
	qDebug() << "[duplicates filter]\tremoves" << nF << "blobs in" << dtf;

	//// collect the blobs
	//QVector<QSharedPointer<MserBlob> > blobs;

	//for (int idx = 0; idx < pixels.size(); idx++) {
	//	
	//	Rect r = Converter::cvRectToQt(boxes[idx]);
	//	QSharedPointer<MserBlob> cb(new MserBlob(pixels[idx], r));
	//	blobs << cb;
	//}

	return blobs;
}

int SuperPixel::filterAspectRatio(MserContainer& blobs, double aRatio) const {

	assert(blobs.pixels.size() == blobs.boxes.size());

	// filter w.r.t aspect ratio
	std::vector<std::vector<cv::Point> > pixelsClean;
	std::vector<cv::Rect> boxesClean;

	for (int idx = 0; idx < blobs.pixels.size(); idx++) {

		cv::Rect b = blobs.boxes[idx];
		double cARatio = (double)qMin(b.width, b.height) / qMax(b.width, b.height);

		if (cARatio > aRatio) {
			boxesClean.push_back(b);
			pixelsClean.push_back(blobs.pixels[idx]);
		}
	}

	int numRemoved = (int)(blobs.pixels.size() - pixelsClean.size());

	blobs.pixels = pixelsClean;
	blobs.boxes = boxesClean;

	return numRemoved;
}

int SuperPixel::filterDuplicates(MserContainer& blobs, int eps) const {

	int cnt = 0;

	std::vector<std::vector<cv::Point>> pixelsClean;
	std::vector<cv::Rect> boxesClean;

	for (int idx = 0; idx < blobs.boxes.size(); idx++) {

		const cv::Rect& r = blobs.boxes[idx];
		bool duplicate = false;

		for (int cIdx = idx+1; cIdx < blobs.boxes.size(); cIdx++) {

			if (idx == cIdx)
				continue;

			const cv::Rect& cr = blobs.boxes[cIdx];

			if (abs(r.x - cr.x) < eps &&
				abs(r.y - cr.y) < eps &&
				abs(r.width - cr.width) < eps &&
				abs(r.height - cr.height) < eps) {

				cnt++;
				duplicate = true;
				break;
			}
		}

		if (!duplicate) {
			pixelsClean.push_back(blobs.pixels[idx]);
			boxesClean.push_back(blobs.boxes[idx]);
		}
	}

	blobs.pixels = pixelsClean;
	blobs.boxes = boxesClean;

	return cnt;
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

	QSharedPointer<MserContainer> rawBlobs(new MserContainer());

	for (int idx = 0; idx < 10; idx += 4) {

		Timer dti;
		QSharedPointer<MserContainer> cb = getBlobs(img, idx);
		rawBlobs->append(*cb);
		qDebug() << cb->size() << "/" << rawBlobs->size() << "collected with kernel size" << 2*idx+1 << "in" << dti;
	}

	// filter duplicates that occur from different erosion sizes
	Timer dtf;
	int nf = filterDuplicates(*rawBlobs);
	qDebug() << "[final duplicates filter] removes" << nf << "blobs in" << dtf;

	// convert to pixels
	mBlobs = rawBlobs->toBlobs();
	for (const QSharedPointer<MserBlob>& b : mBlobs)
		mPixels << b->toPixel();

	dtf.start();
	localOrientation(mPixels);
	qDebug() << "local orientation estimation takes" << dtf;

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

void SuperPixel::localOrientation(QVector<QSharedPointer<Pixel>>& set) const {

	localOrientation(set, 250, 8);

}

void SuperPixel::localOrientation(QVector<QSharedPointer<Pixel>>& set, double radius, int n) const {

	// loop here
	//for (QSharedPointer<Pixel>& p : set) {
	//	localOrientation(p, set, radius, n);
	//}

	// debug
	QSharedPointer<Pixel> visPix;
	for (auto p : set)
		//if (p->id() == "636") {
		if (p->id() == "1012") {
			visPix = p;
			break;
		}

	localOrientationDebug(visPix, set, radius);

}

void SuperPixel::localOrientationDebug(QSharedPointer<Pixel>& pixel, const QVector<QSharedPointer<Pixel>>& set, double radius) const {

	// debug - remove
	QPixmap pm = Image::instance().mat2QPixmap(mSrcImg);
	QPainter painter(&pm);
	
	Ellipse e(pixel->center(), Vector2D(radius, radius));
	e.draw(painter, 0.3);
	painter.setPen(ColorManager::instance().colors()[2]);

	const Vector2D& ec = pixel->center();
	QVector<QSharedPointer<Pixel> > neighbors;

	// create neighbor set
	for (const QSharedPointer<Pixel>& p : set) {

		if (Vector2D(ec - p->center()).length() < radius) {
			neighbors << p;
			p->draw(painter);
		}
	}

	painter.setPen(ColorManager::instance().colors()[0]);
	pixel->draw(painter);

	// compute all orientations
	int histSize = 100;
	int n = 8;
	cv::Mat orHist(n, histSize, CV_32FC1);
	for (int k = 0; k < n; k++) {
		
		// create orientation vector
		double cAngle = k * CV_PI / n;
		Vector2D orVec(radius, 0);
		orVec.rotate(cAngle);

		cv::Mat cRow = orHist.row(k);
		localOrientation(pixel, neighbors, orVec, cRow);

		rdf::Histogram h(cRow);
		Rect r(30 + k * (histSize+5), pixel->center().y()-radius-150, histSize, 50);
		h.draw(painter, r);
		painter.drawText(r.bottomLeft().toQPoint(), QString::number(cAngle * DK_RAD2DEG));
	}

	cv::Mat dbImg = Image::instance().qPixmap2Mat(pm);
	Image::instance().save(dbImg, "D:/read/test/localNeighbors.tif");
}

void SuperPixel::localOrientation(QSharedPointer<Pixel>& pixel, const QVector<QSharedPointer<Pixel>>& set, double radius, int n) const {

	const Vector2D& ec = pixel->center();
	QVector<QSharedPointer<Pixel> > neighbors;

	// create neighbor set
	for (const QSharedPointer<Pixel>& p : set) {

		if (Vector2D(ec - p->center()).length() < radius) {
			neighbors << p;
		}
	}

	// compute all orientations
	int histSize = 100;
	cv::Mat orHist(n, histSize, CV_32FC1);
	for (int k = 0; k < n; k++) {

		// create orientation vector
		double cAngle = k * CV_PI / n;
		Vector2D orVec(radius, 0);
		orVec.rotate(cAngle);

		cv::Mat cRow = orHist.row(k);
		localOrientation(pixel, neighbors, orVec, cRow);
		// TODO: set the orientation histogram here
	}
}

void SuperPixel::localOrientation(const QSharedPointer<Pixel>& pixel, const QVector<QSharedPointer<Pixel>>& set, const Vector2D & histVec, cv::Mat& orHist) const {
	
	
	double hl = histVec.length();
	Vector2D histVecNorm = histVec;
	histVecNorm /= hl;
	double scale = 1.0 / (2 * hl) * (orHist.cols - 1);

	const Vector2D pc = pixel->center();
	
	// prepare histogram
	orHist.setTo(0);
	float* orPtr = orHist.ptr<float>();

	for (const QSharedPointer<Pixel>& p : set) {

		Vector2D lc = p->center() - pc;
		double v = histVecNorm * lc;

		// bin it
		int hIdx = qRound((v + hl) * scale);
		assert(hIdx >= 0 && hIdx < orHist.cols);

		orPtr[hIdx] += 1;

		//// estimate radius (assuming circles)
		//const Vector2D& a = p->ellipse().axis();
		//int r = qRound((a.x() + a.y()) / 2.0 * scale);
		//int start = (hIdx - r < 0) ? 0 : hIdx - r;

		//for (int idx = start; idx < hIdx + r && idx < orHist.cols; idx++) {
		//	orPtr[idx] += 1;
		//}
		
	}
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

void MserContainer::append(const MserContainer & o) {

	std::move(o.pixels.begin(), o.pixels.end(), std::back_inserter(pixels));
	std::move(o.boxes.begin(), o.boxes.end(), std::back_inserter(boxes));
}

QVector<QSharedPointer<MserBlob>> MserContainer::toBlobs() const {
	
	QVector<QSharedPointer<MserBlob> > blobs;
	for (int idx = 0; idx < pixels.size(); idx++) {

		QSharedPointer<MserBlob> b(new MserBlob(pixels[idx], boxes[idx], QString::number(idx)));
		blobs << b;
	}
	
	return blobs;
}

size_t MserContainer::size() const {
	return pixels.size();
}

}