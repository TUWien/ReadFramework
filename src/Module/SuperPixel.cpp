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

#include "PixelSet.h"

#include "GCGraph.hpp"

#pragma warning(push, 0)	// no warnings from includes
#include <QDebug>

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d/features2d.hpp>

#include "graphcut/GCoptimization.h"

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
	int nF = filterDuplicates(*blobs, 7, 10);
	qDebug() << "[duplicates filter]\tremoves" << nF << "blobs in" << dtf;

	dtf.start();
	nF = filterAspectRatio(*blobs);
	qDebug() << "[aspect ratio filter]\tremoves" << nF << "blobs in" << dtf;

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
	QPixmap pm = Image::mat2QPixmap(img);
	QPainter p(&pm);


	//DBScanPixel dbs(mPixels);
	//dbs.compute();
	//QVector<PixelSet> sets = dbs.sets();
	//qDebug() << "dbscan found" << sets.size() << "clusters in" << dtf;

	//for (auto s : sets) {
	//	Drawer::instance().setColor(ColorManager::getRandomColor());
	//	QPen pen = Drawer::instance().pen();
	//	p.setPen(pen);
	//	s.draw(p);
	//}

	for (int idx = 0; idx < mBlobs.size(); idx++) {
		Drawer::instance().setColor(ColorManager::getRandomColor());
		QPen pen = Drawer::instance().pen();
		pen.setWidth(2);
		p.setPen(pen);

		//// uncomment if you want to see MSER & SuperPixel at the same time
		//mBlobs[idx].draw(p);
		mPixels[idx]->draw(p, 0.2, Pixel::draw_ellipse_only);
		//qDebug() << mPixels[idx].ellipse();
	}

	qDebug() << "drawing takes" << dtf;
	return Image::qPixmap2Mat(pm);
}

cv::Mat SuperPixel::drawMserBlobs(const cv::Mat & img) const {

	// draw mser blobs
	Timer dtf;
	QPixmap pm = Image::mat2QPixmap(img);
	QPainter p(&pm);

	for (auto b : mBlobs) {
		Drawer::instance().setColor(ColorManager::getRandomColor());
		p.setPen(Drawer::instance().pen());

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

	QVector<Pixel*> ptrSet;
	for (const QSharedPointer<Pixel> p : mSet)
		ptrSet << p.data();

	for (Pixel* p : ptrSet)
		computeScales(p, ptrSet);

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
	cv::Mat orHist(nOr, histSize, CV_32FC1, cv::Scalar(0));
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
	//orHist.setTo(0);
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
		
		if (cIdx >= 10) {
			// see Koo16: val = -log( (val*val) / (hist[0]*hist[0]) + 1.0);
			orPtr[cIdx] *= orPtr[cIdx];
			orPtr[cIdx] /= normValSq;
			orPtr[cIdx] += 1.0f;	// for log
			orPtr[cIdx] = std::log(orPtr[cIdx]) * -1.0f;
		}
		else {
			// remove very low frequencies - they might create larger peaks than the recurring frequency
			orPtr[cIdx] = 0.0f;
		}
	}
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
	QPixmap pm = Image::mat2QPixmap(img);
	QPainter painter(&pm);

	Ellipse e(pixel->center(), Vector2D(radius, radius));
	e.draw(painter, 0.3);
	painter.setPen(ColorManager::colors()[2]);

	const Vector2D& ec = pixel->center();
	QVector<const Pixel*> neighbors;

	// create neighbor set
	for (const QSharedPointer<Pixel>& p : mSet) {
		
		if (ec.isNeighbor(p->center(), radius)) {
			neighbors << p.data();
			p->draw(painter, 0.3, Pixel::draw_ellipse_stats);
		}
	}

	// draw the selected pixel in a different color
	painter.setPen(ColorManager::colors()[0]);
	pixel->draw(painter, 0.3, Pixel::draw_all);

	// compute all orientations
	int histSize = config()->histSize();
	int nOr = config()->numOrientations();
	cv::Mat orHist(nOr, histSize, CV_32FC1, cv::Scalar(0));
	
	for (int k = 0; k < nOr; k++) {

		// create orientation vector
		float sp = 0.0f;	// not needed
		double cAngle = k * CV_PI / nOr;
		Vector2D orVec(radius, 0);
		orVec.rotate(cAngle);

		cv::Mat cRow = orHist.row(k);
		computeOrHist(pixel.data(), neighbors, orVec, cRow, sp);

		//qDebug().noquote() << Image::printImage(cRow, "row" + QString::number(cAngle * DK_RAD2DEG));

		rdf::Histogram h(cRow);
		Rect r(30 + k * (histSize+5), pixel->center().y()-radius-150, histSize, 50);
		h.draw(painter, r);
		painter.drawText(r.bottomLeft().toQPoint(), QString::number(cAngle * DK_RAD2DEG));
	}

	return Image::qPixmap2Mat(pm);
}


// GraphCutOrientation --------------------------------------------------------------------
GraphCutOrientation::GraphCutOrientation(const QVector<QSharedPointer<Pixel>>& set) {
	mSet = set;
}

bool GraphCutOrientation::isEmpty() const {
	return mSet.isEmpty();
}

bool GraphCutOrientation::compute() {
	
	if (!checkInput())
		return false;

	DelauneyPixelConnector dpc;

	Timer dt;
	PixelGraph graph(mSet);
	graph.connect(dpc);
	graphCut(graph);
	qInfo() << "[Graph Cut] computed in" << dt;

	return true;
}

QVector<QSharedPointer<Pixel>> GraphCutOrientation::getSuperPixels() const {
	
	return mSet;
}

bool GraphCutOrientation::checkInput() const {
	return !isEmpty();
}

void GraphCutOrientation::graphCut(const PixelGraph& graph) {

	if (graph.isEmpty())
		return;

	int gcIter = 2;	// # iterations of graph-cut (expansion)

	// stats must be computed already
	QVector<QSharedPointer<Pixel> > pixel = graph.set()->pixels();
	assert(pixel[0]->stats());

	// the statistics columns == the number of possible labels
	int nLabels = graph.set()->pixels()[0]->stats()->data().cols;
	
	// get costs and smoothness term
	cv::Mat c = costs(nLabels);					 // SetSize x #labels
	cv::Mat sm = orientationDistMatrix(nLabels); // #labels x #labels

	// init the graph
	QSharedPointer<GCoptimizationGeneralGraph> graphCut(new GCoptimizationGeneralGraph(pixel.size(), nLabels));
	graphCut->setDataCost(c.ptr<int>());
	graphCut->setSmoothCost(sm.ptr<int>());

	// create neighbors
	const QVector<QSharedPointer<PixelEdge> >& edges = graph.edges();
	for (int idx = 0; idx < pixel.size(); idx++) {

		for (int edgeIdx : graph.edgeIndexes(pixel.at(idx)->id())) {

			assert(edgeIdx != -1);

			// get vertex ID
			const QSharedPointer<PixelEdge>& pe = edges[edgeIdx];
			int sVtxIdx = graph.pixelIndex(pe->second()->id());
			
			// compute weight
			int w = qRound((1.0-pe->edgeWeight()) * mScaleFactor);

			graphCut->setNeighbors(idx, sVtxIdx, w);
		}
	}

	// run the expansion-move
	graphCut->expansion(gcIter);

	for (int idx = 0; idx < pixel.size(); idx++) {

		auto ps = pixel[idx]->stats();
		ps->setOrientationIndex(graphCut->whatLabel(idx));
	}

}

cv::Mat GraphCutOrientation::costs(int numLabels) const {
	
	// fill costs
	cv::Mat data(mSet.size(), numLabels, CV_32SC1);

	for (int idx = 0; idx < mSet.size(); idx++) {

		auto ps = mSet[idx]->stats();
		assert(ps);

		cv::Mat cData = ps->data(PixelStats::combined_idx);
		cData.convertTo(data.row(idx), CV_32SC1, mScaleFactor);	// TODO: check scaling
	}

	return data;
}

cv::Mat GraphCutOrientation::orientationDistMatrix(int numLabels) const {
	
	cv::Mat orDist(numLabels, numLabels, CV_32SC1);

	for (int rIdx = 0; rIdx < orDist.rows; rIdx++) {

		unsigned int* sPtr = orDist.ptr<unsigned int>(rIdx);

		for (int cIdx = 0; cIdx < orDist.cols; cIdx++) {

			// set smoothness cost for orientations
			int diff = abs(rIdx - cIdx);
			sPtr[cIdx] = qMin(diff, numLabels - diff);
		}
	}

	return orDist;
}

}