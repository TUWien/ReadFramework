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

	// * 2.0 -> radius vs. diameter
	return (double)lineSpacingIndex()/mHistSize * mHistSize/scale() * scale() * 2.0;
}

int PixelStats::numOrientations() const {

	return mData.cols;
}

double PixelStats::minVal() const {
	
	return mMinVal;
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

//Vector2D Pixel::center() const {
//	return mEllipse.center();
//}

Vector2D Pixel::size() const {
	return mEllipse.axis();
}

double Pixel::angle() const {
	return mEllipse.angle();
}

Ellipse Pixel::ellipse() const {
	return mEllipse;
}

//Rect Pixel::bbox() const {
//	return mBBox;
//}


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

void Pixel::draw(QPainter & p, double alpha, const DrawFlag & df) const {
	
	if (df == draw_all) {
		QPen pen = p.pen();
		p.setPen(QColor(255, 33, 33));
		p.drawText(center().toQPoint(), id());
		p.setPen(pen);
	}

	if (stats() && (df != draw_ellipse_only)) {

		auto s = stats();

		//QColor c(255,33,33);

		//if (s->scale() == 256)
		//	c = ColorManager::instance().colors()[0];
		//else if (s->scale() == 128)
		//	c = ColorManager::instance().colors()[1];
		//else if (s->scale() == 64)
		//	c = ColorManager::instance().colors()[2];

		//QPen pen = p.pen();
		//p.setPen(c);

		Vector2D vec = stats()->orVec();
		vec *= stats()->lineSpacing();
		
		vec = vec + center();

		p.drawLine(Line(center(), vec).line());
		//p.setPen(pen);
	}

	if (stats() && tabStop().type() != PixelTabStop::type_none) {

		// get tab vec
		Vector2D vec = stats()->orVec();
		vec *= 40;
		vec.rotate(tabStop().orientation());

		QPen oPen = p.pen();
		p.setPen(ColorManager::instance().red());
		p.drawLine(Line(center(), center()+vec).line());
		p.setPen(oPen);
	}

	//if (stats()) {
	//	QPen pe = p.pen();
	//	p.setPen(QColor(255, 0, 0));
	//	Vector2D upper = mEllipse.getPoint(stats()->orientation());
	//	Vector2D lower = mEllipse.getPoint(stats()->orientation() + CV_PI);
	//	p.drawLine(upper.toQPointF(), lower.toQPointF());
	//	p.setPen(pe);
	//}

	if (!stats() || df != draw_stats_only)
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

double PixelEdge::edgeWeight() const {

	if (!mFirst || !mSecond)
		return 0.0;

	double beta = 1.0;

	if (mFirst->stats() && mSecond->stats()) {
	
		double sp = mFirst->stats()->lineSpacing();
		double sq = mSecond->stats()->lineSpacing();
		double nl = (beta * edge().squaredLength()) / (sp * sp + sq * sq);
		double ew = 1.0-exp(-nl);

		if (ew < 0.0 || ew > 1.0) {
			qDebug() << "illegal edge weight: " << ew;
		}
		//else
		//	qDebug() << "weight: " << nl;

		// TODO: add mu(fp,fq) according to koo's indices
		return ew;
	}

	qDebug() << "no stats when computing the scaled edges...";

	return 0.0;

	//// get minimum scale
	//double ms = qMin(mFirst->ellipse().majorAxis(), mSecond->ellipse().majorAxis());
	//assert(ms > 0);

	//return edge().length() / ms;
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

// LineEdge --------------------------------------------------------------------
LineEdge::LineEdge() {
}

LineEdge::LineEdge(const PixelEdge & pe) : PixelEdge(pe) {
	mStatsWeight = calcWeight();
}

LineEdge::LineEdge(
	const QSharedPointer<Pixel> first, 
	const QSharedPointer<Pixel> second, 
	const QString & id) : 
	PixelEdge(first, second, id) {

	mStatsWeight = calcWeight();
}

double LineEdge::edgeWeight() const {
	return mStatsWeight;
}

double LineEdge::calcWeight() const {
	double d1 = statsWeight(mFirst);
	double d2 = statsWeight(mSecond);

	return qMax(abs(d1), abs(d2));
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

// PixelSet --------------------------------------------------------------------
PixelSet::PixelSet() {
}

PixelSet::PixelSet(const QVector<QSharedPointer<Pixel> >& set) {
	mSet = set;
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

void PixelSet::remove(const QSharedPointer<Pixel>& pixel) {
	
	int pIdx = mSet.indexOf(pixel);

	if (pIdx != -1)
		mSet.remove(pIdx);
	else
		qWarning() << "cannot remove a pixel which is not in the set:" << pixel->id();
}

QVector<QSharedPointer<Pixel> > PixelSet::pixels() const {
	return mSet;
}

Polygon PixelSet::polygon() {

	qWarning() << "PixelSet::polygon() not implemented yet";
	return Polygon();
}

Rect PixelSet::boundingBox() const {

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

Line PixelSet::baseline(double offsetAngle) const {

	if (mSet.empty()) {
		qWarning() << "cannot compute baseline if the set is empty...";
		return Line();
	}

	std::vector<cv::Point> lowerProfile;

	for (auto p : mSet) {
		if (p->stats()) {
			double angle = p->stats()->orientation() + offsetAngle;
			lowerProfile.push_back(p->ellipse().getPoint(angle).toCvPoint());	// get ellipse point at the bottom
		}
	}

	cv::Vec4f lowerLineVec;
	cv::fitLine(lowerProfile, lowerLineVec, CV_DIST_WELSCH, 0, 10, 0.01);

	Vector2D x0(lowerLineVec[2], lowerLineVec[3]);
	Vector2D x1 = x0 + Vector2D(lowerLineVec[0], lowerLineVec[1]) * 200.0;
	
	Line baseLine(x0, x1);
	baseLine = baseLine.extendBorder(boundingBox());

	//// reject baseline?
	//// TODO: calculate the mean angle
	//double meanAngle = mSet.empty() || mSet[0]->stats() ? 0.0 : mSet[0]->stats()->orientation();

	//if (abs(Algorithms::instance().angleDist(lowerLine.angle(), meanAngle)) > CV_PI*0.1)
	//	return BaseLine();

	return baseLine;
}

Ellipse PixelSet::profileRect() const {
	
	// TODO: this is not fixed yet
	Line bLine = baseline();
	if (bLine.isEmpty())
		return Ellipse();

	Line xLine = baseline(CV_PI);

	double angle = (bLine.angle() + Algorithms::instance().angleDist(xLine.angle(), bLine.angle())*0.5f);
	
	Vector2D blc = bLine.p1() + bLine.vector() * 0.5;
	Vector2D xlc = xLine.p1() + xLine.vector() * 0.5;
	Vector2D centerLine = blc - xlc;
	Vector2D center = xlc + centerLine * 0.5;

	Ellipse el;
	el.setAngle(angle);
	el.setCenter(center);
	el.setAxis(Vector2D(std::max(bLine.length(), xLine.length()), centerLine.length()));

	return el;
}

QVector<QSharedPointer<PixelEdge> > PixelSet::connect(const QVector<QSharedPointer<Pixel> >& superPixels, const Rect& rect, const ConnectionMode& mode) {

	switch (mode) {
	case connect_delauney: {
		return connectDelauney(superPixels, rect);
	}
	case connect_region: {
		return connectRegion(superPixels);
	}
	case connect_tab_stops: {
		return connectTabStops(superPixels);
	}
	}

	qWarning() << "unkown mode in PixelSet::connect - mode: " << mode;
	return connectDelauney(superPixels, rect);
}

QVector<QSharedPointer<PixelEdge> > PixelSet::connectDelauney(const QVector<QSharedPointer<Pixel> >& superPixels, const Rect& rect) {
	
	// Create an instance of Subdiv2D
	cv::Subdiv2D subdiv(rect.toCvRect());

	QVector<int> ids;
	for (const QSharedPointer<Pixel>& b : superPixels)
		ids << subdiv.insert(b->center().toCvPointF());

	// that took me long... but this is how get can map the edges to our objects without an (expensive) lookup
	QVector<QSharedPointer<PixelEdge> > edges;
	for (int idx = 0; idx < (superPixels.size()-8)*3; idx++) {

		int orgVertex = ids.indexOf(subdiv.edgeOrg((idx << 2)));
		int dstVertex = ids.indexOf(subdiv.edgeDst((idx << 2)));

		// there are a few edges that lead to nowhere
		if (orgVertex == -1 || dstVertex == -1) {
			continue;
		}

		assert(orgVertex >= 0 && orgVertex < superPixels.size());
		assert(dstVertex >= 0 && dstVertex < superPixels.size());

		QSharedPointer<PixelEdge> pe(new PixelEdge(superPixels[orgVertex], superPixels[dstVertex]));
		edges << pe;
	}

	return edges;
}

/// <summary>
/// Fully connected graph.
/// Super pixels are connected with all other super pixels within a region 
/// of radius lineSpacing() * multiplier.
/// </summary>
/// <param name="superPixels">The super pixels.</param>
/// <param name="multiplier">Factor that is multiplied to the lineSpacing value for local neighborhood.</param>
/// <returns>Connecting edges.</returns>
QVector<QSharedPointer<PixelEdge> > PixelSet::connectRegion(const QVector<QSharedPointer<Pixel> >& superPixels, double multiplier) {
	
	QVector<QSharedPointer<PixelEdge> > edges;

	for (const QSharedPointer<Pixel>& px : superPixels) {

		if (!px->stats()) {
			qWarning() << "pixel stats are NULL where they should not be: " << px->id();
			continue;
		}

		double cR = px->stats()->lineSpacing() * multiplier;
		const Vector2D& pxc = px->center();

		for (const QSharedPointer<Pixel>& npx : superPixels) {

			if (npx->id() == px->id())
				continue;

			if (pxc.isNeighbor(npx->center(), cR))
				edges << QSharedPointer<PixelEdge>::create(px, npx);

		}
	}

	return edges;
}

QVector<QSharedPointer<PixelEdge> > PixelSet::connectTabStops(const QVector<QSharedPointer<Pixel>>& superPixels, double multiplier) {

	QVector<QSharedPointer<PixelEdge> > edges;

	for (const QSharedPointer<Pixel>& px : superPixels) {

		if (!px->stats()) {
			qWarning() << "pixel stats are NULL where they should not be: " << px->id();
			continue;
		}

		double cR = px->stats()->lineSpacing() * multiplier;
		double tOr = px->stats()->orientation() - px->tabStop().orientation();
		const Vector2D& pxc = px->center();

		// tab's orientation vector
		Vector2D orVec = px->stats()->orVec();
		orVec.rotate(px->tabStop().orientation());

		QList<double> dists;
		QVector<QSharedPointer<PixelEdge> > cEdges;

		for (const QSharedPointer<Pixel>& npx : superPixels) {

			if (npx->id() == px->id())
				continue;

			double cOr = npx->stats()->orientation() - npx->tabStop().orientation();

			// are the tab-stop orientations the same?? and are both pixels within the the currently defined radius?
			if (Algorithms::instance().angleDist(tOr, cOr) < .1 &&		// do we have the same orientation?
				pxc.isNeighbor(npx->center(), cR)) {						// is the other pixel in a local environment
				
				QSharedPointer<PixelEdge> edge = QSharedPointer<PixelEdge>::create(px, npx);
			
				// normalize distance with tab orientation
				double ea = orVec * edge->edge().vector();
				dists << ea;
				cEdges << edge;
			}

		}

		if (cEdges.size() > 2) {
			// only take the closest 10%
			double q1 = Algorithms::statMoment(dists, 0.1);

			for (int idx = 0; idx < dists.size(); idx++) {

				if (dists[idx] <= q1)
					edges << cEdges[idx];
			}
		}
		else
			edges << cEdges;

	}

	return edges;
}

QVector<QSharedPointer<PixelSet> > PixelSet::fromEdges(const QVector<QSharedPointer<PixelEdge> >& edges) {

	QVector<QSharedPointer<PixelSet> > sets;

	for (const QSharedPointer<PixelEdge> e : edges) {

		int fIdx = -1;
		int sIdx = -1;

		for (int idx = 0; idx < sets.size(); idx++) {

			if (sets[idx]->contains(e->first()))
				fIdx = idx;
			if (sets[idx]->contains(e->second()))
				sIdx = idx;

			if (fIdx != -1 && sIdx != -1)
				break;
		}

		// none is contained in a set
		if (fIdx == -1 && sIdx == -1) {
			QSharedPointer<PixelSet> ps(new PixelSet());
			ps->add(e->first());
			ps->add(e->second());
			sets << ps;
		}
		// add first to the set of second
		else if (fIdx == -1) {
			sets[sIdx]->add(e->first());
		}
		// add second to the set of first
		else if (sIdx == -1) {
			sets[fIdx]->add(e->second());
		}
		// two different idx? - merge the sets
		else if (fIdx != sIdx) {
			sets[fIdx]->merge(*sets[sIdx]);
			sets.remove(sIdx);
		}
		// else : nothing to do - they are both already added

	}

	return sets;
}

void PixelSet::draw(QPainter& p) const {

	for (auto px : mSet)
		px->draw(p, 0.3, Pixel::draw_ellipse_only);

	p.drawRect(boundingBox().toQRectF());
}

// PixelGraph --------------------------------------------------------------------
PixelGraph::PixelGraph() {

}

PixelGraph::PixelGraph(const QVector<QSharedPointer<Pixel>>& set) {
	mSet = QSharedPointer<PixelSet>::create(set);
}

bool PixelGraph::isEmpty() const {
	return !mSet || mSet->pixels().isEmpty();
}

void PixelGraph::draw(QPainter& p) const {

	p.setPen(ColorManager::instance().colors()[0]);
	for (auto px : mSet->pixels())
		px->draw(p, 0.3, Pixel::draw_ellipse_only);

	p.setPen(ColorManager::instance().darkGray(.4));
	for (auto e : edges())
		e->draw(p);

}

void PixelGraph::connect(const Rect & rect, const PixelSet::ConnectionMode& mode) {

	// nothing todo?
	if (isEmpty())
		return;

	assert(mSet);
	const QVector<QSharedPointer<Pixel> >& pixels = mSet->pixels();
	mEdges = PixelSet::connect(pixels, rect, mode);

	// edge lookup (maps pixel IDs to their corresponding edge index) this is a 1 ... n relationship
	for (int idx = 0; idx < mEdges.size(); idx++) {

		QString key = mEdges[idx]->first()->id();

		QVector<int> v = mPixelEdges.value(key);
		v << idx;

		mPixelEdges.insert(key, v);
	}

	// pixel lookup (maps pixel IDs to their current vector index)
	for (int idx = 0; idx < pixels.size(); idx++) {
		mPixelLookup.insert(pixels[idx]->id(), idx);
	}

}

QSharedPointer<PixelSet> PixelGraph::set() const {
	return mSet;
}

/// <summary>
/// Returns all pixel edges which were found using Delauney triangulation.
/// </summary>
/// <returns>A vector of PixelEdges which connect 2 pixels each.</returns>
QVector<QSharedPointer<PixelEdge> > PixelGraph::edges() const {

	return mEdges;
}

/// <summary>
/// Returns all edges connected to the pixel with ID pixelID.
/// </summary>
/// <param name="pixelID">The pixel identifier.</param>
/// <returns></returns>
QVector<QSharedPointer<PixelEdge>> PixelGraph::edges(const QString & pixelID) const {

	return edges(edgeIndexes(pixelID));
}

/// <summary>
/// Returns a vector with all edges in the ID vector edgeIDs.
/// </summary>
/// <param name="edgeIDs">The edge IDs.</param>
/// <returns></returns>
QVector<QSharedPointer<PixelEdge> > PixelGraph::edges(const QVector<int>& edgeIDs) const {

	QVector<QSharedPointer<PixelEdge> > pe;
	for (int eId : edgeIDs) {
		assert(eId >= 0 && eId < mEdges.length());
		pe << mEdges[eId];
	}

	return pe;
}

/// <summary>
/// Maps pixel IDs (pixel->id()) to their current vector index.
/// </summary>
/// <param name="pixelID">The unique ID of a pixel.</param>
/// <returns>The current vector position.</returns>
int PixelGraph::pixelIndex(const QString & pixelID) const {
	return mPixelLookup.value(pixelID);
};

/// <summary>
/// Returns all edges indexes of the current pixel.
/// Each pixel has N edges in the graph. This function
/// returns a vector with all edge indexes of the current pixel.
/// The edge objects can then be retreived using edges()[idx].
/// </summary>
/// <param name="pixelID">Unique pixel ID.</param>
/// <returns>A vector with edge indexes.</returns>
QVector<int> PixelGraph::edgeIndexes(const QString & pixelID) const {
	return mPixelEdges.value(pixelID);
};

// PixelTabStop --------------------------------------------------------------------
PixelTabStop::PixelTabStop(const Type & type) {
	mType = type;
}

PixelTabStop PixelTabStop::create(const QSharedPointer<Pixel>& pixel, const QVector<QSharedPointer<PixelEdge> >& edges) {

	// parameter
	double epsilon = 0.1;	// what we consider to be orthogonal
	double minEdgeLength = 10;
	double neighborRel = 0.3;	// horizontal neighbor relation - if 0.5 -> min left neighbor must be at least 50% the distance of min right neighbor

	Vector2D pVec = pixel->stats()->orVec();
	pVec.rotate(CV_PI*0.5);

	int vEdgeCnt = 0;

	QVector<double> cwe;	// clock-wise edges (0°)
	QVector<double> cce;	// counter-clockwise edges (180°)

	for (QSharedPointer<PixelEdge> e : edges) {

		Vector2D eVec = e->edge().vector();

		if (eVec.length() < minEdgeLength)
			continue;

		double theta = pVec * eVec / (pVec.length() * eVec.length());
		//qDebug() << "theta: " << theta;

		if (abs(1.0 - abs(theta)) < .25) {

			if (theta > 0)
				cwe << abs(theta*eVec.length());
			else
				cce << abs(theta*eVec.length());
		}
		else if (abs(theta) < epsilon)
			vEdgeCnt++;
	}

	PixelTabStop::Type mode = PixelTabStop::type_none;
	

	if (cwe.empty() && !cce.empty()) {
		mode = PixelTabStop::type_right;
	}
	else if (!cwe.empty() && cce.empty()) {
		mode = PixelTabStop::type_left;
	}
	else {

		//if (vEdgeCnt >= 1) {

			double minCC = Algorithms::instance().min(cce);
			double minCW = Algorithms::instance().min(cwe);

			if (minCC / minCW < neighborRel)
				mode = PixelTabStop::type_right;
			else if (minCW / minCC < neighborRel)
				mode = PixelTabStop::type_left;
		//}
	}

	return PixelTabStop(mode);
}

/// <summary>
/// Returns the angle corresponding to the tab stop.
/// Hence, if this angle is applied to the pixel's orientation,
/// the vector points into the tab stops empty region (away from the textline).
/// </summary>
/// <returns></returns>
double PixelTabStop::orientation() const {

	if (type() == type_left)
		return CV_PI*0.5;
	else if (type() == type_right)
		return -CV_PI*0.5;

	return 0.0;
}

PixelTabStop::Type PixelTabStop::type() const {
	return mType;
}


}