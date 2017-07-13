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
#include "Algorithms.h"
#include "Elements.h"

#pragma warning(push, 0)	// no warnings from includes
#include <QColor>
#include <QDebug>
#include <QPainter>

#include <opencv2/imgproc/imgproc.hpp>
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

	QColor col = p.pen().color();
	col.setAlpha(60);
	p.setPen(col);
	Drawer::drawPoints(p, mPts);

	// draw bounding box
	col.setAlpha(255);
	p.setPen(col);
	p.drawRect(bbox().toQRectF());

	// draw center
	p.drawPoint(bbox().center().toQPointF());
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

	double lambda = 0.5;	// weights sparsity & line frequency measure (1.0 is dft only)
	assert(orHist.rows == sparsity.cols);

	// enrich our data
	mData = cv::Mat(idx_end, orHist.rows, CV_32FC1);
	sparsity.copyTo(mData.row(sparsity_idx));

	float* maxP = mData.ptr<float>(max_val_idx);
	float* spP = mData.ptr<float>(sparsity_idx);
	float* tlP = mData.ptr<float>(spacing_idx);
	float* cbP = mData.ptr<float>(combined_idx);

	for (int rIdx = 0; rIdx < orHist.rows; rIdx++) {
		
		double minVal = 0;
		cv::Point minIdx;
		cv::minMaxLoc(orHist.row(rIdx), &minVal, 0, &minIdx);

		maxP[rIdx] = (float)minVal;
		tlP[rIdx] = (float)minIdx.x;
		cbP[rIdx] = (float)(minVal*lambda + (1.0 - lambda) * spP[rIdx]);
	}

	// find dominant peak
	cv::Point minIdx;
	cv::minMaxLoc(mData.row(combined_idx), &mMinVal, 0, &minIdx);

	mOrIdx = minIdx.x;
	mHistSize = orHist.cols;
}

bool PixelStats::isEmpty() const {
	return mData.empty();
}

void PixelStats::setOrientationIndex(int orIdx) {

	assert(orIdx >= 0 && orIdx < mData.cols);
	mOrIdx = orIdx;
}

void PixelStats::setLineSpacing(int ls) {
	mLineSpacing = ls;
}

int PixelStats::orientationIndex() const {

	return mOrIdx;
}

double PixelStats::orientation() const {

	return mOrIdx * CV_PI / numOrientations();
}

/// <summary>
/// Returns the orientation vector scaled to unit length.
/// </summary>
/// <returns>The orientation vector (0° = postive y-axis).</returns>
Vector2D PixelStats::orVec() const {
	
	Vector2D vec(1, 0);
	vec.rotate(orientation());

	return vec;
}

int PixelStats::lineSpacingIndex() const {
	
	if (mData.rows < spacing_idx || mOrIdx < 0 || mOrIdx >= mData.cols)
		return 0;

	return qRound(mData.ptr<float>(spacing_idx)[mOrIdx]);
}

double PixelStats::lineSpacing() const {

	// is a specific line spacing set? (usually from the graph-cut)
	if (mLineSpacing != -1)
		return mLineSpacing;

	double sr = (256 / scaleFactor());	// sampling rate
	return qMax((double)lineSpacingIndex() * sr, 20.0);	// assume some minimum line spacing
}

int PixelStats::numOrientations() const {
	return mData.cols;
}

double PixelStats::minVal() const {
	return mMinVal;
}

void PixelStats::scale(double factor) {
	mScale *= factor;
}

double PixelStats::scaleFactor() const {
	return mScale;
}

cv::Mat PixelStats::data(const DataIndex& dIdx) {

	if (dIdx == all_data)
		return mData;

	assert(dIdx >= 0 && dIdx <= mData.cols);
	return mData.row(dIdx);
}

QString PixelStats::toString() const {
	
	QString msg;
	msg += id();
	msg += " or idx " + QString::number(mOrIdx);
	msg += " or " + QString::number(orientation());
	msg += " spacing idx " + QString::number(lineSpacingIndex());
	//msg += " spacing " + QString::number(lineSpacing());
	
	return msg;
}

// Pixel --------------------------------------------------------------------
Pixel::Pixel() {

}

Pixel::Pixel(const Ellipse & ellipse, const Rect& bbox, const QString& id) : BaseElement(id) {

	mIsNull = false;
	mEllipse = ellipse;
	mBBox = bbox;

	if (mBBox.isNull())
		mBBox = mEllipse.bbox();
}

bool Pixel::isNull() const {
	return mIsNull;
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

void Pixel::scale(double factor) {
	mEllipse.scale(factor);
	mBBox.scale(factor);
	
	for (auto s : mStats)
		s->scale(factor);
}

void Pixel::move(const Vector2D & vec) {
	mEllipse.move(vec);
	mBBox.move(vec);
}

void Pixel::addStats(const QSharedPointer<PixelStats>& stats) {
	mStats << stats;
}

QSharedPointer<PixelStats> Pixel::stats(int idx) const {
	
	// select best scale by default
	if (idx == -1) {

		QSharedPointer<PixelStats> bps;
		double minScaleV = DBL_MAX;

		for (auto ps : mStats) {
			
			if (minScaleV > ps->minVal()) {
				bps = ps;
				minScaleV = ps->minVal();
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

void Pixel::setTabStop(const PixelTabStop& tabStop) {
	mTabStop = tabStop;
}

/// <summary>
/// Returns tab stop statistics or NULL if tab stops are not computed.
/// PixelTabStop stores if the Pixel is a tab stop (left/right) candidate or not.
/// </summary>
/// <returns>Tab stop statistics.</returns>
PixelTabStop Pixel::tabStop() const {
	return mTabStop;
}

void Pixel::setLabel(const PixelLabel & label) {
	mLabel = label;
}

PixelLabel Pixel::label() const {
	return mLabel;
}

bool operator==(const QSharedPointer<const Pixel>& px, const cv::KeyPoint & kp) {

	double angle = px->stats() ? px->stats()->orientation() : px->ellipse().angle();
	return px->ellipse().center().toCvPoint2f() == kp.pt && px->ellipse().majorAxis() == kp.size && kp.angle == angle;
}

void Pixel::setPyramidLevel(int level) {
	mPyramidLevel = level;
}

int Pixel::pyramidLevel() const {
	return mPyramidLevel;
}

cv::KeyPoint Pixel::toKeyPoint() const {

	double angle = stats() ? stats()->orientation() : mEllipse.angle();
	cv::KeyPoint kp(mEllipse.center().toCvPoint2f(), (float)mEllipse.majorAxis(), (float)angle);

	return kp;
}

void Pixel::setValue(double value) {
	mValue = value;
}

double Pixel::value() const {
	return mValue;
}

void Pixel::draw(QPainter & p, double alpha, const DrawFlags & df) const {
	
	QPen oldPen = p.pen();

	// show pixel id
	if (df & draw_id) {
		Vector2D c = center() + Vector2D(5, 3);
		p.drawText(c.toQPoint(), id());
	}

	// colorize according to labels
	if (df & draw_label_colors) {
		if (!label().trueLabel().isNull()) {
			p.setPen(label().trueLabel().visColor());
		}
		else if (!label().label().isNull()) {
			p.setPen(label().label().visColor());
		}
	}
	
	if (stats()) {

		// show local orientation
		if (df & draw_stats) {

			Vector2D vec = stats()->orVec();
			vec *= stats()->lineSpacing();
			vec = vec + center();

			p.drawLine(Line(center(), vec).line());
		}

		// indicate tab stop
		if (tabStop().type() != PixelTabStop::type_none && df & draw_tab_stops) {

			// get tab vec
			Vector2D vec = stats()->orVec();
			vec *= 40;
			vec.rotate(tabStop().orientation());

			QPen oPen = p.pen();
			p.setPen(ColorManager::red());
			p.drawLine(Line(center(), center() + vec).line());
			p.setPen(oPen);
			
		}
	}

	// scale stroke w.r.t pyramid level
	if (mPyramidLevel != 0.0) {
		QPen pen = p.pen();
		pen.setWidthF(pen.widthF()*(mPyramidLevel+1));
		p.setPen(pen);
	}

	// draw the superpixel (ellipse)
	if (df & draw_ellipse)
		mEllipse.draw(p, alpha);

	if (df & draw_center) {

		p.setRenderHint(QPainter::HighQualityAntialiasing);
		Ellipse e = mEllipse;
		e.setAxis(Vector2D(1, 1));
		e.draw(p, alpha);
	}

	p.setPen(oldPen);
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

bool operator<(const PixelEdge & pe1, const PixelEdge & pe2) {
	return pe1.lessThan(pe2);
}

bool operator<(const QSharedPointer<PixelEdge> & pe1, const QSharedPointer<PixelEdge> & pe2) {
	return pe1->lessThan(*pe2);
}

bool PixelEdge::isNull() const {
	return mIsNull;
}

void PixelEdge::setEdgeWeightFunction(PixelDistance::EdgeWeightFunction & fnc) {
	mWeightFnc = fnc;
}

double PixelEdge::edgeWeightConst() const {
	
	if (mEdgeWeight != DBL_MAX)
		return mEdgeWeight;
	
	return mWeightFnc(this);
}

double PixelEdge::edgeWeight() {
	
	if (mEdgeWeight == DBL_MAX)
		mEdgeWeight = edgeWeightConst();	// cache it
	
	return mEdgeWeight;
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

void PixelEdge::scale(double s) {
	mEdge.scale(s);
}

void PixelEdge::draw(QPainter & p) const {

	Line e = edge();
	e.draw(p);
}

bool PixelEdge::lessThan(const PixelEdge & e) const {
	return edgeWeightConst() < e.edgeWeightConst();
}

// LineEdge --------------------------------------------------------------------
LineEdge::LineEdge() {
}

LineEdge::LineEdge(const PixelEdge & pe) : PixelEdge(pe) {
	mEdgeWeight = calcWeight();
}

LineEdge::LineEdge(
	const QSharedPointer<Pixel> first, 
	const QSharedPointer<Pixel> second, 
	const QString & id) : 
	PixelEdge(first, second, id) {

	mEdgeWeight = calcWeight();
}

bool operator<(const QSharedPointer<LineEdge>& le1, const QSharedPointer<LineEdge>& le2) {
	return le1->lessThan(*le2);
}

double LineEdge::edgeWeightConst() const {
	return mEdgeWeight;
}

double LineEdge::calcWeight() const {

	assert(mFirst && mSecond);
	double d1 = statsWeight(mFirst);
	double d2 = statsWeight(mSecond);

	double d = qMax(abs(d1), abs(d2));
	
	// normalize by scale
	double mr = qMin(mFirst->ellipse().radius(), mSecond->ellipse().radius());
	if (mr > 0)
		d /= mr;

	return d;
}

double LineEdge::statsWeight(const QSharedPointer<Pixel>& pixel) const {

	if (!pixel)
		return DBL_MAX;

	if (!pixel->stats()) {
		qWarning() << "pixel stats are NULL where they must not be...";
		return DBL_MAX;
	}

	Vector2D vec = pixel->stats()->orVec();
	Vector2D eVec = edge().vector();

	return vec * eVec;
}

}
