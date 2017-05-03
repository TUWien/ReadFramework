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

#include "GraphCut.h"

// read includes
#include "Image.h"
#include "ImageProcessor.h"

#pragma warning(push, 0)	// no warnings from includes

#include <QPixmap>
#include <QPainter>

//#pragma warning(disable: 4706)
#include "GCGraph.hpp"
#include "graphcut/GCoptimization.h"

#pragma warning(pop)

namespace rdf {

// GraphCutConfig --------------------------------------------------------------------
GraphCutConfig::GraphCutConfig(const QString& name) : ModuleConfig(name) {
}

double GraphCutConfig::scaleFactor() const {
	return mScaleFactor;
}

int GraphCutConfig::numIter() const {
	return mGcIter;
}

void GraphCutConfig::load(const QSettings & settings) {

	mGcIter = settings.value("numIter", mGcIter).toInt();
	mScaleFactor = settings.value("scaleFactor", mScaleFactor).toDouble();
}

void GraphCutConfig::save(QSettings & settings) const {
	settings.setValue("numIter", mGcIter);
	settings.setValue("scaleFactor", mScaleFactor);
}

// GraphCutPixel --------------------------------------------------------------------
GraphCutPixel::GraphCutPixel(const PixelSet & set) : mSet(set) {
	mWeightFnc = PixelDistance::spacingWeighted;
	mConnector = QSharedPointer<DelauneyPixelConnector>::create();

	mConfig = QSharedPointer<GraphCutConfig>::create();
}

bool GraphCutPixel::isEmpty() const {
	return mSet.isEmpty();
}

QSharedPointer<GraphCutConfig> GraphCutPixel::config() const {
	return qSharedPointerCast<GraphCutConfig>(mConfig);
}

PixelSet GraphCutPixel::set() const {
	return mSet;
}

QSharedPointer<GCoptimizationGeneralGraph> GraphCutPixel::graphCut(const PixelGraph & graph) const {

	if (graph.isEmpty()) {
		return QSharedPointer<GCoptimizationGeneralGraph>();
	}

	int nLabels = numLabels();

	QVector<QSharedPointer<Pixel> > pixel = graph.set().pixels();

	// get costs and smoothness term
	cv::Mat c = costs(nLabels);				// Set size x #labels
	cv::Mat sm = labelDistMatrix(nLabels);	// #labels x #labels

	// init the graph
	QSharedPointer<GCoptimizationGeneralGraph> gc(new GCoptimizationGeneralGraph(pixel.size(), nLabels));
	gc->setDataCost(c.ptr<int>());
	gc->setSmoothCost(sm.ptr<int>());
	
	// create neighbors
	const QVector<QSharedPointer<PixelEdge> >& edges = graph.edges();
	for (int idx = 0; idx < pixel.size(); idx++) {

		for (int edgeIdx : graph.edgeIndexes(pixel.at(idx)->id())) {

			assert(edgeIdx != -1);

			// get vertex ID
			const QSharedPointer<PixelEdge>& pe = edges[edgeIdx];
			int sVtxIdx = graph.pixelIndex(pe->second()->id());

			// compute weight
			double rawWeight = mWeightFnc(pe.data());
			int w = qRound(rawWeight * config()->scaleFactor());

			gc->setNeighbors(idx, sVtxIdx, w);
		}
	}

	//Image::imageInfo(c, "cost");
	//Image::imageInfo(sm, "labelDist");

	// run the expansion-move
	try {
		qDebug() << "energy before:" << gc->compute_energy();
		//gc->expansion(config()->numIter());
		gc->swap(config()->numIter());
		qDebug() << "energy after:" << gc->compute_energy();
	}
	catch (GCException gce) {

		mWarning << "exception while performing graph-cut";
		mWarning << QString::fromUtf8(gce.message);
		return QSharedPointer<GCoptimizationGeneralGraph>();
	}
	
	return gc;
}

int GraphCutPixel::numLabels() const {
	return 0;
}


// GraphCutOrientation --------------------------------------------------------------------
GraphCutOrientation::GraphCutOrientation(const PixelSet& set) : GraphCutPixel(set) {
}

bool GraphCutOrientation::checkInput() const {
	return !isEmpty();
}

bool GraphCutOrientation::compute() {

	if (!checkInput())
		return false;

	Timer dt;

	// create graph
	PixelGraph graph(mSet);
	graph.connect(*mConnector);

	// perform graphcut
	auto gc = graphCut(graph);

	if (gc) {
		QVector<QSharedPointer<Pixel> > pixel = graph.set().pixels();
		for (int idx = 0; idx < pixel.size(); idx++) {

			auto ps = pixel[idx]->stats();
			ps->setOrientationIndex(gc->whatLabel(idx));
		}
	}

	mInfo << "computed in" << dt;

	return true;
}

cv::Mat GraphCutOrientation::costs(int numLabels) const {

	// fill costs
	cv::Mat data(mSet.size(), numLabels, CV_32SC1);

	for (int idx = 0; idx < mSet.size(); idx++) {

		auto ps = mSet[idx]->stats();
		assert(ps);

		cv::Mat cData = ps->data(PixelStats::combined_idx);
		cData.convertTo(data.row(idx), CV_32SC1, config()->scaleFactor()*100);	// TODO: check scaling
	}

	return data;
}

cv::Mat GraphCutOrientation::labelDistMatrix(int numLabels) const {

	cv::Mat orDist(numLabels, numLabels, CV_32SC1);

	for (int rIdx = 0; rIdx < orDist.rows; rIdx++) {

		unsigned int* sPtr = orDist.ptr<unsigned int>(rIdx);

		for (int cIdx = 0; cIdx < orDist.cols; cIdx++) {
			
			// set smoothness cost for orientations
			int diff = abs(rIdx - cIdx);
			sPtr[cIdx] = qMin(diff, numLabels - diff);

			// Il Koo proposed this:
			//// set smoothness cost for orientations
			//if (rIdx == cIdx)
			//	sPtr[cIdx] = 0;

			//int diff = abs(rIdx - cIdx);
			//sPtr[cIdx] = diff <= 3 ? 1 : 12; // Il Koo: 0.4 : 5
		}
	}

	return orDist;
}

/// <summary>
/// Returns the numbers of labels (states).
/// The statistics' columns == the number of possible labels
/// </summary>
/// <returns></returns>
int GraphCutOrientation::numLabels() const {
	return mSet.isEmpty() ? 0 : mSet.pixels()[0]->stats()->data().cols;
}

cv::Mat GraphCutOrientation::draw(const cv::Mat & img, const QColor & col) const {

	// debug - remove
	QPixmap pm = Image::mat2QPixmap(img);
	QPainter p(&pm);

	// show the graph
	PixelGraph graph(mSet);
	graph.connect(*mConnector);

	p.setPen(ColorManager::darkGray(0.3));
	graph.draw(p);

	p.setPen(col);

	for (auto px : mSet.pixels()) {

		if (!col.isValid())
			p.setPen(ColorManager::randColor());
		px->draw(p, 0.3, (Pixel::DrawFlags)(Pixel::draw_stats));
	}

	return Image::qPixmap2Mat(pm);
}

// GraphCutTextLine --------------------------------------------------------------------
GraphCutTextLine::GraphCutTextLine(const QVector<PixelSet>& sets) : GraphCutPixel(PixelSet::merge(sets)) {
	mWeightFnc = PixelDistance::orientationWeighted;
	
	mTextLines = sets;
}

bool GraphCutTextLine::compute() {
	
	if (!checkInput())
		return false;

	Timer dt;

	// remove small text lines
	QVector<PixelSet> tlf;
	for (auto tl : mTextLines)
		if (tl.size() > 5)
			tlf << tl;
	mTextLines = tlf;

	// DEBUG only!
	// sort w.r.t y
	std::sort(mTextLines.begin(), mTextLines.end(),
		[](const PixelSet& ps1, const PixelSet& ps2) {
		return ps1.center().y() < ps2.center().y();
	}
	);

	// create graph & perform energy minimization
	PixelGraph graph(mSet);
	graph.connect(*mConnector);
	auto gc = graphCut(graph);

	int ntl = mTextLines.size();

	if (gc) {

		// convert gc labels to text lines
		QMap<int, PixelSet> tlMap;
		auto pixels = mSet.pixels();

		for (int idx = 0; idx < pixels.size(); idx++) {

			int cl = gc->whatLabel(idx);

			// append pixel to text line
			if (tlMap.contains(cl))
				tlMap[cl].add(pixels[idx]);
			else {
				PixelSet ps;
				ps.add(pixels[idx]);
				tlMap.insert(cl, ps);
			}
		}

		mTextLines = tlMap.values().toVector();
	}

	mInfo << "computed in" << dt;
	qDebug() << mTextLines.size() << "/" << ntl << "textlines after graph-cut";

	return true;
}

cv::Mat GraphCutTextLine::draw(const cv::Mat & img, const QColor& col) const {
	
	// debug - remove
	QPixmap pm = Image::mat2QPixmap(img);
	QPainter p(&pm);

	//  -------------------------------------------------------------------- show the graph
	p.setOpacity(0.3);
	PixelGraph graph(mSet);
	graph.connect(*mConnector);

	//p.setPen(ColorManager::darkGray(0.3));
	p.setPen(ColorManager::pink());
	auto wf = PixelDistance::orientationWeighted;
	graph.draw(p, &wf, 0.1);

	//  -------------------------------------------------------------------- draw pixels
	p.setPen(ColorManager::blue());
	for (auto px : mSet.pixels())
		px->draw(p, 0.3, Pixel::draw_stats);

	//  -------------------------------------------------------------------- draw text lines
	p.setOpacity(0.7);
	p.setPen(col);

	for (auto tl : mTextLines) {

		if (!col.isValid())
			p.setPen(ColorManager::randColor());
		tl.draw(p, PixelSet::draw_poly);
	}

	//  -------------------------------------------------------------------- draw baselines
	p.setOpacity(0.3);
	p.setPen(ColorManager::pink().darker());

	for (auto tl : mTextLines) {
		tl.fitLine().draw(p);
	}

	//saveDistsDebug("C:/temp/lines/line.jpg", img);

	return Image::qPixmap2Mat(pm);
}

QVector<PixelSet> GraphCutTextLine::textLines() {
	return mTextLines;
}

bool GraphCutTextLine::checkInput() const {
	return !isEmpty();
}

cv::Mat GraphCutTextLine::costs(int numLabels) const {

	// fill costs
	cv::Mat data(mSet.size(), numLabels, CV_64FC1);

	//  -------------------------------------------------------------------- compute mahalanobis dists
	// convert centers
	cv::Mat centers = pixelSetCentersToMat(mSet);

	for (int idx = 0; idx < mTextLines.size(); idx++) {
		cv::Mat md = mahalanobisDists(mTextLines[idx], centers);
		//md *= 1.0/mTextLines[idx].area();
		md.copyTo(data.col(idx));
	}

	data *= 5000;
	data.convertTo(data, CV_32SC1);

	Image::imageInfo(data, "costs");

	return data;
}

cv::Mat GraphCutTextLine::labelDistMatrix(int numLabels) const {

	cv::Mat labelDist(numLabels, numLabels, CV_32FC1);

	for (int rIdx = 0; rIdx < numLabels; rIdx++) {

		float* lp = labelDist.ptr<float>(rIdx);
		cv::Mat icov = mTextLines[rIdx].fitEllipse().toCov();
		icov = icov.mul(icov);	// square the covariance - to prefer 'horizontally' aligned text lines
		cv::invert(icov, icov, cv::DECOMP_SVD);

		for (int cIdx = 0; cIdx < numLabels; cIdx++) {

			// compute the mahalnobis distance
			Vector2D vec(mTextLines[rIdx].center() - mTextLines[cIdx].center());
			cv::Mat nm = vec.toMatCol();
			nm = nm*icov*nm.t();

			double d = std::sqrt(nm.at<double>(0, 0));

			lp[cIdx] = (float)d;
		}
	}

	cv::Mat labelIdx;
	cv::sortIdx(labelDist, labelIdx, CV_SORT_EVERY_ROW + CV_SORT_ASCENDING);

	int numNeighbors = 3;
	//double maxDist = 0.1;	// turned off 2

	cv::Mat labelCosts(labelDist.size(), CV_32SC1, cv::Scalar(numNeighbors));
	for (int rIdx = 0; rIdx < labelCosts.rows; rIdx++) {

		const int* lIdxPtr = labelIdx.ptr<int>(rIdx);
		//const float* dPtr = labelDist.ptr<float>(rIdx);

		int* lcPtr = labelCosts.ptr<int>(rIdx);

		for (int cIdx = 0; cIdx < labelCosts.cols && cIdx < numNeighbors; cIdx++) {

			// look-up the nth nearest neighbor
			int lIdx = lIdxPtr[cIdx];

			//if (dPtr[lIdx] < maxDist)
				lcPtr[lIdx] = cIdx;
				//lcPtr[lIdx] = dPtr[lIdx];		// we could use the distance here too
		}
	}

	labelCosts = makeSymmetric<int>(labelCosts);

	Image::imageInfo(labelDist, "label dists");
	Image::imageInfo(labelCosts, "label costs");

	return labelCosts;
}

int GraphCutTextLine::numLabels() const {

	return mTextLines.size();
}

cv::Mat GraphCutTextLine::mahalanobisDists(const PixelSet & tl, const cv::Mat& centers) const {
	
	cv::Mat dists(centers.rows, 1, CV_64FC1);
	double* dp = dists.ptr<double>();

	// get the textlines mean & cov
	cv::Mat c = tl.center().toMatCol();
	cv::Mat icov = tl.fitEllipse().toCov();
	//icov = icov.mul(icov);
	cv::invert(icov, icov, cv::DECOMP_SVD);

	//double mthr = 7.0;

	for (int idx = 0; idx < centers.rows; idx++) {

		// compute the mahalnobis distance
		cv::Mat nm = centers.row(idx) - c;
		nm = nm*icov*nm.t();

		double d = std::sqrt(nm.at<double>(0, 0));
		dp[idx] = d;// (d < mthr) ? d : mthr;
	}

	return dists;
}

cv::Mat GraphCutTextLine::euclideanDists(const PixelSet & tl) const {
	
	// not in use
	cv::Mat dists(mSet.size(), 1, CV_64FC1);
	double* dp = dists.ptr<double>();

	Vector2D c = tl.center();

	for (int idx = 0; idx < mSet.size(); idx++) {

		auto px = mSet[idx];
		Vector2D d(c - px->center());

		// local orientation weighted
		double a = 1.0;
		if (px->stats())
			a = std::abs(d.theta(mSet[idx]->stats()->orVec()));

		dp[idx] = qRound(d.length() * (a + 0.01));	// + 0.01 -> we don't want to map all 'aligned' pixels to 0
	}

	return dists;
}

/// <summary>
/// Converts the pixel centers into a mat.
/// </summary>
/// <param name="set">The pixel set.</param>
/// <returns>a N x 2 CV_64FC1 center mat.</returns>
cv::Mat GraphCutTextLine::pixelSetCentersToMat(const PixelSet & set) const {
	
	cv::Mat centers(set.size(), 2, CV_64FC1);
	double* cp = centers.ptr<double>();

	auto pixels = set.pixels();

	for (int idx = 0; idx < pixels.size(); idx++) {
		
		*cp = pixels[idx]->center().x(); cp++;
		*cp = pixels[idx]->center().y(); cp++;
	}

	return centers;
}

void GraphCutTextLine::saveDistsDebug(const QString & filePath, const cv::Mat& img) const {

	// sort w.r.t y
	QVector<PixelSet> tls = mTextLines;
	//std::sort(tls.begin(), tls.end(),
	//	[](const PixelSet& ps1, const PixelSet& ps2) {
	//	return ps1.center().y() < ps2.center().y();
	//}
	//);


	//cv::Mat labelDist(numLabels(), numLabels(), CV_32FC1);
	//cv::Mat dist = labelDistMatrix(numLabels());
	cv::Mat dist = costs(numLabels());
	dist.convertTo(dist, CV_32FC1);


	int idx = 0;
	for (auto tl : tls) {

		QPixmap pm = Image::mat2QPixmap(img);
		QPainter p(&pm);

		// the next lines show the mahalanobis distance
		cv::Mat d = mahalanobisDists(tl, pixelSetCentersToMat(mSet));
		//cv::Mat d = euclideanDists(tl);
		//cv::Mat d = labelDist.row(idx);

		//// show distances of text lines --------------------------------------------------------------------
		//float* dp = d.ptr<float>();

		//for (int i = 0; i < d.cols; i++) {
		//	int v = qMin(qRound(dp[i]*45), 255);

		//	p.setPen(QColor(v, 0, 0));
		//	tls[i].draw(p);
		//}

		// show distances of pixels --------------------------------------------------------------------
		double* dp = d.ptr<double>();

		int i = 0;
		for (auto px : mSet.pixels()) {
			int v = qMin(qRound(dp[i]*20), 255);

			p.setPen(QColor(v, 0, 0));
			px->draw(p);
			i++;
		}

		// draw currently selected text line
		p.setOpacity(1.0);
		p.setPen(ColorManager::blue());
		//tl.fitEllipse().draw(p, 0.1);
		tls[idx].draw(p, PixelSet::draw_poly);
		
		QPen pen(ColorManager::pink());
		pen.setWidth(3);
		p.setPen(pen);
		auto e = tls[idx].fitEllipse();
		e.draw(p);

		QString sp = rdf::Utils::createFilePath(filePath, QString::number(idx));
		cv::Mat rImg = Image::qPixmap2Mat(pm);
		//rImg = rImg.t();
		Image::save(rImg, sp);
		idx++;
		qDebug() << "writing" << sp;
	}

}

// GraphCutLineSpacing --------------------------------------------------------------------
GraphCutLineSpacing::GraphCutLineSpacing(const PixelSet & set) : GraphCutPixel(set) {
	mConfig = QSharedPointer<GraphCutLineSpacingConfig>::create();
}

bool GraphCutLineSpacing::checkInput() const {
	return !isEmpty();
}

bool GraphCutLineSpacing::compute() {

	if (!checkInput())
		return false;

	Timer dt;

	// compute scaling hist
	mSpaceHist = spacingHist();

	// create graph
	PixelGraph graph(mSet);
	graph.connect(*mConnector);

	// perform graphcut
	auto gc = graphCut(graph);

	if (gc) {

		QVector<QSharedPointer<Pixel> > pixel = graph.set().pixels();
		for (int idx = 0; idx < pixel.size(); idx++) {

			auto ps = pixel[idx]->stats();
			double ls = mSpaceHist.value(gc->whatLabel(idx));

			ps->setLineSpacing(qRound(ls));
		}
	}

	mInfo << "computed in" << dt;

	return true;
}

QSharedPointer<GraphCutLineSpacingConfig> GraphCutLineSpacing::config() const {
	return qSharedPointerDynamicCast<GraphCutLineSpacingConfig>(mConfig);
}

cv::Mat GraphCutLineSpacing::costs(int numLabels) const {

	// fill costs
	cv::Mat data(mSet.size(), numLabels, CV_32SC1);

	for (int idx = 0; idx < mSet.size(); idx++) {

		auto ps = mSet[idx]->stats();
		assert(ps);

		// get the current histogram bin index
		int bIdx = mSpaceHist.binIdx(ps->lineSpacing());
		
		cv::Mat cl(1, numLabels, CV_32SC1);
		unsigned int* clp = cl.ptr<unsigned int>();

		for (int lIdx = 0; lIdx < numLabels; lIdx++) {
			clp[lIdx] = abs(bIdx - lIdx);
		}

		cl.convertTo(data.row(idx), CV_32SC1, config()->scaleFactor());	// TODO: check scaling
	}

	return data;
}

cv::Mat GraphCutLineSpacing::labelDistMatrix(int numLabels) const {

	cv::Mat orDist(numLabels, numLabels, CV_32SC1);

	for (int rIdx = 0; rIdx < orDist.rows; rIdx++) {

		unsigned int* sPtr = orDist.ptr<unsigned int>(rIdx);

		for (int cIdx = 0; cIdx < orDist.cols; cIdx++) {

			// set smoothness cost for orientations
			sPtr[cIdx] = abs(rIdx - cIdx);
		}
	}

	return orDist;
}

/// <summary>
/// Returns the numbers of labels (states).
/// The statistics' columns == the number of possible labels
/// </summary>
/// <returns></returns>
int GraphCutLineSpacing::numLabels() const {
	return config()->numLabels();
}

Histogram GraphCutLineSpacing::spacingHist() const {
	
	cv::Mat scales(1, mSet.size(), CV_32FC1);
	float* sp = scales.ptr<float>();
	
	for (int cIdx = 0; cIdx < scales.cols; cIdx++) {
	
		float v = (float)mSet[cIdx]->stats()->lineSpacing();
		sp[cIdx] = v;
	}

	return Histogram::fromData<float>(scales, numLabels());
}

//cv::Mat GraphCutLineSpacing::spacingHist() const {
//	
//	cv::Mat scales(1, mSet.size(), CV_32FC1);
//	float* sp = scales.ptr<float>();
//	float minVal = FLT_MAX;
//	float maxVal = FLT_MIN;
//
//	for (int cIdx = 0; cIdx < scales.cols; cIdx++) {
//
//		float v = (float)mSet[cIdx]->stats()->lineSpacing();
//		sp[cIdx] = v;
//
//		if (v > maxVal)
//			maxVal = v;
//		if (v < minVal)
//			minVal = v;
//	}
//
//	int histSize = numLabels();
//	float range[] = { minVal, maxVal };
//	const float* histRange = { range };
//
//	cv::Mat hist;
//	cv::calcHist(&scales, 1, 0, cv::Mat(), hist, 1, &histSize, &histRange);
//
//	return hist;
//}

cv::Mat GraphCutLineSpacing::draw(const cv::Mat & img, const QColor & col) const {

	// debug - remove
	QPixmap pm = Image::mat2QPixmap(img);
	QPainter p(&pm);

	// show the graph
	PixelGraph graph(mSet);
	graph.connect(*mConnector);

	p.setPen(ColorManager::darkGray(0.3));
	graph.draw(p);

	p.setPen(col);

	for (auto px : mSet.pixels()) {

		if (!col.isValid())
			p.setPen(ColorManager::randColor());
		px->draw(p, 0.3, (Pixel::DrawFlags)(Pixel::draw_stats));
	}

	Rect r(Vector2D(10, 10), Vector2D(100, 100));
	mSpaceHist.draw(p, r);

	return Image::qPixmap2Mat(pm);
}

// GraphCutLineSpacingConfig --------------------------------------------------------------------
GraphCutLineSpacingConfig::GraphCutLineSpacingConfig() : GraphCutConfig("GraphCutLineSpacingConfig") {
}

int GraphCutLineSpacingConfig::numLabels() const {
	return mNumLabels;
}

void GraphCutLineSpacingConfig::load(const QSettings & settings) {
	
	GraphCutConfig::load(settings);
	mNumLabels = settings.value("numLabels", mNumLabels).toInt();
}

void GraphCutLineSpacingConfig::save(QSettings & settings) const {

	GraphCutConfig::save(settings);
	settings.setValue("numLabels", numLabels());
}

}