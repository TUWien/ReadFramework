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

#include "Pixel.h"

#include "ImageProcessor.h"
#include "Drawer.h"
#include "Shapes.h"
#include "Utils.h"
#include "Image.h"

#pragma warning(push, 0)	// no warnings from includes
#include <QColor>
#include <QDebug>
#include <QPainter>
#pragma warning(pop)

namespace rdf {


// MserBlob --------------------------------------------------------------------
MserBlob::MserBlob(const std::vector<cv::Point>& pts, 
		const Rect & bbox, 
		const QString& id) : BaseElement(id) {

	mPts = pts;
	mBBox = bbox;
	mCenter = bbox.center();	// cache center
}

bool MserBlob::isNull() const {
	return mPts.empty();
}

double MserBlob::area() const {

	return (double)mPts.size();
}

//double MserBlob::uniqueArea(const QVector<MserBlob>& blobs) const {
//
//	if (!mPts)
//		return 0;
//
//	cv::Mat m(mBBox.size().toCvSize(), CV_8UC1);
//	IP::draw(*relativePts(bbox().topLeft()), m, 1);
//
//	for (const MserBlob& b : blobs) {
//
//		// remove pixels
//		if (bbox().contains(b.bbox())) {
//			IP::draw(*b.relativePts(bbox().topLeft()), m, 0);
//		}
//	}
//
//	return cv::sum(m)[0];
//}

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

cv::Mat MserBlob::toBinaryMask() const {

	cv::Mat img(bbox().size().toCvSize(), CV_8UC1, cv::Scalar(0));
	IP::draw(relativePts(bbox().topLeft()), img, 1);

	return img;
}

QSharedPointer<Pixel> MserBlob::toPixel() const {

	Ellipse e = Ellipse::fromData(mPts);
	QSharedPointer<Pixel> p(new Pixel(e, bbox(), id()));	// assign my ID to pixel - so we can trace back that they are related

	return p;
}

double MserBlob::overlapArea(const Rect& r) const {

	if (!mBBox.contains(r))
		return 0.0;

	Vector2D tl = Vector2D::max(mBBox.topLeft(), r.topLeft());
	Vector2D br = Vector2D::min(mBBox.bottomRight(), r.bottomRight());

	double width = br.x() - tl.x();
	double height = br.y() - tl.y();

	assert(width > 0 && height > 0);

	return width * height;
}

void MserBlob::draw(QPainter & p) {

	QColor col = Drawer::instance().pen().color();
	col.setAlpha(30);
	Drawer::instance().setColor(col);
	Drawer::instance().drawPoints(p, mPts);

	// draw bounding box
	col.setAlpha(255);
	Drawer::instance().setStrokeWidth(1);
	Drawer::instance().setColor(col);
	Drawer::instance().drawRect(p, bbox().toQRectF());

	// draw center
	//Drawer::instance().setStrokeWidth(3);
	Drawer::instance().drawPoint(p, bbox().center().toQPointF());
}

// PixelStats --------------------------------------------------------------------
PixelStats::PixelStats(const cv::Mat& orHist, 
	const cv::Mat& sparsity, 
	double scale, 
	const QString& id) : BaseElement(id) {

	mScale = scale;
	convertData(orHist, sparsity);

}

void PixelStats::convertData(const cv::Mat& orHist, const cv::Mat& sparsity) {

	double lambda = 0.5;	// weights sparsity & line frequency measure
	assert(orHist.rows == sparsity.cols);

	// enrich our data
	mData = cv::Mat(idx_end, orHist.rows, CV_32FC1);
	sparsity.copyTo(mData.row(sparsity_idx));

	float* maxP = mData.ptr<float>(max_val_idx);
	float* spP = mData.ptr<float>(sparsity_idx);
	float* tlP = mData.ptr<float>(spacing_idx);
	float* cbP = mData.ptr<float>(combined_idx);

	for (int rIdx = 0; rIdx < orHist.rows; rIdx++) {

		double maxVal = 0;
		cv::Point maxIdx;
		cv::minMaxLoc(orHist.row(rIdx), 0, &maxVal, 0, &maxIdx);

		maxP[rIdx] = (float)maxVal;
		tlP[rIdx] = (float)maxIdx.x;
		cbP[rIdx] = (float)(maxVal*lambda + (1.0 - lambda) * spP[rIdx]);
	}


	// find dominant peak
	cv::Point maxIdx;
	cv::minMaxLoc(mData.row(combined_idx), 0, &mMaxVal, 0, &maxIdx);

	mOrIdx = maxIdx.x;
	mHistSize = orHist.cols;
}

bool PixelStats::isEmpty() const {
	return mData.empty();
}

int PixelStats::orientationIndex() const {

	return mOrIdx;
}

double PixelStats::orientation() const {

	return mOrIdx * CV_PI / numOrientations();
}

int PixelStats::lineSpacingIndex() const {

	if (mData.rows < spacing_idx || mOrIdx < 0 || mOrIdx >= mData.cols)
		return 0;

	return qRound(mData.ptr<float>(spacing_idx)[mOrIdx]);
}

double PixelStats::lineSpacing() const {

	// * 2.0 -> radius vs. diameter
	return (double)lineSpacingIndex()/mHistSize * mHistSize/scale() * scale() * 2.0;
}

int PixelStats::numOrientations() const {

	return mData.cols;
}

double PixelStats::maxVal() const {
	
	return mMaxVal;
}

double PixelStats::scale() const {
	
	return mScale;
}

cv::Mat PixelStats::data(const DataIndex& dIdx) {

	if (dIdx == all_data)
		return mData;

	assert(dIdx >= 0 && dIdx <= mData.cols);
	return mData.row(dIdx);
}

// Pixel --------------------------------------------------------------------
Pixel::Pixel() {

}

Pixel::Pixel(const Ellipse & ellipse, const Rect& bbox, const QString& id) : BaseElement(id) {

	mIsNull = false;
	mEllipse = ellipse;
	mBBox = bbox;
}

bool Pixel::isNull() const {
	return mIsNull;
}

Vector2D Pixel::center() const {
	return mEllipse.center();
}

Vector2D Pixel::size() const {
	return mEllipse.axis();
}

double Pixel::angle() const {
	return mEllipse.angle();
}

Ellipse Pixel::ellipse() const {
	return mEllipse;
}

Rect Pixel::bbox() const {
	return mBBox;
}


void Pixel::addStats(const QSharedPointer<PixelStats>& stats) {
	mStats << stats;
}

QSharedPointer<PixelStats> Pixel::stats(int idx) const {
	
	// select best scale by default
	if (idx == -1) {


		QSharedPointer<PixelStats> bps;
		double mv = -1.0;

		for (auto ps : mStats) {
			
			if (mv < ps->maxVal()) {
				bps = ps;
				mv = ps->maxVal();
			}
		}

		return bps;
	}

	if (idx < 0 || idx >= mStats.size()) {
		qWarning() << "cannot return PixelStats at" << idx;
		return QSharedPointer<PixelStats>();
	}
	
	return mStats[idx];
}

void Pixel::draw(QPainter & p, double alpha, bool showId) const {
	
	if (showId) {
		QPen pen = p.pen();
		p.setPen(QColor(255, 33, 33));
		p.drawText(center().toQPoint(), id());
		p.setPen(pen);
	}

	if (stats()) {


		auto s = stats();

		//if (id() == "507") {
		//	qDebug().noquote() << Image::instance().printMat<float>(s->data(PixelStats::sparsity_idx), "sparsity");
		//	qDebug().noquote() << Image::instance().printMat<float>(s->data(PixelStats::combined_idx), "combined");
		//	qDebug() << "max val: " << s->maxVal();
		//}

		QColor c(255,33,33);

		if (s->scale() == 256)
			c = ColorManager::instance().colors()[0];
		else if (s->scale() == 128)
			c = ColorManager::instance().colors()[1];
		else if (s->scale() == 64)
			c = ColorManager::instance().colors()[2];

		QPen pen = p.pen();
		p.setPen(c);

		Vector2D vec(1, 0);
		vec *= stats()->lineSpacing();
		vec.rotate(stats()->orientation());
		vec = vec + center();

		p.drawLine(Line(center(), vec).line());
		p.setPen(pen);

	}
	else
		mEllipse.draw(p, alpha);

}

PixelEdge::PixelEdge() {
}

// PixelEdge --------------------------------------------------------------------
PixelEdge::PixelEdge(const QSharedPointer<Pixel> first, 
	const QSharedPointer<Pixel> second, 
	const QString & id) : 
	BaseElement(id) {

	mIsNull = false;
	mFirst = first;
	mSecond = second;

	// cache edge
	if (mFirst && mSecond)
		mEdge = Line(mFirst->center(), mSecond->center());

}

bool PixelEdge::isNull() const {
	return mIsNull;
}

double PixelEdge::scaledEdgeLength() const {

	if (!mFirst || !mSecond)
		return 0.0;

	// get minimum scale
	double ms = qMin(mFirst->ellipse().majorAxis(), mSecond->ellipse().majorAxis());
	assert(ms > 0);

	return edge().length() / ms;
}

Line PixelEdge::edge() const {

	return mEdge;
}

QSharedPointer<Pixel> PixelEdge::first() const {
	
	return mFirst;
}

QSharedPointer<Pixel> PixelEdge::second() const {

	return mSecond;
}

void PixelEdge::draw(QPainter & p) const {

	edge().draw(p);
}

// PixelSet --------------------------------------------------------------------
PixelSet::PixelSet() {
}

bool PixelSet::contains(const QSharedPointer<Pixel>& pixel) const {

	return mSet.contains(pixel);
}

void PixelSet::merge(const PixelSet& o) {

	mSet.append(o.pixels());
}

void PixelSet::add(const QSharedPointer<Pixel>& pixel) {
	mSet << pixel;
}

QVector<QSharedPointer<Pixel> > PixelSet::pixels() const {
	return mSet;
}

Polygon PixelSet::polygon() {

	return Polygon();
}

Rect PixelSet::boundingBox() {

	double top = DBL_MAX, left = DBL_MAX;
	double bottom = -DBL_MAX, right = -DBL_MAX;

	for (const QSharedPointer<Pixel> p : mSet) {

		if (!p)
			continue;

		const Rect& r = p->bbox();

		if (r.top() < top)
			top = r.top();
		if (r.bottom() > bottom)
			bottom = r.bottom();
		if (r.left() < left)
			left = r.left();
		if (r.right() > right)
			right = r.right();
	}

	return Rect(left, top, right-left, bottom-top);
}

void PixelSet::draw(QPainter& p) {

	for (auto px : mSet)
		px->draw(p);

	//p.drawRect(boundingBox().toQRectF());
}



}