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

#include "PixelSet.h"
#include "Pixel.h"
#include "Algorithms.h"
#include "Elements.h"
#include "Utils.h"

#pragma warning(push, 0)	// no warnings from includes
#include <QDebug>
#include <QPainter>

#include <opencv2/imgproc/imgproc.hpp>
#pragma warning(pop)

namespace rdf {


// PixelSet --------------------------------------------------------------------
PixelSet::PixelSet() {
}

PixelSet::PixelSet(const QVector<QSharedPointer<Pixel> >& set) {
	mSet = set;
}

QSharedPointer<Pixel> PixelSet::operator[](int idx) const {
	assert(idx >= 0 && idx < size());
	return mSet[idx];
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

int PixelSet::size() const {
	return mSet.size();
}

/// <summary>
/// Returns a point set.
/// The points are sampled along the pixel's ellipses w.r.t
/// the relative angle which is added to the text line orientation.
/// If angle == DBL_MAX or pixel->stats() == NULL, the ellipse centers
/// are sampled instead.
/// </summary>
/// <param name="offsetAngle">The angle which is added to the pixel's local orientation.</param>
/// <returns></returns>
QVector<Vector2D> PixelSet::pointSet(double offsetAngle) const {
	
	QVector<Vector2D> ptSet;

	for (auto p : mSet) {
		if (p->stats() && offsetAngle != DBL_MAX) {
			double angle = p->stats()->orientation() + offsetAngle;
			ptSet << p->ellipse().getPoint(angle);	// get ellipse point at the bottom
		}
		else
			ptSet << p->ellipse().center();
	}

	return ptSet;
}

Polygon PixelSet::convexHull() const {
	
	QVector<Vector2D> pts = pointSet(0.0);
	pts << pointSet(CV_PI*0.5);
	pts << pointSet(CV_PI);
	pts << pointSet(-CV_PI*0.5);
	
	return polygon(pts);
}

/// <summary>
/// Returns a poly line along the points defined by angle.
/// If maxAngleThr != -1, points are rejected if the cosine of their enclosing angle
/// is smaller than maxAngleThr. e.g. maxAngleThr = 0 would allow polygon changes
/// of up to 90°.
/// </summary>
/// <param name="angle">The angle for sampling the ellipses.</param>
/// <param name="maxAngleThr">The maximum angle thresh.</param>
/// <returns></returns>
//Polygon PixelSet::polyLine(double, double) const {
//	
//	// TODO!
//	//QVector<Vector2D> pts = pointSet(angle);
//
//	//QPolygonF poly = polygon(pts).polygon();
//
//	//QList<double> len;
//	//Vector2D lp;
//	//for (const QPointF& p : poly) {
//
//	//	if (!lp.isNull())
//	//		len << Vector2D(lp - p).length();
//
//	//	lp = p;
//	//}
//
//	//double maxLen = Algorithms::statMoment(len, 0.9);
//
//	//Polygon cleanedPoly;
//	//for (int idx = 0; idx < len.size(); idx++) {
//	//	if (len[idx] < maxLen)
//	//		cleanedPoly << poly[idx + 1];
//	//}
//
//	//cleanedPoly = poly;
//
//	//qDebug() << "max length: " << maxLen;
//	//Polygon poly;
//	//Vector2D p1, p2;
//	//for (const Vector2D& p0 : pts) {
//
//	//	bool isValid = true;
//	//	if (!p1.isNull() && !p2.isNull()) {
//
//	//		Vector2D lVec = p1 - p2;
//	//		Vector2D rVec = p0 - p1;
//	//		double ct = rVec.theta(lVec);
//	//		
//	//		if (ct < maxCosThr) {
//	//			isValid = false;
//	//			qInfo() << "point dropped, angle: " << ct;
//	//		} 
//	//	}
//
//	//	if (isValid) {
//	//		poly << p0;
//	//		p2 = p1;
//	//		p1 = p0;
//	//		qDebug() << "pushing back: " << p0;
//	//	}
//	//}
//
//	return Polygon();
//}

/// <summary>
/// Returns the convex hull of the PixelSet.
/// </summary>
/// <returns></returns>
Polygon PixelSet::polygon(const QVector<Vector2D>& pts) const {

	// convert
	std::vector<cv::Point> ptsCv;
	for (const Vector2D& pt : pts) {
		ptsCv.push_back(pt.toCvPoint());
	}

	if (pts.empty())
		return Polygon();

	// compute convex hull
	std::vector<cv::Point> cPts;
	cv::convexHull(ptsCv, cPts, true);

	Polygon poly = Polygon::fromCvPoints(cPts);

	return poly;
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

Line PixelSet::fitLine(double offsetAngle) const {

	if (mSet.empty()) {
		qWarning() << "cannot compute baseline if the set is empty...";
		return Line();
	}

	QVector<Vector2D> ptSet = pointSet(offsetAngle);

	LineFitting lf(ptSet);
	Line line = lf.fitLineLMS();
	line = line.extendBorder(boundingBox());

	return line;
}

Ellipse PixelSet::profileRect() const {
	
	// TODO: this is not fixed yet
	Line bLine = fitLine();
	if (bLine.isEmpty())
		return Ellipse();

	Line xLine = fitLine(CV_PI);

	double angle = (bLine.angle() + Algorithms::angleDist(xLine.angle(), bLine.angle())*0.5f);
	
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

/// <summary>
/// Computes the sets median orientation.
/// </summary>
/// <param name="statMoment">if != 0.5 a quartile other than the median is computed.</param>
/// <returns>The set's median orientation.</returns>
double PixelSet::orientation(double statMoment) const {
	
	QList<double> angles;
	for (const QSharedPointer<Pixel>& px : pixels()) {

		if (!px->stats()) {
			qWarning() << "stats is NULL where it should not be...";
			continue;
		}
		angles << px->stats()->orientation();
	}

	return Algorithms::statMoment(angles, statMoment);
}

double PixelSet::lineSpacing(double statMoment) const {

	QList<double> spacings;
	for (const QSharedPointer<Pixel>& px : pixels()) {

		if (!px->stats()) {
			qWarning() << "stats is NULL where it should not be...";
			continue;
		}
		spacings << px->stats()->lineSpacing();
	}

	return Algorithms::statMoment(spacings, statMoment);
}

/// <summary>
/// This is a convenience function that connects SuperPixels.
/// It is recommended to use the connector classes respectively.
/// </summary>
/// <param name="superPixels">The super pixels.</param>
/// <param name="rect">The bounding rect.</param>
/// <param name="mode">The connection mode.</param>
/// <returns></returns>
QVector<QSharedPointer<PixelEdge> > PixelSet::connect(const QVector<QSharedPointer<Pixel> >& superPixels, const ConnectionMode& mode) {

	QSharedPointer<PixelConnector> connector;

	switch (mode) {
	case connect_delauney: {
		connector = QSharedPointer<PixelConnector>(new DelauneyPixelConnector());
		break;
	}
	case connect_region: {
		connector = QSharedPointer<PixelConnector>(new RegionPixelConnector());
		break;
	}
	}

	if (!connector) {
		qWarning() << "unkown mode in PixelSet::connect - mode: " << mode;
		connector = QSharedPointer<PixelConnector>(new DelauneyPixelConnector());
	}
	return connector->connect(superPixels);
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

QSharedPointer<TextLine> PixelSet::toTextLine() const {

	BaseLine bl(fitLine(0.0));

	QSharedPointer<TextLine> textLine = QSharedPointer<TextLine>::create();
	textLine->setBaseLine(bl);
	textLine->setPolygon(convexHull());

	return textLine;
}

void PixelSet::draw(QPainter& p) const {

	for (auto px : mSet)
		px->draw(p, 0.3, Pixel::draw_ellipse_stats);

	//polyLine(0.0).draw(p);
	convexHull().draw(p);
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

	p.setPen(ColorManager::colors()[0]);
	for (auto px : mSet->pixels())
		px->draw(p, 0.3, Pixel::draw_ellipse_only);

	p.setPen(ColorManager::darkGray(.4));
	for (auto e : edges())
		e->draw(p);

}

void PixelGraph::connect(const PixelConnector& connector) {

	// nothing todo?
	if (isEmpty())
		return;

	assert(mSet);
	const QVector<QSharedPointer<Pixel> >& pixels = mSet->pixels();

	mEdges = connector.connect(pixels);

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

			double minCC = Algorithms::min(cce);
			double minCW = Algorithms::min(cwe);

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

// PixelConnector --------------------------------------------------------------------
PixelConnector::PixelConnector() {
}

// DelauneyPixelConnector --------------------------------------------------------------------
DelauneyPixelConnector::DelauneyPixelConnector() : PixelConnector() {
}

QVector<QSharedPointer<PixelEdge>> DelauneyPixelConnector::connect(const QVector<QSharedPointer<Pixel> >& pixels) const {
	
	Timer dt;
	// Create an instance of Subdiv2D
	QVector<Vector2D> pts;
	for (const QSharedPointer<Pixel>& px : pixels) {
		assert(px);
		pts << px->center();
	}
	Rect rect = Rect::fromPoints(pts);
	rect.expand(2.0);	// needed for round from Rect to cvRect
	//qDebug() << "bounding rect found" << dt;

	cv::Subdiv2D subdiv(rect.toCvRect());

	QVector<int> ids;
	for (const QSharedPointer<Pixel>& b : pixels) {
		Vector2D np = b->center();
		ids << subdiv.insert(np.toCvPoint2f());
	}
	//qDebug() << "delauney triangulation (OpenCV)" << dt;

	// that took me long... but this is how we can map the edges to our objects without an (expensive) lookup
	QVector<QSharedPointer<PixelEdge> > edges;
	for (int idx = 0; idx < (pixels.size()-8)*3; idx++) {

		int orgVertex = ids.indexOf(subdiv.edgeOrg((idx << 2)));
		int dstVertex = ids.indexOf(subdiv.edgeDst((idx << 2)));

		// there are a few edges that lead to nowhere
		if (orgVertex == -1 || dstVertex == -1) {
			continue;
		}

		assert(orgVertex >= 0 && orgVertex < pixels.size());
		assert(dstVertex >= 0 && dstVertex < pixels.size());

		QSharedPointer<PixelEdge> pe(new PixelEdge(pixels[orgVertex], pixels[dstVertex]));
		edges << pe;
	}
	//qDebug() << "converted to edges" << dt;

	return edges;

}

// RegionPixelConnector --------------------------------------------------------------------
RegionPixelConnector::RegionPixelConnector(double multiplier) {
	mMultiplier = multiplier;
}

/// <summary>
/// Fully connected graph.
/// Super pixels are connected with all other super pixels within a region 
/// of radius lineSpacing() * radiusFactor or radius.
/// </summary>
/// <returns>Connecting edges.</returns>
QVector<QSharedPointer<PixelEdge>> RegionPixelConnector::connect(const QVector<QSharedPointer<Pixel> >& pixels) const {
	
	Timer dt;
	QVector<QSharedPointer<PixelEdge> > edges;

	for (const QSharedPointer<Pixel>& px : pixels) {

		if (!px->stats() && mRadius == 0.0) {
			qWarning() << "pixel stats are NULL where they should not be: " << px->id();
			continue;
		}

		double cR = (mRadius != 0.0) ? mRadius : px->stats()->lineSpacing() * mMultiplier;
		const Vector2D& pxc = px->center();

		for (const QSharedPointer<Pixel>& npx : pixels) {

			if (npx->id() == px->id())
				continue;

			if (pxc.isNeighbor(npx->center(), cR))
				edges << QSharedPointer<PixelEdge>::create(px, npx);
		}
	}
	qDebug() << edges.size() << "edges connected in " << dt;

	return edges;

}

void RegionPixelConnector::setRadius(double radius) {
	mRadius = radius;
}

void RegionPixelConnector::setLineSpacingMultiplier(double multiplier) {
	mMultiplier = multiplier;
}

// TabStopPixelConnector --------------------------------------------------------------------
TabStopPixelConnector::TabStopPixelConnector() : PixelConnector() {
}

QVector<QSharedPointer<PixelEdge>> TabStopPixelConnector::connect(const QVector<QSharedPointer<Pixel> >& pixels) const {
	
	QVector<QSharedPointer<PixelEdge> > edges;

	for (const QSharedPointer<Pixel>& px : pixels) {

		if (!px->stats()) {
			qWarning() << "pixel stats are NULL where they should not be: " << px->id();
			continue;
		}

		double cR = px->stats()->lineSpacing() * mMultiplier;
		double tOr = px->stats()->orientation() - px->tabStop().orientation();
		const Vector2D& pxc = px->center();

		// tab's orientation vector
		Vector2D orVec = px->stats()->orVec();
		orVec.rotate(px->tabStop().orientation());

		QList<double> dists;
		QVector<QSharedPointer<PixelEdge> > cEdges;

		for (const QSharedPointer<Pixel>& npx : pixels) {

			if (npx->id() == px->id())
				continue;

			// directely reject
			if (!pxc.isNeighbor(npx->center(), cR * 3))
				continue;

			double cOr = npx->stats()->orientation() - npx->tabStop().orientation();

			// tabstop pixels must be 'aligned' w.r.t to the line orientation
			Vector2D corVec(1.0, 0.0);
			corVec.rotate(px->stats()->orientation());
			Line line(pxc, npx->center());
			bool isN = line.weightedLength(corVec) < cR;

			// are the tab-stop orientations the same?? and are both pixels within the the currently defined radius?
			if (isN && Algorithms::angleDist(tOr, cOr) < .1) {	// do we have the same orientation?

				QSharedPointer<PixelEdge> edge = QSharedPointer<PixelEdge>::create(px, npx);

				// normalize distance with tab orientation
				double ea = orVec * edge->edge().vector();
				dists << ea;
				cEdges << edge;
			}

		}

		if (cEdges.size() > 2) {
			// only take the closest 10%
			double q1 = Algorithms::statMoment(dists, 0.5);

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

void TabStopPixelConnector::setLineSpacingMultiplier(double multiplier) {

	mMultiplier = multiplier;
}
// DBScanPixelConnector --------------------------------------------------------------------
DBScanPixelConnector::DBScanPixelConnector() {
}

QVector<QSharedPointer<PixelEdge>> DBScanPixelConnector::connect(const QVector<QSharedPointer<Pixel>>& pixels) const {
	
	DBScanPixel dbs(pixels);
	dbs.compute();

	return dbs.edges();
}

// DBScanPixel --------------------------------------------------------------------
DBScanPixel::DBScanPixel(const QVector<QSharedPointer<Pixel>>& pixels) {
	mPixels = pixels;
}

void DBScanPixel::compute() {

	mLineSpacing = mPixels.lineSpacing();

	// cache
	mDists = calcDists(mPixels);
	mLabels = cv::Mat(1, mPixels.size(), CV_32S, cv::Scalar(not_visited));
	mLabelPtr = mLabels.ptr<unsigned int>();

	for (int pIdx = 0; pIdx < mPixels.size(); pIdx++) {
		
		if (mLabelPtr[pIdx] != not_visited)
			continue;

		mLabelPtr[pIdx] = visited;
		
		// epsilon depends on line spacing
		double cEps = mLineSpacing*mEpsMultiplier;// mPixels[pIdx]->stats() ? mPixels[pIdx]->stats()->lineSpacing()*mEpsMultiplier : 15.0*mEpsMultiplier;
		QVector<int> neighbors = regionQuery(pIdx, cEps);
		
		if (neighbors.size() >= mMinPts) {
			expandCluster(pIdx, mCLabel, neighbors, mEpsMultiplier, mMinPts);
			mCLabel++;	// start a new cluster
		}
		else
			mLabelPtr[pIdx] = noise;
	}
}

void DBScanPixel::setEpsilonMultiplier(double eps) {
	mEpsMultiplier = eps;
}

void DBScanPixel::setDistanceFunction(const PixelDistance::PixelDistanceFunction & distFnc) {
	mDistFnc = distFnc;
}

QVector<PixelSet> DBScanPixel::sets() const {

	QVector<PixelSet> sets(mCLabel-cluster0);

	if (sets.empty())
		return QVector<PixelSet>();

	for (int cIdx = 0; cIdx < mLabels.cols; cIdx++) {

		int setIdx = mLabelPtr[cIdx] - cluster0;

		if (setIdx >= 0) {
			assert(setIdx < sets.size());
			sets[setIdx].add(mPixels[cIdx]);
		}
	}

	return sets;
}

QVector<QSharedPointer<PixelEdge> > DBScanPixel::edges() const {

	QVector<PixelSet> ps = sets();
	QVector<QSharedPointer<PixelEdge> > edges;

	for (const PixelSet& cs : ps) {
		edges << PixelSet::connect(cs.pixels());
	}

	return edges;
}

void DBScanPixel::expandCluster(int pixelIndex, unsigned int clusterIndex, const QVector<int>& neighbors, double eps, int minPts) const {

	assert(pixelIndex >= 0 && pixelIndex < mLabels.cols);
	mLabelPtr[pixelIndex] = clusterIndex;

	for (int nIdx : neighbors) {

		assert(nIdx >= 0 && nIdx < mLabels.cols);

		if (mLabelPtr[nIdx] == not_visited) {
			mLabelPtr[nIdx] = visited;

			double cEps = mLineSpacing*mEpsMultiplier;
			QVector<int> nPts = regionQuery(nIdx, cEps);
			if (nPts.size() >= minPts) {
				expandCluster(nIdx, clusterIndex, nPts, eps, minPts);
			}
		}
		if (mLabelPtr[nIdx] == visited)
			mLabelPtr[nIdx] = clusterIndex;
	}

}

QVector<int> DBScanPixel::regionQuery(int pixelIdx, double eps) const {

	assert(pixelIdx >= 0 && pixelIdx < mDists.rows);

	QVector<int> neighbors;
	const float* dPtr = mDists.ptr<float>(pixelIdx);

	for (int cIdx = 0; cIdx < mDists.cols; cIdx++) {

		if (dPtr[cIdx] < eps)
			neighbors << cIdx;

	}

	return neighbors;
}

cv::Mat DBScanPixel::calcDists(const PixelSet& pixels) const {
	
	cv::Mat dists(mPixels.size(), mPixels.size(), CV_32FC1, cv::Scalar(0));

	for (int rIdx = 0; rIdx < dists.rows; rIdx++) {

		float* dPtr = dists.ptr<float>(rIdx);

		for (int cIdx = rIdx + 1; cIdx < dists.cols; cIdx++) {
			dPtr[cIdx] = (float)mDistFnc(pixels[rIdx], pixels[cIdx]);
			dists.ptr<float>(cIdx)[rIdx] = dPtr[cIdx];	// reflect
		}
	}
	
	return dists;
}

}