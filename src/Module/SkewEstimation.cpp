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

#include "SkewEstimation.h"

#include "Algorithms.h"
#include "Image.h"

#pragma warning(push, 0)	// no warnings from includes
// Qt Includes
#include "opencv2/imgproc/imgproc.hpp"
#include <QVector3D>
#include <QtCore/qmath.h>
#pragma warning(pop)

namespace rdf {

	/// <summary>
	/// Initializes a new instance of the <see cref="BaseSkewEstimation"/> class.
	/// </summary>
	/// <param name="img">The source img.</param>
	/// <param name="mask">The optional mask.</param>
	BaseSkewEstimation::BaseSkewEstimation(const cv::Mat& img, const cv::Mat& mask) {
		mSrcImg = img;
		mMask = mask;

		mIntegralImg = cv::Mat();
		mIntegralSqdImg = cv::Mat();
	}

	/// <summary>
	/// Sets the images.
	/// </summary>
	/// <param name="img">The source img.</param>
	/// <param name="mask">The optional mask.</param>
	void BaseSkewEstimation::setImages(const cv::Mat & img, const cv::Mat & mask)
	{
		mSrcImg = img;
		mMask = mask;

		mIntegralImg = cv::Mat();
		mIntegralSqdImg = cv::Mat();
	}

	/// <summary>
	/// Computes the skew angle based on "Skew Estimation of natural images based on a
	/// salient line detector", Hyung Il Koo and Nam Ik Choo.
	/// </summary>
	/// <returns>true if the angle could be computed</returns>
	bool BaseSkewEstimation::compute()
	{
		if (isEmpty()) {
			return false;
		}

		int w, h;
		int delta, epsilon;
		w = mW;
		w = qRound(mSrcImg.cols / 1430.0*49.0); //check  (nomacs plugin version)
		h = mH;
		h = qRound(mSrcImg.rows / 700.0*12.0); //check (nomacs plugin version)
		w = w <= 1 ? 10 : w;
		h = h <= 1 ? 5 : h;
		epsilon = mEpsilon;
		delta = mDelta;
		delta = qRound(mSrcImg.cols / 1430.0*20.0); //check (nomacs plugin version)
		mMinLineLength = qRound(mSrcImg.cols / 1430.0 * 20.0); //check
		

		cv::Mat horSep = separability(mSrcImg, w, h, mMask);
		cv::Mat verSep = separability(mSrcImg, h, w, mMask);
		
		double min, max;
		cv::minMaxLoc(horSep, &min, &max); //* max -> check
		cv::Mat edgeHor = edgeMap(horSep, mThr*max, HORIZONTAL, mMask);
		cv::minMaxLoc(verSep, &min, &max);
		cv::Mat edgeVer = edgeMap(verSep, mThr*max, VERTICAL, mMask);

		mSelectedLines.clear();
		mSelectedLineTypes.clear();

		QVector<QVector3D> weightsHor = computeWeights(edgeHor, delta, epsilon, HORIZONTAL);
		QVector<QVector3D> weightsVer = computeWeights(edgeVer, delta, epsilon, VERTICAL);

		QVector<QVector3D> weightsAll = weightsHor + weightsVer;

		double diagonal = qSqrt(mSrcImg.rows*mSrcImg.rows + mSrcImg.cols*mSrcImg.cols);
		bool ok = true;

		mSkewAngle = skewEst(weightsAll, diagonal, ok);


		return ok;
	}

	/// <summary>
	/// Gets the skew angle.
	/// </summary>
	/// <returns>The skew angle</returns>
	double BaseSkewEstimation::getAngle()
	{
		if (mSrcImg.empty()) {
			return std::numeric_limits<double>::infinity();
		}

		return mSkewAngle;
	}

	/// <summary>
	/// Computes the separability of two neighbouring regions based on mean and variance.
	/// </summary>
	/// <param name="srcImg">The source img.</param>
	/// <param name="w">The width of the region.</param>
	/// <param name="h">The height of the region.</param>
	/// <param name="mask">The optional mask.</param>
	/// <returns>The separability map</returns>
	cv::Mat BaseSkewEstimation::separability(const cv::Mat& srcImg, int w, int h, const cv::Mat& mask) const
	{

		cv::Mat grayImg = srcImg;

		if (mSrcImg.channels() > 1) {
			cv::cvtColor(mSrcImg, grayImg, CV_BGR2GRAY);
		}

		//check if already computed
		if (mIntegralImg.empty() || mIntegralSqdImg.empty()) {
			cv::integral(grayImg, mIntegralImg, mIntegralSqdImg, CV_64F);
		}

		cv::Mat meanImg, stdImg;

		meanImg = Algorithms::instance().convolveIntegralImage(mIntegralImg, w, h, Algorithms::BORDER_FLIP);
		meanImg /= (float)(w*h);
		stdImg = Algorithms::instance().convolveIntegralImage(mIntegralSqdImg, w, w, Algorithms::BORDER_FLIP);
		stdImg /= (float)(w*h);
		stdImg = stdImg - meanImg.mul(meanImg);
		//cv::sqrt(stdImg, stdImg);  //we need sigma^2

		cv::Mat separability = cv::Mat::zeros(meanImg.size(), CV_64FC1);
		int halfKRows = cvCeil(h * 0.5);

		// if the image dimension (rows, cols) <= 2
		if (2*halfKRows + 1 > separability.rows) {
			return separability;
		}

		//compute separability
		for (int row = halfKRows; row < separability.rows - halfKRows; row++) {

			double* ptrSep = separability.ptr<double>(row);
			//upper support
			const float* ptrM1 = meanImg.ptr<float>(row-halfKRows);
			const float* ptrStd1 = stdImg.ptr<float>(row-halfKRows);
			//lower support
			const float* ptrM2 = meanImg.ptr<float>(row + halfKRows);
			const float* ptrStd2 = stdImg.ptr<float>(row + halfKRows);
			//mask
			const unsigned char* ptrMask = 0;
			if (!mask.empty())
				ptrMask = mask.ptr<unsigned char>(row);
			
			//compute separability
			for (int col = 0; col < separability.cols; col++) {
				if (ptrMask && ptrMask[col] > 0) {
					ptrSep[col] = (double)((ptrM1[col] - ptrM2[col])*(ptrM1[col] - ptrM2[col]));
					ptrSep[col] = ptrSep[col] / (double)(ptrStd1[col] + ptrStd2[col]);
				} else {
					ptrSep[col] = (double)((ptrM1[col] - ptrM2[col])*(ptrM1[col] - ptrM2[col]));
					ptrSep[col] = ptrSep[col] / (double)(ptrStd1[col] + ptrStd2[col]);
				}
			}
		}

		return separability;
	}

	/// <summary>
	/// Computes the edge map based on the separability
	/// </summary>
	/// <param name="separability">The separability map.</param>
	/// <param name="thr">A threshold value for edges (=0.1).</param>
	/// <param name="direction">The direction (horizontal or vertical).</param>
	/// <param name="mask">The optional mask.</param>
	/// <returns>The edge map.</returns>
	cv::Mat BaseSkewEstimation::edgeMap(const cv::Mat& separability, double thr, EdgeDirection direction, const cv::Mat& mask) const
	{
		cv::Mat edgeM = cv::Mat::zeros(separability.size(), CV_8UC1);

		for (int row = 0; row < separability.rows; row++) {

			const double* ptrSep = separability.ptr<double>(row);
			unsigned char* ptrEdge = edgeM.ptr<unsigned char>(row);

			const unsigned char* ptrMask = 0;
			if (!mask.empty())
				ptrMask = mask.ptr<unsigned char>(row);

			for (int col = 0; col < separability.cols; col++) {

				if (ptrSep[col] > thr) {
					bool edgeT = true;
					for (int k = -mKMax; k <= mKMax; k++) {
						const double* sepKPtr = 0;
						if (direction == HORIZONTAL) {
							if (row + k >= 0 && row + k < separability.rows) {
								sepKPtr = separability.ptr<double>(row + k);
							}
						}
						else if (direction == VERTICAL) {
							if (col + k >= 0 && col + k < separability.cols) {
								sepKPtr = separability.ptr<double>(row) + (col+k);
							}
						}

						if (sepKPtr && ptrSep[col] <= *sepKPtr) {
							edgeT = false;
						}
					}

					if (ptrMask && ptrMask[col] && edgeT) {
						ptrEdge[col] = 255;
					} else if (edgeT) {
						ptrEdge[col] = 255;
					}
				}
			}
		}

		return edgeM;
	}

	/// <summary>
	/// Computes the PPHT, detects straight lines and refines it
	/// </summary>
	/// <param name="edgeMap">The edge map.</param>
	/// <param name="delta">Delta parameter: max coordinate deviation for refined lines.</param>
	/// <param name="epsilon">Epsilon parameter: max allowed deviation from line coordinates for the weight calculation (=2).</param>
	/// <param name="direction">The direction (horizontal or vertical).</param>
	/// <returns>The line weights</returns>
	QVector<QVector3D> BaseSkewEstimation::computeWeights(cv::Mat edgeMap, int delta, int epsilon, EdgeDirection direction) {
		std::vector<cv::Vec4i> lines;
		QVector4D maxLine = QVector4D();
		minLineProjLength = mMinLineLength / 4;
		//params: rho resolution, theta resolution, threshold, min Line length, max line gap
		if (direction == HORIZONTAL) {
			HoughLinesP(edgeMap, lines, 1, CV_PI / 180, 50, mMinLineLength, 20);
		} else {
			HoughLinesP(edgeMap.t(), lines, 1, CV_PI / 180, 50, mMinLineLength, 20);
		}

		QVector<QVector3D> computedWeights = QVector<QVector3D>();

		for (size_t i = 0; i < lines.size(); i++) {

			cv::Vec4i l = lines[i];
			QVector3D currMax = QVector3D(0.0, 0.0, 0.0);

			//if (direction == HORIZONTAL) {

				int K = 0;

				if (l[2] < l[0]) l = cv::Vec4i(l[2], l[3], l[0], l[1]);

				int x1 = l[0];
				int x2 = l[2];

				while (x2 > edgeMap.cols) x2--;
				while (x1 < 0) x1--;

				double lineAngle = atan2((l[3] - l[1]), (l[2] - l[0]));
				double slope = qTan(lineAngle);
				//double slope = (l[3] - l[1]) / (l[2] - l[0]); test

				while (qAbs(x1 - x2) > minLineProjLength && K < mNIter) {

					int y1 = qRound(l[1] + (x1 - l[0]) * slope);
					int y2 = qRound(l[1] + (x2 - l[0]) * slope);

					QVector<int> yrPoss1 = QVector<int>();
					for (int di = -delta; di <= delta && y1 + di < edgeMap.rows; di++) {
						if (y1 + di >= 0)
							if (edgeMap.at<uchar>(y1 + di, x1) == 1) yrPoss1.append(y1 + di);
					}

					QVector<int> yrPoss2 = QVector<int>();
					for (int di = -delta; di <= delta && y2 + di < edgeMap.rows; di++) {
						if (y2 + di >= 0)
							if (edgeMap.at<uchar>(y2 + di, x2) == 1) yrPoss2.append(y2 + di);
					}

					if (yrPoss1.size() > 0 && yrPoss2.size() > 0) {
						for (int y1i = 0; y1i < yrPoss1.size(); y1i++) {
							for (int y2i = 0; y2i < yrPoss2.size(); y2i++) {

								double sumVal = 0;
								for (int xi = x1; xi <= x2; xi++)
									for (int yi = -epsilon; yi <= epsilon; yi++) {
										int yc = qRound(l[1] + (xi - l[0]) * slope) + yi;
										if (yc < edgeMap.rows && xi < edgeMap.cols && yc > 0 && xi > 0) sumVal += edgeMap.at<uchar>(yc, xi);
									}

								if (sumVal > currMax.x()) {

									QPointF centerPoint = QPointF(0.5*(x1 + x2), 0.5*(y1 + y2));
									currMax = QVector3D((float)sumVal, (float)(-mRotationFactor * lineAngle), (float)qSqrt((edgeMap.cols*0.5 - centerPoint.x()) * (edgeMap.cols*0.5 - centerPoint.x()) + (edgeMap.rows*0.5 - centerPoint.y()) * (edgeMap.rows*0.5 - centerPoint.y())));
									maxLine = QVector4D((float)x1, (float)y1, (float)x2, (float)y2);
								}

								K++;
							}
						}
					}

					x1++;
					x2--;
				}

				if (direction == HORIZONTAL) {
					if (currMax.x() > 0) {
						computedWeights.append(currMax);
						//if (mRotationFactor == -1) maxLine = QVector4D(maxLine.y(), maxLine.x(), maxLine.w(), maxLine.z());
						mSelectedLines.append(maxLine);
						mSelectedLineTypes.append(0);
					}
				} else {
					if (currMax.x() > 0) {
						currMax = QVector3D(currMax[0], -currMax[1], currMax[2]);
						computedWeights.append(currMax);
						maxLine = QVector4D(maxLine.y(), maxLine.x(), maxLine.w(), maxLine.z());
						//if (mRotationFactor == -1) maxLine = QVector4D(maxLine.y(), maxLine.x(), maxLine.w(), maxLine.z());
						mSelectedLines.append(maxLine);
						mSelectedLineTypes.append(0);
					}
				}
		}
		return computedWeights;
	}

	/// <summary>
	/// Computes the skew based oon the detected lines
	/// </summary>
	/// <param name="weights">The weights.</param>
	/// <param name="imgDiagonal">The img diagonal.</param>
	/// <param name="ok">Ok - is set to false if only one line is detected.</param>
	/// <param name="eta">Eta -  parameter to reject small lines.</param>
	/// <returns></returns>
	double BaseSkewEstimation::skewEst(const QVector<QVector3D>& weights, double imgDiagonal, bool& ok, double eta) {

		if (weights.size() < 1) {
			ok = false;
			return 0.0;
		}

		double maxWeight = 0;
		for (int i = 0; i < weights.size(); i++) {
			if (weights[i].x() > maxWeight) {
				maxWeight = weights.at(i).x();
			}
		}

		QVector<QVector3D> thrWeights = QVector<QVector3D>();
		for (int i = 0; i < weights.size(); i++) {
			if (weights[i].x() / maxWeight > eta) {
				thrWeights.append(QVector3D((float)qSqrt((weights[i].x() / maxWeight - eta) / (1.0 - eta)), (float)(weights[i].y() / M_PI * 180.0), (float)(weights[i].z() / imgDiagonal)));
				//thrWeights.append(QVector3D((weights.at(i).x()/maxWeight - eta) * (weights.at(i).x()/maxWeight - eta), weights.at(i).y() / M_PI * 180, weights.at(i).z() / imgDiagonal));
			}
		}


		QVector<QPointF> saliencyVec = QVector<QPointF>();

		for (double skewAngle = -30; skewAngle <= 30.001; skewAngle += 0.01) {

			double saliency = 0;

			for (int i = 0; i < thrWeights.size(); i++) {
				saliency += thrWeights[i].x() * qExp(-thrWeights[i].z()) * qExp(-0.5 * ((skewAngle - thrWeights[i].y()) * (skewAngle - thrWeights[i].y())) / (mSigma * mSigma));
			}

			saliencyVec.append(QPointF(skewAngle, saliency));
		}

		double maxSaliency = 0;
		double salSkewAngle = 0;

		for (int i = 0; i < saliencyVec.size(); i++) {
			if (maxSaliency < saliencyVec[i].y()) {
				maxSaliency = saliencyVec[i].y();
				salSkewAngle = saliencyVec[i].x();
			}
		}

		for (int i = 0; i < weights.size(); i++) {
			if (weights[i].x() > eta && qAbs(weights[i].y() / M_PI * 180.0 - salSkewAngle) < 0.15)
				mSelectedLineTypes.replace(i, 1);
		}

		if (maxSaliency == 0) { 
			ok = false;
			return 0; 
		}

		ok = true;
		return salSkewAngle;
	}

	/// <summary>
	/// Determines whether this instance is empty.
	/// </summary>
	/// <returns>true if no src image is set.</returns>
	bool BaseSkewEstimation::isEmpty() const {

		return mSrcImg.empty();

	}


	/// <summary>
	/// Summarize the method as string. Not yet implemented.
	/// </summary>
	/// <returns>Summary string.</returns>
	QString BaseSkewEstimation::toString() const
	{
		return QString();
	}

}