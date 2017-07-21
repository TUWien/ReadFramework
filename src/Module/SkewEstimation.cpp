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

#include "SuperPixel.h"
#include "GraphCut.h"

#include "Algorithms.h"
#include "Image.h"
#include "ImageProcessor.h"

#pragma warning(push, 0)	// no warnings from includes
// Qt Includes
#include <opencv2/imgproc.hpp>
#include <QVector3D>
#include <QtCore/qmath.h>
#include <QDebug>
#include <QSettings>
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

		mConfig = QSharedPointer<BaseSkewEstimationConfig>::create();
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

		cv::Mat skewImg = mSrcImg;
		if (mSrcImg.channels() != 1) {
			cv::cvtColor(mSrcImg, skewImg, CV_RGB2GRAY);
		}

		int w, h;
		int delta, epsilon;
		w = config()->width();
		//w = qRound(mSrcImg.cols / 1430.0*49.0); //check  (nomacs plugin version)
		h = config()->height();
		//h = qRound(mSrcImg.rows / 700.0*12.0); //check (nomacs plugin version)
		w = w <= 1 ? 10 : w;
		h = h <= 1 ? 5 : h;
		epsilon = config()->epsilon();
		delta = config()->delta();
		//delta = qRound(mSrcImg.cols / 1430.0*20.0); //check (nomacs plugin version)
		//mMinLineLength = qRound(mSrcImg.cols / 1430.0 * 20.0); //check
		
		//Image::imageInfo(mSrcImg, "horSep");
		//half again to be consistent with original implementation
		int halfW = w / 2;
		int halfH = h / 2;

		//Image::imageInfo(mSrcImg, "srcImg");

		cv::Mat horSep = separability(skewImg, halfW, halfH, mMask);
		cv::Mat verSep = separability(skewImg.t(), halfW, halfH, mMask);
		verSep = verSep.t();

		//Image::imageInfo(horSep, "horSep");
		//Image::imageInfo(verSep, "verSep");

		//cv::Mat sepHV;
		//cv::normalize(horSep, sepHV, 0, 255, cv::NORM_MINMAX, CV_8UC1);
		//Image::save(sepHV, "D:\\tmp\\horSep.png");
		//cv::normalize(verSep, sepHV, 0, 255, cv::NORM_MINMAX, CV_8UC1);
		//Image::save(sepHV, "D:\\tmp\\verSep.png");

		double min, max;
		cv::minMaxLoc(horSep, &min, &max); //* max -> check
		double thr = mFixedThr ? config()->thr() : config()->thr() * max;
		cv::Mat edgeHor = edgeMap(horSep, thr, HORIZONTAL, mMask);
		cv::minMaxLoc(verSep, &min, &max);
		thr = mFixedThr ? config()->thr() : config()->thr() * max;
		cv::Mat edgeVer = edgeMap(verSep, thr, VERTICAL, mMask);

		//Image::save(edgeHor, "D:\\tmp\\edgeHorF.png");
		//Image::save(edgeVer, "D:\\tmp\\edgeVerF.png");

		mSelectedLines.clear();
		mSelectedLineTypes.clear();

		QVector<QVector3D> weightsHor = computeWeights(edgeHor, delta, epsilon, HORIZONTAL);
		QVector<QVector3D> weightsVer = computeWeights(edgeVer.t(), delta, epsilon, VERTICAL);

		QVector<QVector3D> weightsAll = weightsHor + weightsVer;

		////debug: print lines 
		//QVector<QVector4D> selLines = mSelectedLines;
		//cv::Mat inputImg = mSrcImg.clone();
		//for (int iL = 0; iL < selLines.size(); iL++) {
		//	QVector4D line = selLines[iL];

		//	cv::line(inputImg, cv::Point((int)line.x(), (int)line.y()), cv::Point((int)line.z(), (int)line.w()), cv::Scalar(255, 255, 255), 3);

		//}
		//Image::save(inputImg, "D:\\tmp\\lines.png");
		////debug: print lines


		double diagonal = qSqrt(mSrcImg.rows*mSrcImg.rows + mSrcImg.cols*mSrcImg.cols);
		bool ok = true;

		mSkewAngle = skewEst(weightsAll, diagonal, ok);

		//do not save here because it is not thread safe
		//saveSettings();

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

	QVector<QVector4D> BaseSkewEstimation::getSelectedLines() const
	{
		return mSelectedLines;
	}

	/// <summary>
	/// Computes the separability of two neighbouring regions based on mean and variance.
	/// </summary>
	/// <param name="srcImg">The source img.</param>
	/// <param name="w">The width of the region.</param>
	/// <param name="h">The height of the region.</param>
	/// <param name="mask">The optional mask.</param>
	/// <returns>The separability map</returns>
	cv::Mat BaseSkewEstimation::separability(const cv::Mat& srcImg, int w, int h, const cv::Mat& mask)
	{

		cv::Mat grayImg;

		if (srcImg.channels() > 1) {
			cv::cvtColor(srcImg, grayImg, CV_RGB2GRAY);
		}
		else {
			grayImg = srcImg;
		}

		//must be computed, because separability is also called with transposed image
		//if (mIntegralImg.empty() || mIntegralSqdImg.empty()) {
			cv::integral(grayImg, mIntegralImg, mIntegralSqdImg, CV_64F, CV_64F);
		//}

		cv::Mat meanImg, stdImg;

		meanImg = IP::convolveIntegralImage(mIntegralImg, w, h, IP::border_flip); //Algorithms::BORDER_ZERO
		//meanImg /= (float)(w*h);  //not needed since BORDER_FLIP (=mean filtering)
		stdImg = IP::convolveIntegralImage(mIntegralSqdImg, w, h, IP::border_flip); //Algorithms::BORDER_ZERO
		//stdImg /= (float)(w*h);	//not needed since BORDER_FLIP (=mean filtering)
		stdImg = stdImg - meanImg.mul(meanImg); // = sigma^2

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
					for (int k = -config()->kMax(); k <= config()->kMax(); k++) {
						if (k == 0) {
							continue;
						}
						const double* sepKPtr = 0;
						if (direction == HORIZONTAL) {
							if (row + k >= 0 && row + k < separability.rows) {
								sepKPtr = separability.ptr<double>(row + k) + col;
							}
						}
						else if (direction == VERTICAL) {
							if (col + k >= 0 && col + k < separability.cols) {
								sepKPtr = separability.ptr<double>(row) + (col+k);
							}
						}

						if (sepKPtr && (ptrSep[col] < *sepKPtr)) {
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
		int minLineProjLength = config()->minLineLength() / 4;
		//params: rho resolution, theta resolution, threshold, min Line length, max line gap
		cv::HoughLinesP(edgeMap, lines, 1, CV_PI / 180, 50, config()->minLineLength(), 20);

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

				if (std::abs(l[2] - l[0]) <= 1) {
					//qWarning() << "detected line almost vertical - skipping line";
					continue;
				}

				double lineAngle = atan2((l[3] - l[1]), (l[2] - l[0]));
				double slope = qTan(lineAngle);
				//double slope = (l[3] - l[1]) / (l[2] - l[0]); test

				//TODO: instead of x1++ and x2-- check if alternating increment/decrement improves result
				while (qAbs(x1 - x2) > minLineProjLength && K < config()->nIter()) {

					int y1 = qRound(l[1] + (x1 - l[0]) * slope);
					int y2 = qRound(l[1] + (x2 - l[0]) * slope);

					QVector<int> yrPoss1 = QVector<int>();
					for (int di = -delta; di <= delta && y1 + di < edgeMap.rows; di++) {
						if (y1 + di >= 0)
							if (edgeMap.at<uchar>(y1 + di, x1) > 0) yrPoss1.append(y1 + di);
					}

					QVector<int> yrPoss2 = QVector<int>();
					for (int di = -delta; di <= delta && y2 + di < edgeMap.rows; di++) {
						if (y2 + di >= 0)
							if (edgeMap.at<uchar>(y2 + di, x2) > 0) yrPoss2.append(y2 + di);
					}

					//yrPoss1 and YrPoss2 can at most contain one value
					if (yrPoss1.size() > 0 && yrPoss2.size() > 0) {
						
						double lineAngleNew = lineAngle;
						for (int y1i = 0; y1i < yrPoss1.size(); y1i++) {
							int ys1 = yrPoss1.at(y1i);

							for (int y2i = 0; y2i < yrPoss2.size(); y2i++) {
								int ys2 = yrPoss2.at(y2i);

								double sumVal = 0;
								lineAngleNew = std::atan2((ys2 - ys1), (x2 - x1));
								double slopeNew = qTan(lineAngleNew);

								//always entire line length, but define new line based on ys1 and ys2
								for (int xi = l[0]; xi <= l[2]; xi++) {
									for (int yi = -epsilon; yi <= epsilon; yi++) {
										int yc = qRound(ys1 + (xi - x1) * slopeNew) + yi;
										if (yc < edgeMap.rows && xi < edgeMap.cols && yc > 0 && xi > 0) sumVal += (edgeMap.at<uchar>(yc, xi) / 255);
									}
								}

								if (sumVal > currMax.x()) {

									QPointF centerPoint = QPointF(0.5*(x1 + x2), 0.5*(y1 + y2));
									currMax = QVector3D((float)sumVal, (float)(-mRotationFactor * lineAngleNew), (float)qSqrt((edgeMap.cols*0.5 - centerPoint.x()) * (edgeMap.cols*0.5 - centerPoint.x()) + (edgeMap.rows*0.5 - centerPoint.y()) * (edgeMap.rows*0.5 - centerPoint.y())));
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

		//determine fi_opt
		double maxWeight = 0;
		for (int i = 0; i < weights.size(); i++) {
			if (weights[i].x() > maxWeight) {
				maxWeight = weights.at(i).x();
			}
		}

		//according to paper eta should be 0.5
		QVector<QVector3D> thrWeights = QVector<QVector3D>();
		for (int i = 0; i < weights.size(); i++) {
			if (weights[i].x() / maxWeight > eta) {
				//plugin version
				//thrWeights.append(QVector3D(qSqrt((weights.at(i).x() / maxWeight - eta) / (1 - eta)), weights.at(i).y() / M_PI * 180, weights.at(i).z() / imgDiagonal));
				thrWeights.append(QVector3D((float)qPow(weights[i].x() - maxWeight*eta,2), (float)(weights[i].y() / M_PI * 180.0), (float)(weights[i].z() / imgDiagonal)));
			}
		}


		QVector<QPointF> saliencyVec = QVector<QPointF>();

		for (double skewAngle = -30; skewAngle <= 30.001; skewAngle += 0.01) {

			double saliency = 0;

			for (int i = 0; i < thrWeights.size(); i++) {
				//plugin version
				//saliency += thrWeights.at(i).x() * qExp(-thrWeights.at(i).z()) * qExp(-0.5 * ((skewAngle - thrWeights.at(i).y()) * (skewAngle - thrWeights.at(i).y())) / (mSigma * mSigma));
				saliency += thrWeights[i].x() * qExp(-thrWeights[i].z()) * (1/qSqrt(2.0*CV_PI*config()->sigma()*config()->sigma())) * qExp(-0.5 * ((skewAngle - thrWeights[i].y()) * (skewAngle - thrWeights[i].y())) / (config()->sigma() * config()->sigma()));
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
			if (weights[i].x()/maxWeight > eta && qAbs(weights[i].y() / M_PI * 180.0 - salSkewAngle) < 0.15)
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
	/// Checks the input - not yet implemented.
	/// </summary>
	/// <returns></returns>
	bool BaseSkewEstimation::checkInput() const
	{
		return false;
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
		QString msg = debugName();
		msg += "estimated skew: " + QString::number(mSkewAngle);
		msg += config()->toString();

		return msg;
	}

	//void BaseSkewEstimation::setThr(double thr)
	//{
	//	mThr = thr;
	//}

	//void BaseSkewEstimation::setmMinLineLength(int ll)
	//{
	//	mMinLineLength = ll;
	//}

	//void BaseSkewEstimation::setDelta(int d)
	//{
	//	mDelta = d;
	//}

	//void BaseSkewEstimation::setEpsilon(int e)
	//{
	//	mEpsilon = e;
	//}

	//void BaseSkewEstimation::setW(int w)
	//{
	//	mW = w;
	//}

	//void BaseSkewEstimation::setH(int h)
	//{
	//	mH = h;
	//}

	//void BaseSkewEstimation::setSigma(double s)
	//{
	//	mSigma = s;
	//}

	void BaseSkewEstimation::setFixedThr(bool f)
	{
		mFixedThr = f;
	}

	QSharedPointer<BaseSkewEstimationConfig> BaseSkewEstimation::config() const
	{
		return qSharedPointerDynamicCast<BaseSkewEstimationConfig>(mConfig);
	}

	BaseSkewEstimationConfig::BaseSkewEstimationConfig()	{
		mModuleName = "BaseSkewEstimation";
	}

	int BaseSkewEstimationConfig::width() const {
		return mW;
	}

	void BaseSkewEstimationConfig::setWidth(int w) 	{
		mW = w;
	}

	int BaseSkewEstimationConfig::height() const {
		return mH;
	}

	void BaseSkewEstimationConfig::setHeight(int h)	{
		mH = h;
	}

	int BaseSkewEstimationConfig::delta() const
	{
		return mDelta;
	}

	void BaseSkewEstimationConfig::setDelta(int d) 	{
		mDelta = d;
	}

	int BaseSkewEstimationConfig::epsilon() const {
		return mEpsilon;
	}

	void BaseSkewEstimationConfig::setEpsilon(int e) {
		mEpsilon = e;
	}

	int BaseSkewEstimationConfig::minLineLength() const {
		return mMinLineLength;
	}

	void BaseSkewEstimationConfig::setMinLineLength(int l)	{
		mMinLineLength = l;
	}

	//int BaseSkewEstimationConfig::minLineProjLength() const	{
	//	return mMinLineProjLength;
	//}

	//void BaseSkewEstimationConfig::setMinLineProjLength(int l)	{
	//	mMinLineProjLength = l;
	//}

	double BaseSkewEstimationConfig::sigma() const	{
		return mSigma;
	}

	void BaseSkewEstimationConfig::setSigma(double s)	{
		mSigma = s;
	}

	double BaseSkewEstimationConfig::thr() const {
		return mThr;
	}

	void BaseSkewEstimationConfig::setThr(double t)	{
		mThr = t;
	}

	int BaseSkewEstimationConfig::kMax() const	{
		return mKMax;
	}

	void BaseSkewEstimationConfig::setKMax(int k)	{
		mKMax = k;
	}

	int BaseSkewEstimationConfig::nIter() const	{
		return mNIter;
	}

	void BaseSkewEstimationConfig::setNIter(int n)	{
		mNIter = n;
	}

	QString BaseSkewEstimationConfig::toString() const
	{
		QString msg;
		msg += "  W: " + QString::number(mW);
		msg += "  H: " + QString::number(mH);
		msg += "  delta: " + QString::number(mDelta);
		msg += "  epsilon: " + QString::number(mEpsilon);
		msg += "  minLineLength: " + QString::number(mMinLineLength);
		//msg += "  minLineLengthProj: " + QString::number(mMinLineProjLength);
		msg += "  sigma: " + QString::number(mSigma);
		msg += "  thr: " + QString::number(mThr);
		msg += "  kmax: " + QString::number(mKMax);
		msg += "  niter: " + QString::number(mNIter);

		return msg;
	}

	void BaseSkewEstimationConfig::load(const QSettings & settings)	{
			mW = settings.value("sepW", mW).toInt();	
			mH = settings.value("sepH", mH).toInt();
			mThr = settings.value("thr", mThr).toDouble();
			mDelta = settings.value("delta", mDelta).toInt();
			mEpsilon = settings.value("eps", mEpsilon).toInt();
			mMinLineLength = settings.value("minLineLength", mMinLineLength).toInt();
			//mMinLineProjLength = settings.value("minLineLengthProj", mMinLineProjLength).toInt();
			mSigma = settings.value("sigma", mSigma).toDouble();
			mKMax = settings.value("kmax", mKMax).toInt();
			mNIter = settings.value("niter", mNIter).toInt();
	}

	void BaseSkewEstimationConfig::save(QSettings & settings) const	{

			settings.setValue("sepW", mW);
			settings.setValue("sepH", mH);
			settings.setValue("thr", mThr);
			settings.setValue("delta", mDelta);
			settings.setValue("eps", mEpsilon);
			settings.setValue("minLineLength", mMinLineLength);
			//settings.setValue("minLineLengthProj", mMinLineProjLength);
			settings.setValue("sigma", mSigma);
			settings.setValue("kmax", mKMax);
			settings.setValue("niter", mNIter);
	}

	TextLineSkewConfig::TextLineSkewConfig() : ModuleConfig("TextLine Skew") {

	}

	QString TextLineSkewConfig::toString() const {
		return ModuleConfig::toString();
	}

	double TextLineSkewConfig::minAngle() const {
		return mMinAngle;
	}

	double TextLineSkewConfig::maxAngle() const {
		return mMaxAngle;
	}

	void TextLineSkewConfig::load(const QSettings & settings) {

		mMinAngle = settings.value("minAngle", minAngle()).toDouble();
		mMaxAngle = settings.value("maxAngle", maxAngle()).toDouble();
	}

	void TextLineSkewConfig::save(QSettings & settings) const {

		settings.setValue("minAngle", minAngle());
		settings.setValue("minAngle", maxAngle());
	}

	// -------------------------------------------------------------------- TextLineSkew 
	TextLineSkew::TextLineSkew(const cv::Mat& img) : mImg(img) {

		mConfig = QSharedPointer<TextLineSkewConfig>::create();
	}

	bool TextLineSkew::compute() {
		
		if (!checkInput()) {
			qWarning() << "cannot compute " << config()->name();
			return false;
		}

		rdf::SuperPixel superPixel(mImg);
		
		if (!superPixel.compute())
			qWarning() << "could not compute super pixel!";

		mSet = superPixel.pixelSet();

		// configure local orientation module
		QSharedPointer<rdf::LocalOrientationConfig> loc(new rdf::LocalOrientationConfig());
		loc->setNumOrientations(64);

		rdf::LocalOrientation lo(mSet);
		lo.setConfig(loc);
		if (!lo.compute())
			qWarning() << "could not compute local orientation";

		rdf::GraphCutOrientation pse(mSet);

		if (!pse.compute())
			qWarning() << "could not compute set orientation";

		QList<double> angles;

		// get median angle
		for (auto p : mSet.pixels()) {

			if (!p->stats())
				continue;

			angles << p->stats()->orientation();
		}

		// no super pixels found?
		if (angles.empty())
			return false;

		// find the median angle
		double skewAngle = -(rdf::Algorithms::statMoment(angles, 0.5) - CV_PI*0.5);

		// if we have an illegal skew angle try the .75 quantile 
		if (skewAngle < config()->minAngle() || skewAngle > config()->maxAngle()) {

			double tmpSkewAngle = -(rdf::Algorithms::statMoment(angles, 0.75) - CV_PI*0.5);
			if (tmpSkewAngle >= config()->minAngle() && tmpSkewAngle <= config()->maxAngle()) {
				skewAngle = tmpSkewAngle;
				qInfo() << "using 2nd guess for skew angle";
			}
			else
				qInfo() << "2nd guess rejected: " << tmpSkewAngle*DK_RAD2DEG;
		}

		return true;
	}

	double TextLineSkew::getAngle() {
		return mAngle;
	}

	bool TextLineSkew::isEmpty() const {
		return mImg.empty();
	}

	QString TextLineSkew::toString() const {
		return "TextLineSkew";
	}

	QSharedPointer<TextLineSkewConfig> TextLineSkew::config() const {
		return mConfig.dynamicCast<TextLineSkewConfig>();
	}

	cv::Mat TextLineSkew::draw(const cv::Mat & img) const {

		QImage qImg = Image::mat2QImage(img, true);

		QPainter p(&qImg);
		p.setPen(rdf::ColorManager::colors()[1]);

		for (auto px : mSet.pixels())
			px->draw(p, 0.3, rdf::Pixel::DrawFlags() | rdf::Pixel::draw_ellipse | rdf::Pixel::draw_stats);

		if (rdf::Utils::hasGui()) {
			QFont font = p.font();
			font.setPointSize(16);
			p.setFont(font);
			p.drawText(QPoint(40, 40), QObject::tr("angle: %1%2").arg(mAngle*DK_RAD2DEG).arg(QChar(0x00B0)));
		}

		return Image::qImage2Mat(qImg);
	}

	cv::Mat TextLineSkew::rotated(const cv::Mat & img) const {

		// apply angle to image
		return rdf::IP::rotateImage(img, mAngle);
	}

	bool TextLineSkew::checkInput() const {
		return !mImg.empty();
	}

}