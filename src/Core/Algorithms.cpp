/*******************************************************************************************************
 ReadFramework is the basis for modules developed at CVL/TU Wien for the EU project READ. 
  
 Copyright (C) 2016 Markus Diem <diem@cvl.tuwien.ac.at>
 Copyright (C) 2016 Stefan Fiel <fiel@cvl.tuwien.ac.at>
 Copyright (C) 2016 Florian Kleber <kleber@cvl.tuwien.ac.at>

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
 [1] https://cvl.tuwien.ac.at/
 [2] https://transkribus.eu/Transkribus/
 [3] https://github.com/TUWien/
 [4] https://nomacs.org
 *******************************************************************************************************/

#include "Algorithms.h"
#include "Utils.h"
#include "Pixel.h"

#pragma warning(push, 0)	// no warnings from includes
#include <QDebug>
#include <QVector2D>
#include <QMatrix4x4>

#include <opencv2/imgproc.hpp>
#include <opencv2/imgproc/imgproc_c.h>
#pragma warning(pop)

namespace rdf {

// Algorithms --------------------------------------------------------------------

/// <summary>
///     The resulting rounding error after floating point computations depend on the specific operations done.The same number computed by
///		different algorithms could present different rounding errors.For a useful comparison, an estimation of the relative rounding error
///		should be considered and compared to a factor times EPS.The factor should be related to the cumulated rounding error in the chain of
///		computation.Here, as a simplification, a fixed factor is used. 
/// </summary>
/// <param name="a">Input a</param>
/// <param name="b">Input b</param>
/// <returns>equal if relative error <= factor x eps</returns>
int Algorithms::doubleEqual(double a, double b) {
	double abs_diff, aa, bb, abs_max;

	/* trivial case */
	if (a == b) return 1;

	abs_diff = fabs(a - b);
	aa = fabs(a);
	bb = fabs(b);
	abs_max = aa > bb ? aa : bb;

	/* DBL_MIN is the smallest normalized number, thus, the smallest
	number whose relative error is bounded by DBL_EPSILON. For
	smaller numbers, the same quantization steps as for DBL_MIN
	are used. Then, for smaller numbers, a meaningful "relative"
	error should be computed by dividing the difference by DBL_MIN. */
	if (abs_max < DBL_MIN) abs_max = DBL_MIN;

	/* equal if relative error <= factor x eps */
	return (abs_diff / abs_max) <= (100.0 * DBL_EPSILON);
}

double Algorithms::absAngleDiff(double a, double b) {

	return std::abs(signedAngleDiff(a, b));
}

double Algorithms::signedAngleDiff(double a, double b) {

	a -= b;
	while (a <= -CV_PI) a += 2*CV_PI;
	while (a >   CV_PI) a -= 2*CV_PI;
	
	return a;
}

/// <summary>
/// Computes the normalized angle within startIvl and endIvl.
/// </summary>
/// <param name="angle">The angle in rad.</param>
/// <param name="startIvl">The intervals lower bound.</param>
/// <param name="endIvl">The intervals upper bound.</param>
/// <returns>The angle within [startIvl endIvl].</returns>
double Algorithms::normAngleRad(double angle, double startIvl, double endIvl) {
	// this could be a bottleneck
	if (abs(angle) > 1000)
		return angle;

	while (angle < startIvl)
		angle += endIvl - startIvl;
	while (angle >= endIvl)
		angle -= endIvl - startIvl;

	return angle;
}

/// <summary>
/// Computes the distance between two angles.
/// Hence, min(angleDiff, CV_PI*2-(angleDiff))
/// </summary>
/// <param name="angle1">The angle1.</param>
/// <param name="angle2">The angle2.</param>
/// <returns>The angular distance.</returns>
double Algorithms::angleDist(double angle1, double angle2, double maxAngle) {

	angle1 = normAngleRad(angle1);
	angle2 = normAngleRad(angle2);
	double dist = normAngleRad(angle1 - angle2, 0, maxAngle);

	return std::min(dist, maxAngle - dist);
}

/// <summary>
/// Returns the minimum of the vector or DBL_MAX if vec is empty.
/// </summary>
/// <param name="vec">A vector with double values.</param>
/// <returns>the minimum</returns>
double Algorithms::min(const QVector<double>& vec) {

	double mn = DBL_MAX;

	for (double v : vec)
		if (mn > v)
			mn = v;

	return mn;
}

/// <summary>
/// Returns the maximum value of vec or -DBL_MAX if vec is empty.
/// </summary>
/// <param name="vec">A vector with double values.</param>
/// <returns>the maximum</returns>
double Algorithms::max(const QVector<double>& vec) {

	double mx = -DBL_MAX;

	for (double v : vec)
		if (mx < v)
			mx = v;

	return mx;
}

// LineFitting --------------------------------------------------------------------
LineFitting::LineFitting(const QVector<Vector2D>& pts) {
	mPts = pts;
}

/// <summary>
/// Fits a line to the given set using the Least Median Squares method.
/// In contrast to the official LMS, we extensively sample lines if 
/// the combinatorial effort is less than num sets.
/// </summary>
/// <returns>The best fitting line.</returns>
Line LineFitting::fitLineLMS() const {

	if (mPts.size() <= mSetSize) {
		qInfo() << "cannot fit line - the set is too small";
		return Line();
	}

	int numFullSampling = qRound(mPts.size() * std::log(mPts.size()));	// is full sampling too expensive?

	Line bestLine;
	double minLMS = DBL_MAX;

	// random sampling
	if (mSetSize != 2 || mNumSets < numFullSampling) {

		// generate random sets
		for (int lIdx = 0; lIdx < mNumSets; lIdx++) {
			
			QVector<Vector2D> set;
			sample(mPts, set, mSetSize);
			Line line;

			if (set.size() == 2) {
				line = Line(set[0], set[1]);
			}
			else {
				qWarning() << "least squares not implemented yet - mSetSize must be 2";
				// TODO: cv least squares here
			}

			// compute distance of current set
			double mr = medianResiduals(mPts, line);

			if (mr < minLMS) {
				minLMS = mr;
				bestLine = line;
			}

			if (minLMS < mEps)
				break;
		}

	}
	// try all permutations
	else {

		// compute all possible lines of the current set
		for (int lIdx = 0; lIdx < mPts.size(); lIdx++) {
			const Vector2D& vec = mPts[lIdx];

			for (int rIdx = lIdx + 1; rIdx < mPts.size(); rIdx++) {
				Line line(vec, mPts[rIdx]);

				if (line.length() < mMinLength)
					continue;

				double mr = medianResiduals(mPts, line);

				if (mr < minLMS) {
					minLMS = mr;
					bestLine = line;
				}

				if (minLMS < mEps)
					break;
			}
		}
	}
	
	return bestLine;
}

Line LineFitting::fitLine() const {

	std::vector<cv::Point> pts;

	for (const Vector2D& pt : mPts)
		pts.push_back(pt.toCvPoint());

	cv::Vec4f lineVec;
	cv::fitLine(pts, lineVec, CV_DIST_L2, 0, 10, 0.01);
	
	// convert line vec to a line:
	// (vx, vy, x0, y0)
	Vector2D g(lineVec[0], lineVec[1]);		
	Vector2D x0(lineVec[2], lineVec[3]);
	Vector2D x1 = x0 + g;

	return Line(x0, x1);
}

/// <summary>
/// Returns randomly sampled pts.
/// </summary>
/// <param name="pts">Input points.</param>
/// <param name="set">A randomly sampled set of size setSize.</param>
/// <param name="setSize">The set size returned.</param>
void LineFitting::sample(const QVector<Vector2D>& pts, QVector<Vector2D>& set, int setSize) const {

	if (setSize > pts.size())
		qWarning() << "[LineFitting] the number of points [" << pts.size() << "] is smaller than the set size: " << setSize;

	for (int idx = 0; idx < setSize; idx++) {
		double r = Utils::rand();
		int rIdx = qRound(r*(pts.size()-1));
		set << pts[rIdx];
	}

}

/// <summary>
/// Returns the median of the squared distances between pts and the line.
/// </summary>
/// <param name="pts">The point set.</param>
/// <param name="line">The current fitting line.</param>
/// <returns>The median squared distance.</returns>
double LineFitting::medianResiduals(const QVector<Vector2D>& pts, const Line & line) const {

	QList<double> squaredDists;

	for (const Vector2D& pt : pts) {

		double d = line.distance(pt);
		squaredDists << d*d;
	}

	return Algorithms::statMoment(squaredDists, 0.5);
}

// Pixel Distances --------------------------------------------------------------------
/// <summary>
/// Euclidean distance between the pixel's centers.
/// </summary>
/// <param name="px1">The first SuperPixel.</param>
/// <param name="px2">The second SuperPixel.</param>
/// <returns></returns>
double PixelDistance::euclidean(const Pixel* px1, const Pixel* px2) {

	assert(px1 && px2);
	return Vector2D(px2->center() - px1->center()).length();
}

double PixelDistance::mahalanobis(const Pixel * px1, const Pixel * px2) {
	
	// untested
	assert(px1 && px2);
	cv::Mat icov = px1->ellipse().toCov();
	cv::invert(icov, icov, cv::DECOMP_SVD);

	Vector2D xm(px2->center() - px1->center());
	cv::Mat xmm = xm.toMatRow();

	cv::Mat m = (xmm.t()*icov*xmm);
	double s = std::sqrt(m.at<double>(0,0));

	return s;
}

double PixelDistance::bhattacharyya(const Pixel * px1, const Pixel * px2) {
	
	assert(px1 && px2);
	cv::Mat cov1 = px1->ellipse().toCov();
	cv::Mat cov2 = px2->ellipse().toCov();
	cv::Mat cov = (cov1 + cov2) / 2.0;
	cv::Mat icov;
	cv::invert(cov, icov, cv::DECOMP_SVD);

	// normalization
	double d1 = cv::determinant(cov1);
	double d2 = cv::determinant(cov2);
	double d = cv::determinant(cov);
	double n = std::log(d / std::sqrt(d1*d2))/2.0;

	// (mu2 - mu1)
	Vector2D xm(px2->center() - px1->center());
	cv::Mat xmm = xm.toMatRow();

	// (mu2 - mu1)T S-1 (mu2 - mu1) <- yes it's the generalization of the mahalnobis distance
	double m = *cv::Mat(xmm.t()*icov*xmm).ptr<double>();
	m /= 8.0;

	return m + n;
}

/// <summary>
/// Angle weighted pixel distance.
/// If the pixel's local orientations are not
/// computed, this method defaults to the euclidean distance.
/// </summary>
/// <param name="px1">The first SuperPixel.</param>
/// <param name="px2">The second SuperPixel.</param>
/// <returns></returns>
double PixelDistance::angleWeighted(const Pixel* px1, const Pixel* px2) {

	assert(px1 && px2);

	if (!px1->stats() || !px2->stats()) {
		qWarning() << "cannot compute angle weighted distance if stats are NULL";
		return euclidean(px1, px2);
	}

	Vector2D edge = px2->center() - px1->center();
	double dt1 = std::abs(edge.theta(px1->stats()->orVec()));
	double dt2 = std::abs(edge.theta(px2->stats()->orVec()));

	double a = qMin(dt1, dt2);

	return edge.length() * (a + 0.01);	// + 0.01 -> we don't want to map all 'aligned' pixels to 0
}

/// <summary>
/// Returns the edge weight normalized by the line spacing.
/// This function returns an edge weight that is similar
/// to the one proposed by Il Koo. Hence, it can be used
/// for graphcuts.
/// </summary>
/// <param name="edge">The pixel edge.</param>
/// <returns></returns>
double PixelDistance::spacingWeighted(const PixelEdge * edge) {

	if (!edge || edge->isNull())
		return 0.0;


	double beta = 1.0;

	auto px1 = edge->first();
	auto px2 = edge->second();

	// this edge weight is needed for the GraphCut
	if (px1->stats() && px2->stats()) {

		double sp = px1->stats()->lineSpacing();
		double sq = px2->stats()->lineSpacing();
		double nl = (beta * edge->edge().squaredLength()) / (sp * sp + sq * sq);
		double ew = exp(-nl);

		if (ew < 0.0 || ew > 1.0) {
			qDebug() << "illegal edge weight: " << ew;
		}

		// TODO: add mu(fp,fq) according to koo's indices
		return ew;
	}

	qDebug() << "no stats when computing the scaled edges...";
	return euclidean(edge);
}

double PixelDistance::orientationWeighted(const PixelEdge * edge) {
	
	if (!edge || edge->isNull())
		return 0.0;


	double beta = 1.0;
	
	auto px1 = edge->first();
	auto px2 = edge->second();

	// this edge weight is needed for the GraphCut
	if (px1->stats() && px2->stats()) {

		double sp = px1->stats()->lineSpacing();
		double sq = px2->stats()->lineSpacing();
		double nl = (beta * PixelDistance::angleWeighted(px1.data(), px2.data())) / (sp * sp + sq * sq);
		
		double ew = exp(-nl);

		if (ew < 0.0 || ew > 1.0) {
			qDebug() << "illegal edge weight: " << ew;
		}

		// TODO: add mu(fp,fq) according to koo's indices
		return ew;
	}

	qDebug() << "no stats when computing the oriented edges...";
	return euclidean(edge);
}

double PixelDistance::euclidean(const PixelEdge * edge) {
	
	assert(edge);
	return exp(-edge->edge().length());
}


}