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

#include "GCGraph.hpp"

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

	for (unsigned int idx = 0; idx < blobs.pixels.size(); idx++) {

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

	for (unsigned int idx = 0; idx < blobs.boxes.size(); idx++) {

		const cv::Rect& r = blobs.boxes[idx];
		bool duplicate = false;

		for (unsigned int cIdx = idx+1; cIdx < blobs.boxes.size(); cIdx++) {

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
		mPixels[idx]->draw(p, 0.3);
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

// MserContainer --------------------------------------------------------------------
void MserContainer::append(const MserContainer & o) {

	std::move(o.pixels.begin(), o.pixels.end(), std::back_inserter(pixels));
	std::move(o.boxes.begin(), o.boxes.end(), std::back_inserter(boxes));
}

QVector<QSharedPointer<MserBlob>> MserContainer::toBlobs() const {
	
	QVector<QSharedPointer<MserBlob> > blobs;
	for (unsigned int idx = 0; idx < pixels.size(); idx++) {

		QSharedPointer<MserBlob> b(new MserBlob(pixels[idx], boxes[idx], QString::number(idx)));
		blobs << b;
	}
	
	return blobs;
}

size_t MserContainer::size() const {
	return pixels.size();
}

// LocalOrientationConfig --------------------------------------------------------------------
LocalOrientationConfig::LocalOrientationConfig() : ModuleConfig("Local Orientation") {
}

QString LocalOrientationConfig::toString() const {
	
	QString msg;
	msg += " scales " + scaleIvl().toString();
	msg += " orientations " + QString::number(numOrientations());

	return msg;
}

int LocalOrientationConfig::maxScale() const {
	return mMaxScale;
}

int LocalOrientationConfig::minScale() const {
	return mMinScale;
}

Vector2D LocalOrientationConfig::scaleIvl() const {
	return Vector2D(mMinScale, mMaxScale);
}

int LocalOrientationConfig::numOrientations() const {
	return mNumOr;
}

int LocalOrientationConfig::histSize() const {
	return mHistSize;
}

void LocalOrientationConfig::load(const QSettings & settings) {

	// add parameters
	mMaxScale = settings.value("MaxScale", mMaxScale).toInt();
	mMinScale = settings.value("MinScale", mMinScale).toInt();
	mNumOr = settings.value("NumOrientations", mNumOr).toInt();
	mHistSize = settings.value("HistSize", mHistSize).toInt();

}

void LocalOrientationConfig::save(QSettings & settings) const {

	// add parameters
	settings.setValue("MaxScale", mMaxScale);
	settings.setValue("MinScale", mMinScale);
	settings.setValue("NumOrientations", mNumOr);
	settings.setValue("HistSize", mHistSize);
}

// LocalOrientation --------------------------------------------------------------------
LocalOrientation::LocalOrientation(const QVector<QSharedPointer<Pixel> >& set) {
	mSet = set;
	mConfig = QSharedPointer<LocalOrientationConfig>::create();
}

bool LocalOrientation::isEmpty() const {
	return mSet.isEmpty();
}

bool LocalOrientation::compute() {
	
	if (!checkInput())
		return false;
	
	Timer dt;
	for (QSharedPointer<Pixel> p : mSet)
		computeScales(p, mSet);

	mDebug << config()->toString();
	mDebug << "computed in" << dt;

	return true;
}

QString LocalOrientation::toString() const {
	return config()->toString();
}

QSharedPointer<LocalOrientationConfig> LocalOrientation::config() const {
	return qSharedPointerDynamicCast<LocalOrientationConfig>(mConfig);
}

QVector<QSharedPointer<Pixel>> LocalOrientation::getSuperPixels() const {
	return mSet;
}

bool LocalOrientation::checkInput() const {
	return !mSet.isEmpty();
}

void LocalOrientation::computeScales(QSharedPointer<Pixel>& pixel, const QVector<QSharedPointer<Pixel> >& set) const {
	
	const Vector2D& ec = pixel->center();
	QVector<QSharedPointer<Pixel> > cSet = set;
	
	// iterate over all scales
	for (double cRadius = config()->maxScale(); cRadius >= config()->minScale(); cRadius /= 2.0) {

		QVector<QSharedPointer<Pixel> > neighbors;

		// create neighbor set
		for (const QSharedPointer<Pixel>& p : cSet) {

			if (Vector2D(ec - p->center()).length() < cRadius) {
				neighbors << p;
			}
		}

		// compute orientation histograms
		computeAllOrHists(pixel, neighbors, cRadius);

		// reduce the set (since we reduce the radius, it must be contained in the current set)
		cSet = neighbors;
	}

}

void LocalOrientation::computeAllOrHists(QSharedPointer<Pixel>& pixel, const QVector<QSharedPointer<Pixel>>& set, double radius) const {

	const Vector2D& ec = pixel->center();
	QVector<QSharedPointer<Pixel> > neighbors;

	// create neighbor set
	for (const QSharedPointer<Pixel>& p : set) {

		if (Vector2D(ec - p->center()).length() < radius) {
			neighbors << p;
		}
	}

	// compute all orientations
	int nOr = config()->numOrientations();
	int histSize = config()->histSize();
	cv::Mat orHist(nOr, histSize, CV_32FC1);
	cv::Mat sparsity(1, nOr, CV_32FC1);

	for (int k = 0; k < nOr; k++) {

		// create orientation vector
		float sp = 0.0f;
		double cAngle = k * CV_PI / nOr;
		Vector2D orVec(radius, 0);
		orVec.rotate(cAngle);

		cv::Mat cRow = orHist.row(k);
		computeOrHist(pixel, neighbors, orVec, cRow, sp);

		sparsity.at<float>(0, k) = sp;
	}

	pixel->addStats(QSharedPointer<PixelStats>(new PixelStats(orHist, sparsity, radius, pixel->id())));
}

void LocalOrientation::computeOrHist(const QSharedPointer<Pixel>& pixel, 
	const QVector<QSharedPointer<Pixel>>& set, 
	const Vector2D & histVec, 
	cv::Mat& orHist,
	float& sparsity) const {

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

	sparsity = (float)std::log(cv::sum(orHist != 0)[0]/(orHist.cols*255.0));

	// DFT according to Koo16
	cv::dft(orHist, orHist);
	assert(!orHist.empty());

	float* lf = orHist.ptr<float>();
	float lfv = *lf;
	
	// remove very low frequencies
	for (int idx = 0; idx < 10 && idx < orHist.cols; idx++)
		lf[idx] = 0.0f;

	//// remove very high frequencies
	//for (int idx = orHist.cols - 1; idx > orHist.cols - 5 && idx > 0; idx--)
	//	lf[idx] = 0.0f;

	// see Koo et al 2016
	// log( X(k)^2/X(0)^2 )
	cv::log(orHist.mul(orHist) / (lfv*lfv) + 1.0, orHist);
	orHist *= -1.0f;
}

cv::Mat LocalOrientation::draw(const cv::Mat & img, const QString & id, double radius) const {

	QSharedPointer<Pixel> pixel;
	for (auto p : mSet)
		if (p->id() == id) {
			pixel = p;
			break;
		}

	if (!pixel) {
		qInfo() << "cannot draw local orientation for" << id << "because I did not find it...";
		return img;
	}

	// debug - remove
	QPixmap pm = Image::instance().mat2QPixmap(img);
	QPainter painter(&pm);

	Ellipse e(pixel->center(), Vector2D(radius, radius));
	e.draw(painter, 0.3);
	painter.setPen(ColorManager::instance().colors()[2]);

	const Vector2D& ec = pixel->center();
	QVector<QSharedPointer<Pixel> > neighbors;

	// create neighbor set
	for (const QSharedPointer<Pixel>& p : mSet) {

		if (Vector2D(ec - p->center()).length() < radius) {
			neighbors << p;
			p->draw(painter, 0.3);
		}
	}

	// draw the selected pixel in a different color
	painter.setPen(ColorManager::instance().colors()[0]);
	pixel->draw(painter);

	// compute all orientations
	int histSize = config()->histSize();
	int nOr = config()->numOrientations();
	cv::Mat orHist(nOr, histSize, CV_32FC1);
	
	for (int k = 0; k < nOr; k++) {

		// create orientation vector
		float sp = 0.0f;	// not needed
		double cAngle = k * CV_PI / nOr;
		Vector2D orVec(radius, 0);
		orVec.rotate(cAngle);

		cv::Mat cRow = orHist.row(k);
		computeOrHist(pixel, neighbors, orVec, cRow, sp);

		rdf::Histogram h(cRow);
		Rect r(30 + k * (histSize+5), pixel->center().y()-radius-150, histSize, 50);
		h.draw(painter, r);
		painter.drawText(r.bottomLeft().toQPoint(), QString::number(cAngle * DK_RAD2DEG));
	}

	return Image::instance().qPixmap2Mat(pm);
}


// PixelSetOrientation --------------------------------------------------------------------
PixelSetOrientation::PixelSetOrientation(const QVector<QSharedPointer<Pixel>>& set) {
	mSet = set;
}

bool PixelSetOrientation::isEmpty() const {
	return mSet.isEmpty();
}

bool PixelSetOrientation::compute() {
	
	if (!checkInput())
		return false;

	return true;
}

QVector<QSharedPointer<Pixel>> PixelSetOrientation::getSuperPixels() const {
	
	return mSet;
}

bool PixelSetOrientation::checkInput() const {
	return !isEmpty();
}

void PixelSetOrientation::constructGraph(const QVector<QSharedPointer<Pixel>>& pixel, const QVector<QSharedPointer<PixelEdge>>& edges) {


	rd3::GCGraph<double> graph;
	graph.create(pixel.size(), edges.size());

	for (int idx = 0; idx < pixel.size(); idx++) {

		int vtxIdx = graph.addVtx();

		double fromSource = 0, toSink = 0;

		//graph.a

	}

}

}