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
#include "LineTrace.h"

#pragma warning(push, 0)	// no warnings from includes
#include <QDebug>

#include <QUuid>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d/features2d.hpp>

#pragma warning(pop)

namespace rdf {


// SuperPixel --------------------------------------------------------------------
SuperPixel::SuperPixel(const cv::Mat& srcImg) : SuperPixelBase(srcImg) {
	
	mConfig = QSharedPointer<SuperPixelConfig>::create();
	mConfig->loadSettings();
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
	/*int nF = */filterDuplicates(*blobs, 7, 10);
	//qDebug() << "[duplicates filter]\tremoves" << nF << "blobs in" << dtf;

	dtf.start();
	/*nF = */filterAspectRatio(*blobs);
	//qDebug() << "[aspect ratio filter]\tremoves" << nF << "blobs in" << dtf;

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

int SuperPixel::filterDuplicates(MserContainer& blobs, int eps, int upperBound) const {

	int cnt = 0;
	size_t nBoxes = blobs.boxes.size();

	std::vector<std::vector<cv::Point>> pixelsClean;
	std::vector<cv::Rect> boxesClean;

	for (size_t idx = 0; idx < nBoxes; idx++) {

		const cv::Rect& r = blobs.boxes[idx];
		bool duplicate = false;

		for (size_t cIdx = idx+1; cIdx < nBoxes; cIdx++) {

			// should never happen...
			assert(idx != cIdx);

			if (upperBound != -1 && cIdx > idx + upperBound)
				break;

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

bool SuperPixel::compute() {

	if (!checkInput())
		return false;

	Timer dt;

	cv::Mat img = mSrcImg.clone();
	img = IP::grayscale(img);
	cv::normalize(img, img, 255, 0, cv::NORM_MINMAX);

	QSharedPointer<MserContainer> rawBlobs(new MserContainer());

	int maxFilter = config()->erosionStep()*config()->numErosionLayers();

	// do not erode large pyramid levels
	if (mPyramidLevel > 3) {
		config()->setNumErosionLayers(1);
	}

	for (int idx = 0; idx < maxFilter; idx += config()->erosionStep()) {

		Timer dti;
		QSharedPointer<MserContainer> cb = getBlobs(img, idx);
		rawBlobs->append(*cb);
		//qDebug() << cb->size() << "/" << rawBlobs->size() << "collected with kernel size" << 2*idx+1 << "in" << dti;
	}

	// filter duplicates that occur from different erosion sizes
	Timer dtf;
	filterDuplicates(*rawBlobs);
	//qDebug() << "[final duplicates filter] removes" << nf << "blobs in" << dtf;

	// convert to pixels
	mBlobs = rawBlobs->toBlobs();
	for (const QSharedPointer<MserBlob>& b : mBlobs)
		mSet << b->toPixel();

	mDebug << mBlobs.size() << "regions computed in" << dt;

	return true;
}

QString SuperPixel::toString() const {

	QString msg = debugName();

	return msg;
}

QVector<QSharedPointer<MserBlob> > SuperPixel::getMserBlobs() const {
	return mBlobs;
}

QSharedPointer<SuperPixelConfig> SuperPixel::config() const {
	return qSharedPointerDynamicCast<SuperPixelConfig>(mConfig);
}

cv::Mat SuperPixel::draw(const cv::Mat & img, const QColor& col) const {

	// draw super pixels
	Timer dtf;
	QPixmap pm = Image::mat2QPixmap(img);
	QPainter p(&pm);


	//DBScanPixel dbs(mPixels);
	//dbs.compute();
	//QVector<PixelSet> sets = dbs.sets();
	//qDebug() << "dbscan found" << sets.size() << "clusters in" << dtf;

	//for (auto s : sets) {
	//	p.setPen(pen);
	//	s.draw(p);
	//}

	p.setPen(col);

	for (int idx = 0; idx < mBlobs.size(); idx++) {
	
		
		if (!col.isValid())
			p.setPen(ColorManager::randColor());

		// uncomment if you want to see MSER & SuperPixel at the same time
		//mBlobs[idx].draw(p);
		mSet[idx]->draw(p, 0.2, Pixel::DrawFlags() | Pixel::draw_ellipse | Pixel::draw_stats | Pixel::draw_label_colors | Pixel::draw_tab_stops);
		//qDebug() << mPixels[idx].ellipse();
	}

	qDebug() << "drawing takes" << dtf;
	return Image::qPixmap2Mat(pm);
}

cv::Mat SuperPixel::drawMserBlobs(const cv::Mat & img, const QColor& col) const {

	// draw mser blobs
	Timer dtf;
	QPixmap pm = Image::mat2QPixmap(img);
	QPainter p(&pm);
	p.setPen(col);

	for (auto b : mBlobs) {

		if (!col.isValid())
			p.setPen(ColorManager::randColor());

		b->draw(p);
	}

	qDebug() << "drawing takes" << dtf;
	
	return Image::qPixmap2Mat(pm);
}

// SuperPixelConfig --------------------------------------------------------------------
SuperPixelConfig::SuperPixelConfig() : ModuleConfig("Super Pixel") {
}

QString SuperPixelConfig::toString() const {
	return ModuleConfig::toString();
}

/// <summary>
/// The minimum SuperPixel area in pixel.
/// NOTE: the segmented pixels are summed up
/// rather than its resulting ellipses.
/// </summary>
/// <returns></returns>
int SuperPixelConfig::mserMinArea() const {
	return mMserMinArea;
}

/// <summary>
/// The maximum SuperPixel area in pixel.
/// NOTE: the segmented pixels are summed up
/// rather than its resulting ellipses.
/// </summary>
/// <returns></returns>
int SuperPixelConfig::mserMaxArea() const {
	
	return checkParam(mMserMaxArea, mserMinArea(), INT_MAX, "mserMaxArea");
}

/// <summary>
/// The erosion step in pixel.
/// The kernelsize is iteratively increased
/// when computing the erosion layers.
/// </summary>
/// <returns></returns>
int SuperPixelConfig::erosionStep() const {

	return checkParam(mErosionStep, 1, 20, "erosionStep");
}

void SuperPixelConfig::setNumErosionLayers(int numLayers) {
	mNumErosionLayers = numLayers;
}

/// <summary>
/// Numbers the erosion layers.
/// The image is iteratively eroded in order to split
/// cursive handwriting. Specify how many erosion layers
/// should be created - when testing 3 seemed to be a good
/// trade-off between accuracy and speed.
/// </summary>
/// <returns></returns>
int SuperPixelConfig::numErosionLayers() const {
	
	return checkParam(mNumErosionLayers, 1, 20, "numErosionLayers");
}

void SuperPixelConfig::load(const QSettings & settings) {

	// add parameters
	mMserMinArea = settings.value("mserMinArea", mserMinArea()).toInt();
	mMserMaxArea = settings.value("mserMaxArea", mserMaxArea()).toInt();
	mErosionStep = settings.value("erosionStep", erosionStep()).toInt();
	mNumErosionLayers = settings.value("numErosionLayers", numErosionLayers()).toInt();
}

void SuperPixelConfig::save(QSettings & settings) const {

	// add parameters
	settings.setValue("mserMinArea", mserMinArea());
	settings.setValue("mserMaxArea", mserMaxArea());
	settings.setValue("erosionStep", erosionStep());
	settings.setValue("numErosionLayers", numErosionLayers());
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
	msg += "scales " + scaleIvl().toString();
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

void LocalOrientationConfig::setNumOrientations(int numOr) {
	mNumOr = numOr;
}

void LocalOrientationConfig::setMaxScale(int maxScale) {
	mMaxScale = maxScale;
}

void LocalOrientationConfig::setMinScale(int minScale) {
	mMinScale = minScale;
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
LocalOrientation::LocalOrientation(const PixelSet& set) {
	mSet = set;
	mConfig = QSharedPointer<LocalOrientationConfig>::create();
	mConfig->loadSettings();
}

bool LocalOrientation::isEmpty() const {
	return mSet.isEmpty();
}

bool LocalOrientation::compute() {
	
	if (!checkInput())
		return false;
	
	QVector<Pixel*> ptrSet;
	for (const QSharedPointer<Pixel>& p : mSet.pixels())
		ptrSet << p.data();

	for (Pixel* p : ptrSet)
		computeScales(p, ptrSet);

	return true;
}

QString LocalOrientation::toString() const {
	return config()->toString();
}

QSharedPointer<LocalOrientationConfig> LocalOrientation::config() const {
	return qSharedPointerDynamicCast<LocalOrientationConfig>(mConfig);
}

PixelSet LocalOrientation::set() const {
	return mSet;
}

bool LocalOrientation::checkInput() const {
	return !mSet.isEmpty();
}

void LocalOrientation::computeScales(Pixel* pixel, const QVector<Pixel*>& set) const {
	
	const Vector2D& ec = pixel->center();
	QVector<Pixel*> cSet = set;
	
	// iterate over all scales
	for (double cRadius = config()->maxScale(); cRadius >= config()->minScale(); cRadius /= 2.0) {

		QVector<Pixel*> neighbors;

		// create neighbor set
		for (Pixel* p : cSet) {

			if (ec.isNeighbor(p->center(), cRadius)) {
				neighbors << p;
			}
		}

		// compute orientation histograms
		computeAllOrHists(pixel, neighbors, cRadius);

		// reduce the set (since we reduce the radius, it must be contained in the current set)
		cSet = neighbors;
	}
}

void LocalOrientation::computeAllOrHists(Pixel* pixel, const QVector<Pixel*>& set, double radius) const {

	const Vector2D& ec = pixel->center();

	QVector<const Pixel*> neighbors;

	// create neighbor set
	for (const Pixel* p : set) {

		if (ec.isNeighbor(p->center(), radius)) {
			neighbors << p;
		}
	}

	// compute all orientations
	int nOr = config()->numOrientations();
	int histSize = config()->histSize();

	// setting the orientation histogram default to 1 
	// gives better estimations if the hist is sparse (lots of 0 entries)
	// due to the dft
	cv::Mat orHist(nOr, histSize, CV_32FC1, cv::Scalar(1));
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

void LocalOrientation::computeOrHist(const Pixel* pixel, 
	const QVector<const Pixel*>& set, 
	const Vector2D & histVec, 
	cv::Mat& orHist,
	float& sparsity) const {


	double hl = histVec.length();
	Vector2D histVecNorm = histVec;
	histVecNorm /= hl;
	double scale = 1.0 / (2 * hl) * (orHist.cols - 1);

	const Vector2D pc = pixel->center();

	// prepare histogram
	float* orPtr = orHist.ptr<float>();

	for (const Pixel* p : set) {

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

	// estimate sparsity
	double sumNonZero = 0;
	
	for (int cIdx = 0; cIdx < orHist.cols; cIdx++) {
		if (orPtr[cIdx] != 0)
			sumNonZero++;
	}

	sparsity = (float)std::log(sumNonZero/orHist.cols);
	
	// DFT according to Koo16
	cv::dft(orHist, orHist);
	assert(!orHist.empty());

	//orPtr = orHist.ptr<float>();
	float normValSq = orPtr[0] * orPtr[0];	// the normalization term is always at [0] - we need it sqaured

	for (int cIdx = 0; cIdx < orHist.cols; cIdx++) {
		
		// >= 3 -> remove low frequencies
		if (cIdx >= 3) {

			// see Koo16:								val = -log( (val*val) / (hist[0]*hist[0]));
			// I changed it (for numerical stability):	val = -log( (val*val) / (hist[0]*hist[0]) + 1.0);
			double v = orPtr[cIdx];
			v *= v;
			v /= normValSq;
			v += 1.0;	// scale log
			orPtr[cIdx] = (float)-std::log(v);
		}
		else {
			// remove very low frequencies - they might create larger peaks than the recurring frequency
			orPtr[cIdx] = 0.0f;
		}
	}
}

cv::Mat LocalOrientation::draw(const cv::Mat & img, const QString & id, double radius) const {

	QSharedPointer<Pixel> pixel = mSet.find(id);

	if (!pixel) {
		qInfo() << "cannot draw local orientation for" << id << "because I did not find it...";
		return img;
	}

	// debug - remove
	QPixmap pm = Image::mat2QPixmap(img);
	QPainter painter(&pm);

	Ellipse e(pixel->center(), Vector2D(radius, radius));
	e.draw(painter, 0.3);
	painter.setPen(ColorManager::colors()[2]);

	const Vector2D& ec = pixel->center();
	QVector<const Pixel*> neighbors;

	// create neighbor set
	for (const QSharedPointer<Pixel>& p : mSet.pixels()) {
		
		if (ec.isNeighbor(p->center(), radius)) {
			neighbors << p.data();
			p->draw(painter, 0.3, Pixel::DrawFlags() | Pixel::draw_ellipse | Pixel::draw_stats);
		}
	}

	// draw the selected pixel in a different color
	painter.setPen(ColorManager::colors()[0]);
	pixel->draw(painter, 0.3, Pixel::DrawFlags() | Pixel::draw_ellipse | Pixel::draw_stats | Pixel::draw_id);

	// compute all orientations
	int histSize = config()->histSize();
	int nOr = config()->numOrientations();
	cv::Mat orHist(nOr, histSize, CV_32FC1, cv::Scalar(0));
	Histogram dominantHist;

	for (int k = 0; k < nOr; k++) {

		// create orientation vector
		float sp = 0.0f;	// not needed
		double cAngle = k * CV_PI / nOr;
		Vector2D orVec(radius, 0);
		orVec.rotate(cAngle);

		cv::Mat cRow = orHist.row(k);
		computeOrHist(pixel.data(), neighbors, orVec, cRow, sp);

		rdf::Histogram h(cRow);
		Rect r(30 + k * (histSize+5), pixel->center().y()-radius-150, histSize, 50);
		h.draw(painter, r);

		if (k == pixel->stats()->orientationIndex())
			dominantHist = h;

		// draw angle
		Vector2D tp = r.topLeft();
		tp.setY(tp.y() - (r.height() / 2.0 + 5));
		painter.drawText(tp.toQPoint(), QString::number(cAngle * DK_RAD2DEG));
	}

	Rect r(30, pixel->center().y() - radius, histSize, 50);
	dominantHist.draw(painter, r);

	return Image::qPixmap2Mat(pm);
}

// LineSuperPixel --------------------------------------------------------------------
LineSuperPixel::LineSuperPixel(const cv::Mat & img) : SuperPixelBase(img) {
	mConfig = QSharedPointer<LinePixelConfig>::create();
}

bool LineSuperPixel::compute() {

	if (!checkInput())
		return false;

	Timer dt;

	cv::Mat img = mSrcImg.clone();
	img = IP::grayscale(img);
	cv::normalize(img, img, 255, 0, cv::NORM_MINMAX);

	LineTraceLSD lt(mSrcImg);
	lt.config()->setScale(1.0);
	lt.lineFilter().config()->setMinLength(config()->minLineLength());

	if (!lt.compute()) {
		qWarning() << "could not compute separators...";
	}

	mLines = lt.lines();

	for (const Line& l : mLines) {

		Ellipse e(l.center(), Vector2D(l.length(), 10), l.angle());
		mSet.add(QSharedPointer<Pixel>(new Pixel(e, e.bbox())));
	}


	mInfo << mSet.size() << "pixels extracted in" << dt;

	return true;
}

QString LineSuperPixel::toString() const {
	return config()->toString();
}

QSharedPointer<LinePixelConfig> LineSuperPixel::config() const {
	return qSharedPointerDynamicCast<LinePixelConfig>(mConfig);
}

cv::Mat LineSuperPixel::draw(const cv::Mat & img) const {

	// debug - remove
	QPixmap pm = Image::mat2QPixmap(img);
	QPainter p(&pm);

	p.setPen(ColorManager::blue());

	for (auto px : mSet.pixels()) {
		px->draw(p, 0.3, Pixel::DrawFlags() | /*Pixel::draw_id |*/ Pixel::draw_center | Pixel::draw_stats);
	}

	return Image::qPixmap2Mat(pm);
}

bool LineSuperPixel::checkInput() const {

	return !mSrcImg.empty();
}

// LinePixelConfig --------------------------------------------------------------------
LinePixelConfig::LinePixelConfig() : ModuleConfig("LSD Super Pixel") {
}

QString LinePixelConfig::toString() const {
	return ModuleConfig::toString();
}

int LinePixelConfig::minLineLength() const {
	return checkParam(mMinLineLength, 0, 1000, "minLineLength");
}

void LinePixelConfig::load(const QSettings & settings) {

	mMinLineLength = settings.value("minLineLength", minLineLength()).toInt();
}

void LinePixelConfig::save(QSettings & settings) const {
	settings.setValue("minLineLenght", minLineLength());
}

// GridSuperPixel --------------------------------------------------------------------
GridSuperPixel::GridSuperPixel(const cv::Mat& img) : SuperPixelBase(img) {
	mConfig = QSharedPointer<GridPixelConfig>::create();
	mConfig->loadSettings();
}

bool GridSuperPixel::compute() {

	if (!checkInput())
		return false;

	Timer dt;

	cv::Mat mag, phase;
	edges(mSrcImg, mag, phase);
	
	QMap<int, QSharedPointer<GridPixel> > pixelMap = computeGrid(mag, phase, config()->winSize(), config()->winOverlap());
	mGridPixel = merge(pixelMap, mag, phase);

	for (auto p : mGridPixel) {
		if (!p->isDead())
			mSet << p->toPixel();
	}

	mSet = filter(mSet, 4.0);

	mDebug << mSet.size() << "regions computed in" << dt;

	return true;
}

void GridSuperPixel::edges(const cv::Mat& src, cv::Mat& magnitude, cv::Mat& orientation) const {

	// prepare
	cv::Mat img = src.clone();
	img = IP::grayscale(img);
	img.convertTo(img, CV_32FC1);
	cv::normalize(img, img, 1.0, 0, cv::NORM_MINMAX);

	float k[3] = { 1.0f, 0.0f, -1.0f };
	cv::Mat kernel(1, 3, CV_32FC1, k);
	cv::Mat imgDx, imgDy;
	cv::filter2D(img, imgDx, CV_32FC1, kernel);		// dx
	cv::filter2D(img, imgDy, CV_32FC1, kernel.t());	// dy
	img.release();

	cv::magnitude(imgDx, imgDy, magnitude);
	cv::phase(imgDx, imgDy, orientation);

	cv::normalize(magnitude, magnitude, 1.0f, 0.0f, cv::NORM_MINMAX);

	// remove separator lines - this is pretty usefull for e.g. tables
	if (config()->applyLineMask()) {
		// remove separator lines
		cv::Mat seps = lineMask(src);
		seps.convertTo(seps, magnitude.depth());

		magnitude = magnitude.mul(seps);
	}
}

QMap<int, QSharedPointer<GridPixel> > GridSuperPixel::computeGrid(const cv::Mat & mag, const cv::Mat& phase, int winSize, double winOverlap) const {
	
	assert(mag.size() == phase.size());

	// setup grid cache
	int step = qRound(winSize*winOverlap);
	Rect winr(Vector2D(), Vector2D(winSize, winSize));

	int nrw = qRound((double)mag.rows / winSize * 1.0 / winOverlap);
	int ncw = qRound((double)mag.cols / winSize * 1.0 / winOverlap);

	Vector2D mr(0, step);
	Vector2D mc(step, 0);
	Vector2D si(mag.size());

	double thr = winSize*winSize * config()->minEnergy();

	cv::Mat gvec = Algorithms::get1DGauss(winSize / 3.0, winSize);
	cv::Mat weight = gvec.t() * gvec;
	cv::normalize(weight, weight, 1.0, 0.0, cv::NORM_MINMAX);

	QMap<int, QSharedPointer<GridPixel> > pixels;
	int idx = 0;

	for (int rIdx = 0; rIdx < nrw; rIdx++) {

		// init window on the current row
		Rect wc = winr;
		winr.move(mr);	// slide (for the next step)

		for (int cIdx = 0; cIdx < ncw; cIdx++) {

			wc = wc.clipped(si);

			cv::Mat winM = mag(wc.toCvRect());
			cv::Mat winP = phase(wc.toCvRect());
			double s = cv::sum(winM)[0];

			if (s > thr) {
				
				
				QSharedPointer<GridPixel> gp(new GridPixel(idx, ncw));
				gp->compute(winM, winP, weight);
				
				// locate pixel might cancel
				if (!gp->isDead()) {
					gp->move(wc.topLeft());	// move back to global coords
					pixels.insert(idx, gp);
				}
			}

			wc.move(mc);	// slide (for the next step)
			idx++;
		}
	}
	
	return pixels;
}

QVector<QSharedPointer<GridPixel>> GridSuperPixel::merge(const QMap<int, QSharedPointer<GridPixel> >& pixels, const cv::Mat& mag, const cv::Mat& phase) const {
	
	QVector<QSharedPointer<GridPixel> > px;

	for (auto p : pixels) {

		assert(p);

		if (p->isDead())
			continue;

		const Rect& pbox = p->ellipse().bbox();

		bool merged = false;
		
		// check all neighbors
		for (int idx : p->neighbors()) {

			QSharedPointer<GridPixel> po = pixels[idx];

			if (po && !po->isDead()) {

				const Rect& pobox = po->ellipse().bbox();

				// check if we have the same local orientation & if the bounding boxes intersect
				if (p->orIdx() == po->orIdx() && pbox.intersects(pobox)) {

					Rect r = p->ellipse().bbox().joined(po->ellipse().bbox());

					r = r.clipped(mag.size());
					cv::Mat magW = mag(r.toCvRect());
					cv::Mat phaseW = phase(r.toCvRect());

					p->compute(magW, phaseW);
					p->move(r.topLeft());	// move back to global coords

					merged = true;
					px << p;

					po->kill();
					
					break;
				}
			}
		}

		if (!merged)
			px << p;
	}
	
	return px;
}

PixelSet GridSuperPixel::filter(const PixelSet& set, double clusterStrength) const {

	Timer dt;

	// parameter
	QVector<PixelSet> sets = cluster(set);

	PixelSet cleanSet;
	int nRm = 0;

	for (const PixelSet& s : sets) {

		// cluster strength
		double cs = 0.0;

		for (auto px : s.pixels())
			cs += px->value();

		if (cs > clusterStrength)
			cleanSet << s;
		else
			nRm += s.size();
	}
	
	qDebug() << nRm << "removed in" << dt;

	return cleanSet;
}

QVector<PixelSet> GridSuperPixel::cluster(const PixelSet & set) const {

	// parameter
	double dbDist = 15;

	DBScanPixel dbp(set);
	dbp.setFast(true);
	dbp.setMaxDistance(dbDist);

	dbp.compute();

	return dbp.sets();
}

cv::Mat GridSuperPixel::lineMask(const cv::Mat & src) const {

	LineTraceLSD ltl(src);
	ltl.config()->setMergeLines(false);

	auto lfc = ltl.lineFilter().config();
	lfc->setMinLength(qRound(lfc->minLength() / std::pow(2, mPyramidLevel)));	// reduce min line length for large scales

	if (!ltl.compute()) {
		qWarning() << "could not compute line image";
		return cv::Mat();
	}

	cv::Mat img(src.size(), CV_32FC1, cv::Scalar(1.0));
	auto lines = ltl.separatorLines();

	for (const Line& l : lines) {
		cv::line(img, l.p1().toCvPoint(), l.p2().toCvPoint(), cv::Scalar(0.0), 8);
	}

	return img;
}

QString GridSuperPixel::toString() const {

	QString msg = debugName();

	return msg;
}

QSharedPointer<GridPixelConfig> GridSuperPixel::config() const {
	return qSharedPointerDynamicCast<GridPixelConfig>(mConfig);
}

cv::Mat GridSuperPixel::draw(const cv::Mat & img, const QColor& col) const {

	// draw super pixels
	Timer dtf;

	cv::Mat mag, phase;
	edges(img, mag, phase);

	QPixmap pm = Image::mat2QPixmap(mag);
	QPainter p(&pm);

	p.setPen(col);

	Histogram hist(0, 1.0, 30);

	for (auto gp : mGridPixel) {
		//gp->draw(p);
		hist.add(gp->edgeStrength());
	}

	for (int idx = 0; idx < mSet.size(); idx++) {

		if (!col.isValid())
			p.setPen(ColorManager::randColor());

		// uncomment if you want to see MSER & SuperPixel at the same time
		mSet[idx]->draw(p, 0.2, Pixel::DrawFlags() | Pixel::draw_ellipse | Pixel::draw_stats | Pixel::draw_label_colors | Pixel::draw_tab_stops);
	}

	p.setPen(ColorManager::pink());
	hist.draw(p, Rect(Vector2D(30, 30), Vector2D(200, 100)));

	qDebug() << "drawing takes" << dtf;
	return Image::qPixmap2Mat(pm);
}

// -------------------------------------------------------------------- GridPixelConfig 
GridPixelConfig::GridPixelConfig() : ModuleConfig("Grid Pixel") {
}

QString GridPixelConfig::toString() const {
	QString msg = ModuleConfig::toString() + ":";
	msg += " window size: " + QString::number(winSize());
	msg += " window overlaps: " + QString::number(winOverlap());
	msg += " minimum energy " + QString::number(minEnergy());

	return msg;
}

int GridPixelConfig::winSize() const {
	return ModuleConfig::checkParam(mWinSize, 1, 1000, "winSize");
}

double GridPixelConfig::winOverlap() const {
	return ModuleConfig::checkParam(mWinOverlap, 0.01, 1.0, "winOverlap");
}

double GridPixelConfig::minEnergy() const {
	return ModuleConfig::checkParam(mMinEnergy, 0.0, 1.0, "minEnergy");
}

bool GridPixelConfig::applyLineMask() const {
	return mLineMask;
}

void GridPixelConfig::load(const QSettings & settings) {

	// add parameters
	mWinSize = settings.value("winSize", winSize()).toInt();
	mWinOverlap = settings.value("winOverlap", winOverlap()).toDouble();
	mMinEnergy = settings.value("minEnergy", minEnergy()).toDouble();
	mLineMask = settings.value("applyLineMask", applyLineMask()).toDouble();
}

void GridPixelConfig::save(QSettings & settings) const {

	// add parameters
	settings.setValue("winSize", winSize());
	settings.setValue("winOverlap", winOverlap());
	settings.setValue("minEnergy", minEnergy());
	settings.setValue("applyLineMask", applyLineMask());
}

// -------------------------------------------------------------------- SuperPixelBase 
SuperPixelBase::SuperPixelBase(const cv::Mat & img) {
	mSrcImg = img;
}

bool SuperPixelBase::isEmpty() const {
	return mSrcImg.empty();
}

PixelSet SuperPixelBase::pixelSet() const {
	return mSet;
}

void SuperPixelBase::setPyramidLevel(int level) {
	mPyramidLevel = level;
}

int SuperPixelBase::pyramidLevel() {
	return mPyramidLevel;
}

bool SuperPixelBase::checkInput() const {

	if (mSrcImg.empty()) {
		mWarning << "the source image must not be empty...";
		return false;
	}

	return true;
}

// -------------------------------------------------------------------- GridPixel 
GridPixel::GridPixel(int index, int numColumns) {
	mIndex = index;
	mNumColumns = numColumns;
}

bool GridPixel::operator==(const GridPixel & gpr) {
	return row() == gpr.row() && col() == gpr.col();
}

void GridPixel::compute(const cv::Mat& mag, const cv::Mat& phase, const cv::Mat& weight) {

	assert(mag.size() == phase.size());

	// parameter!
	double minMag = 0.05;	// minimum gradient magnitude
	int numHistBins = 8;	// number of orientation histogram bins (not too crucial)

	QVector<Vector2D> pts;
	Histogram orHist(0, 2*CV_PI, numHistBins);

	for (int rIdx = 0; rIdx < mag.rows; rIdx++) {

		const float* mp = mag.ptr<float>(rIdx);
		const float* pp = phase.ptr<float>(rIdx);
		const float* w = weight.empty() ? 0 : weight.ptr<float>(rIdx);

		for (int cIdx = 0; cIdx < mag.cols; cIdx++) {

			float spw = w ? mp[cIdx] * w[cIdx] : mp[cIdx];

			if (spw > minMag) {
				pts << Vector2D(cIdx, rIdx);
				orHist.add(pp[cIdx], spw);
			}
		}
	}

	mOrIdx = orHist.maxBinIdx();
	mEdgeStrength = (double)pts.size()/(mag.rows*mag.cols);

	// reject if there are too few gradients present
	if (isDead()) {
		return;
	}

	mEllipse = Ellipse::fromData(pts);
}

bool GridPixel::isDead() const {

	double minEdgeStrength = 0.01;		// minimum # of points
	return mEdgeStrength < minEdgeStrength;
}

void GridPixel::kill() {
	mEdgeStrength = 0;
}

void GridPixel::move(const Vector2D & vec) {
	mEllipse.move(vec);
}

int GridPixel::row() const {
	return cvFloor(mIndex/(double)mNumColumns);
}

int GridPixel::col() const {
	return mIndex % mNumColumns;
}

int GridPixel::orIdx() const {
	return mOrIdx;
}

double GridPixel::edgeStrength() const {
	return mEdgeStrength;
}

Ellipse GridPixel::ellipse() const {
	return mEllipse;
}

void GridPixel::draw(QPainter & p) const {

	QPen oPen = p.pen();

	double angle = mOrIdx / 8.0 * 2 * CV_PI;
	int length = 20;

	p.setPen(QColor(qRound(mOrIdx/8.0*255), 100, 100));
	p.setOpacity(mEdgeStrength);

	p.translate(ellipse().center().toQPointF());
	p.rotate(angle*DK_RAD2DEG);
	p.drawLine(QPointF(), QPointF(-length, 0));
	p.rotate(-angle*DK_RAD2DEG);
	p.translate(-ellipse().center().toQPointF());
	
	p.setPen(oPen);
	p.setOpacity(1.0);
}

int GridPixel::index(int row, int col) const {
	
	return row*mNumColumns+col;
}

QVector<int> rdf::GridPixel::neighbors() const {
	
	QVector<int> nb;
	nb << index(row() - 1, col());
	nb << index(row() + 1, col());
	nb << index(row(), col() - 1);
	nb << index(row(), col() + 1);
	
	return nb;
}

QSharedPointer<Pixel> GridPixel::toPixel() const {
	
	QSharedPointer<Pixel> px(new Pixel(mEllipse, mEllipse.bbox(), id()));
	px->setValue(mEdgeStrength);

	return px;
}

}