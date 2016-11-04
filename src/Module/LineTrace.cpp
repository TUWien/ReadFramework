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

#include "LineTrace.h"
#include "Image.h"
#include "GradientVector.h"
#include "Blobs.h"
#include "Algorithms.h"

#pragma warning(push, 0)	// no warnings from includes
// Qt Includes
#include <QDebug>
#include <QSettings>
#include <qmath.h>
#include "opencv2/imgproc/imgproc.hpp"
#pragma warning(pop)



namespace rdf {


	// LineTrace--------------------------------------------------------------------
	/// <summary>
	/// Initializes a new instance of the <see cref="LineTrace"/> class.
	/// </summary>
	/// <param name="img">The binary input image.</param>
	/// <param name="mask">The mask image.</param>
	LineTrace::LineTrace(const cv::Mat& img, const cv::Mat& mask) {
		mSrcImg = img;
		mMask = mask;

		if (mMask.empty()) {
			mMask = cv::Mat(mSrcImg.size(), CV_8UC1, cv::Scalar(255));
		}

		mConfig = QSharedPointer<LineTraceConfig>::create();

		//mModuleName = "LineTrace";
	}

	bool LineTrace::checkInput() const {

		if (mSrcImg.empty()) {
			mWarning << "Source Image is empty";
			return false;
		}

		if (mSrcImg.channels() != 1) {
			mWarning << "Source Image must have 1 channel, and has: " << mSrcImg.channels();
			return false;
		}

		if (mSrcImg.depth() != CV_8U) {
			mWarning << "Source Image must have CV_8U, and has: " << mSrcImg.depth();
			return false;
		}

		return true;
	}

	/// <summary>
	/// Determines whether this instance is empty.
	/// </summary>
	/// <returns>True if src image and mask is empty.</returns>
	bool LineTrace::isEmpty() const {
		return mSrcImg.empty() && mMask.empty();
	}


	/// <summary>
	/// Gets the horizontal lines.
	/// </summary>
	/// <returns>A line vector containing all horizontal lines</returns>
	QVector<rdf::Line> LineTrace::getHLines() const {
		return hLines;
	}

	/// <summary>
	/// Sets the angle to filter the lines horizontally and vertically.
	/// </summary>
	/// <param name="angle">The angle to define a horizontal line.</param>
	void LineTrace::setAngle(double angle) {
		mAngle = angle;
	}

	/// <summary>
	/// Resets the angle which defines horizontal.
	/// </summary>
	void LineTrace::resetAngle() {
		mAngle = std::numeric_limits<double>::infinity();
	}

	QSharedPointer<LineTraceConfig> LineTrace::config() const {

		return qSharedPointerDynamicCast<LineTraceConfig>(mConfig);		
	}

	/// <summary>
	/// Gets the vertical lines.
	/// </summary>
	/// <returns>A line vector containing all vertical lines</returns>
	QVector<rdf::Line> LineTrace::getVLines() const {
		return vLines;
	}

	QVector<rdf::Line> LineTrace::getLines() const
	{
		return hLines+vLines;
	}

	/// <summary>
	/// Computes the binary line image as will as the line vectors.
	/// </summary>
	/// <returns>True if the lines are computed successfully.</returns>
	bool LineTrace::compute() {

		if (!checkInput())
			return false;

		cv::Mat hDSCCImg = hDSCC(mSrcImg);
		cv::Mat transposedSrc = mSrcImg.t();
		cv::Mat vDSCCImg = hDSCC(transposedSrc).t();

		//mDAngle = 0.0;

		filter(hDSCCImg, vDSCCImg);

		//Image::save(hDSCCImg, "D:\\tmp\\hdscc.tif");
		//Image::save(vDSCCImg, "D:\\tmp\\vdscc.tif");
		//return true;

		mLineImg = cv::Mat(mSrcImg.rows, mSrcImg.cols, CV_8UC1);
		cv::bitwise_or(hDSCCImg, vDSCCImg, mLineImg);

		//Image::save(mLineImg, "D:\\tmp\\mLineImg.tif");

		//if (mDAngle != 361.0f) {
		if (!std::isinf(mAngle)) {
			QVector<rdf::Line> tmp;
			tmp.append(hLines);
			tmp.append(vLines);

			hLines = filterLineAngle(tmp, (float)mAngle, config()->maxSlopeRotat());
			vLines = filterLineAngle(tmp, (float)(mAngle+CV_PI*0.5f), config()->maxSlopeRotat());
		}

		//Image::save(hDSCCImg, "D:\\tmp\\hdscc.tif");
		//Image::save(vDSCCImg, "D:\\tmp\\vdscc.tif");

		QVector<rdf::Line> gapLines;
		gapLines      = mergeLines(hLines);
		gapLines.append(mergeLines(vLines));
		drawGapLines(mLineImg, gapLines);

		filterLines();

		rdf::Blobs finalBlobs;
		finalBlobs.setImage(mLineImg);
		finalBlobs.compute();

		finalBlobs.setBlobs(rdf::BlobManager::instance().filterMar(1.0f, config()->minLenSecondRun(), finalBlobs));
		mLineImg = rdf::BlobManager::instance().drawBlobs(finalBlobs);

		return true;
	}


	///// <summary>
	///// Sets the minimum length for the final line filtering.
	//// Default value = 60;
	///// </summary>
	///// <param name="len">The minimum length threshold in pixel.</param>
	//void LineTrace::setMinLenSecondRun(int len) {
	//	mMinLenSecondRun = len;
	//}

	///// <summary>
	///// Sets the maximum aspect ratio allowed for the minimum area rectangle of a line.
	//// Default value = 0.3.
	///// </summary>
	///// <param name="ratio">The max aspect ratio of a MAR of a line.</param>
	//void LineTrace::setMaxAspectRatio(float ratio) {
	//	mMaxAspectRatio = ratio;
	//}

	/// <summary>
	/// Generates the line image based on the synthetic line vector.
	/// </summary>
	/// <returns>A binary CV_8UC1 image where all line vectors are drawn.</returns>
	cv::Mat LineTrace::generatedLineImage() const {

		if (mSrcImg.empty()) return cv::Mat();

		cv::Mat synLinImg = cv::Mat(mSrcImg.rows, mSrcImg.cols, CV_8UC1);
		synLinImg.setTo(0);

		for (auto l : hLines) {

			cv::Point2d pStart(l.p1().x(), l.p1().y());
			cv::Point2d pEnd(l.p2().x(), l.p2().y());

			cv::line(synLinImg, pStart, pEnd, cv::Scalar(255), (int)l.thickness(), 8, 0);
		}

		for (auto l : vLines) {

			cv::Point2d pStart(l.p1().x(), l.p1().y());
			cv::Point2d pEnd(l.p2().x(), l.p2().y());

			cv::line(synLinImg, pStart, pEnd, cv::Scalar(255), (int)l.thickness(), 8, 0);
		}

		return synLinImg;
	}

	void LineTrace::generateLineImage(const QVector<rdf::Line>& hline, const QVector<rdf::Line>& vline, cv::Mat & img, cv::Scalar hCol, cv::Scalar vCol)
	{
		if (hline.empty() && vline.empty())
			return;

		//img.setTo(0);

		for (auto l : hline) {

			cv::Point2d pStart(l.p1().x(), l.p1().y());
			cv::Point2d pEnd(l.p2().x(), l.p2().y());

			cv::line(img, pStart, pEnd, hCol, (int)l.thickness(), 8, 0);
		}

		for (auto l : vline) {

			cv::Point2d pStart(l.p1().x(), l.p1().y());
			cv::Point2d pEnd(l.p2().x(), l.p2().y());

			cv::line(img, pStart, pEnd, vCol, (int)l.thickness(), 8, 0);
		}
	}

	QVector<rdf::Line> LineTrace::mergeLines(QVector<rdf::Line>& lines) {

		QVector<rdf::Line>::iterator lineIter, lineIterCmp;
		QVector<rdf::Line> gapLines;

		int lineIdx, lineCmpIdx;
		for (lineIter = lines.begin(), lineIdx = 0; lineIter != lines.end(); ) {
			rdf::Line cLine = *lineIter;

			for (lineIterCmp = lineIter, lineCmpIdx = lineIdx; lineIterCmp != lines.end(); lineIterCmp++, lineCmpIdx++) {
				if (lineIterCmp == lineIter) {
					continue;
				}

				rdf::Line cmpLine = *lineIterCmp;
				double dist = cLine.minDistance(cmpLine);

				if (dist > config()->maxGap()) continue;

				double diffAngle = cLine.diffAngle(cmpLine);
				if (diffAngle > qDegreesToRadians(config()->maxSlopeDiff())) continue;
				
				//orientation of new line differ in orientation with connecting lines by maxSlopeDiff degree
				//difference in orientation between gapline and cline/cmpline. the larger deviation is taken
				rdf::Line newLine, gapLine;
				newLine = cLine.merge(cmpLine);

				double angle1 = newLine.diffAngle(cLine);
				double angle2 = newLine.diffAngle(cmpLine);
				double angle = angle1 > angle2 ? angle1 : angle2;

				double weight = 1.0f - (dist / config()->maxGap());
				if (angle > (weight * qDegreesToRadians(config()->maxAngleDiff()))) continue;

				gapLine = cLine.gapLine(cmpLine);
				angle1 = gapLine.diffAngle(cLine);
				angle2 = gapLine.diffAngle(cmpLine);
				angle = angle1 > angle2 ? angle1 : angle2;

				weight = 1.0f - (dist / config()->maxGap())*(dist / config()->maxGap());
				if (dist > 5 && angle > weight * qDegreesToRadians(20.0f)) continue;

				gapLines.append(gapLine);

				*lineIterCmp = newLine;

				lineIter = lines.erase(lineIter);
				break;
			}

			if (lineCmpIdx == (int)lines.size()) {
				lineIter++;
				lineIdx++;
			}
		}



		QVector<int> eraseIdx;

		// now remove small lines 'within' large lines
		for (lineIter = lines.begin(), lineIdx = 0, lineIdx = 0; lineIter != lines.end(); ) {
			rdf::Line cLine = *lineIter;

			for (lineIterCmp = lineIter, lineCmpIdx = lineIdx; lineIterCmp != lines.end(); lineIterCmp++, lineCmpIdx++) {

				if (lineIterCmp == lineIter) {
					continue;
				}

				rdf::Line cmpLine = *lineIterCmp;

				if (cmpLine.distance(cLine.line().p1()) > 5 || cmpLine.distance(cLine.line().p2()) > 5)
					continue;

				if (cmpLine.within(cLine.line().p1()) && cmpLine.within(cLine.line().p2())) {
					eraseIdx.push_back(lineIdx);
				}
				else if (cLine.within(cmpLine.line().p1()) && cLine.within(cmpLine.line().p2())) {
					eraseIdx.push_back(lineCmpIdx);
				}
			}

			if (lineCmpIdx == (int)lines.size()) {
				lineIter++;
				lineIdx++;
			}
		}

		qSort(eraseIdx.begin(), eraseIdx.end());

		//dout << "size: " << (int)lineVector.size() << dkendl;
		//use tmIdx to avoid double entries... deletes additional lines and can cause an exception!
		int tmpIdx = -1;

		for (int idx = ((int)eraseIdx.size()) - 1; idx >= 0; idx--) {
			lineIter = lines.begin();
			lineIter += eraseIdx[idx];

			if (tmpIdx == eraseIdx[idx])
				continue;
			//dout << "idx: " << eraseIdx[idx] << dkendl;
			lines.erase(lineIter);
			tmpIdx = eraseIdx[idx];
		}




		return gapLines;
	}

	void LineTrace::drawGapLines(cv::Mat& img, QVector<rdf::Line> lines) {

		for (auto l : lines) {

			cv::Point2d pStart(l.p1().x(), l.p1().y());
			cv::Point2d pEnd(l.p2().x(), l.p2().y());

			cv::line(img, pStart, pEnd, cv::Scalar(255), (int)l.thickness(), 8, 0);
		}

	}

	void LineTrace::filterLines() {
		QVector<rdf::Line> tmp;

		for (auto l : hLines) {
			if (l.length() > (float)config()->minLenSecondRun())
				tmp.append(l);
		}
		hLines = tmp;

		tmp.clear();
		for (auto l : vLines) {
			if (l.length() > (float)config()->minLenSecondRun())
				tmp.append(l);
		}
		vLines = tmp;

	}

	/// <summary>
	/// Filters all lines accordinge a specified angle and the allowed angle difference angleDiff.
	/// </summary>
	/// <param name="lines">The line vector.</param>
	/// <param name="angle">The angle in rad.</param>
	/// <param name="angleDiff">The maximal allowed angle difference in degree.</param>
	/// <returns>The filtered line vector.</returns>
	QVector<rdf::Line> LineTrace::filterLineAngle(const QVector<rdf::Line>& lines, float angle, float angleDiff) const {

		QVector<rdf::Line> resultLines;

		for (auto l : lines) {
			
			double a = Algorithms::normAngleRad(angle, 0.0, CV_PI);
			double angleNewLine = Algorithms::normAngleRad(l.angle(), 0.0, CV_PI);

			double diffangle = cv::min(fabs(Algorithms::normAngleRad(a, 0, CV_PI) - Algorithms::normAngleRad(angleNewLine, 0, CV_PI))
				, CV_PI - fabs(Algorithms::normAngleRad(a, 0, CV_PI) - Algorithms::normAngleRad(angleNewLine, 0, CV_PI)));

			if (diffangle < angleDiff / 180.0*CV_PI)
				resultLines.append(l);

		}
		return resultLines;
	}

	cv::Mat LineTrace::hDSCC(const cv::Mat& bwImg) const {

		//std::vector<int> invalidLabels;
		//std::vector<int> currentLen;
		QVector<int> invalidLabels;
		QVector<int> currentLen;

		int equivalenceLbl[2];
		equivalenceLbl[0] = 0;
		equivalenceLbl[1] = 0;

		cv::Mat tmp32F;

		int width, height;
		int label;
		int runlen;
		int lastupprleft = 0;
		float leftNeighbour;
		float upprNeighbour;
		float rightNeighbour;
		float leftupprNeighbour;
		float rightupprNeighbour;

		height = bwImg.rows;
		width = bwImg.cols;

		label = 1;
		runlen = 1;

		//DkIP::imwrite("bwimg464.png", bwImg);

		bwImg.convertTo(tmp32F, CV_32F);
		cv::Mat horizontalDSCC;
		horizontalDSCC.create(bwImg.rows, bwImg.cols, CV_8UC1);
		horizontalDSCC.setTo(0);

		//first pass - label run lengths and mark it as valid or invalid...
		for (int col = 0; col<width; col++) {
			for (int row = 0; row<height; row++) {

				float* ptrBw = tmp32F.ptr<float>(row);

				ptrBw += col;

				if (row >= 1) leftNeighbour = (tmp32F.ptr<float>(row - 1))[col];
				else leftNeighbour = 0;
				if (row < (height - 1)) rightNeighbour = (tmp32F.ptr<float>(row + 1))[col];
				else rightNeighbour = 0;
				if (col >= 1) upprNeighbour = (tmp32F.ptr<float>(row))[col - 1];
				else upprNeighbour = 0;

				if (row >= 1 && col >= 1) leftupprNeighbour = (tmp32F.ptr<float>(row - 1))[col - 1];
				else leftupprNeighbour = 0;

				if (row < (height - 1) && col >= 1) rightupprNeighbour = (tmp32F.ptr<float>(row + 1))[col - 1];
				else rightupprNeighbour = 0;

				//if (col==1719 && row==1273)
				//	printf("debug\n");

				if ((*ptrBw) != 0) {
					//no neighbours -> assign new label
					if ((leftNeighbour == 0) && (upprNeighbour == 0)) {
						(*ptrBw) = (float)label;
						invalidLabels.push_back(-1);

						if (leftupprNeighbour != 0)
							invalidLabels[label - 1] = col;

						label++;
					}
					//only left neighbour -> copy the label from the left pixel
					if ((leftNeighbour != 0) && (upprNeighbour == 0)) {
						(*ptrBw) = leftNeighbour;

						//printf("invalidLabels Size: %i  leftNeighbour: %i\n", invalidLabels.size(), (int)leftNeighbour);

						if (rightNeighbour == 0 && rightupprNeighbour != 0)
							invalidLabels[(int)leftNeighbour - 1] = col;

						runlen++;
					}

					//only upper Neighbour -> create new label
					if ((leftNeighbour == 0) && (upprNeighbour != 0)) {
						(*ptrBw) = (float)label;
						invalidLabels.push_back(-1);
						lastupprleft = (int)upprNeighbour;  //memory for the last upprNeighbour
															//if upprNeighbour is invalid -> mark current RL as invalid
						if (invalidLabels[(int)upprNeighbour - 1] == col) {
							invalidLabels[label - 1] = col;
							//if upprNeighbour has an equivalent -> mark all as invalid
						}
						else if (equivalenceLbl[0] == (int)upprNeighbour) {
							invalidLabels[(int)upprNeighbour - 1] = col;
							invalidLabels[equivalenceLbl[1] - 1] = col;
							invalidLabels[label - 1] = col;
							//assign upprNeighbour current label as equivalent
						}
						else {
							equivalenceLbl[0] = (int)upprNeighbour;
							equivalenceLbl[1] = label;
							//equivalenceMap[(int)upprNeighbour] = label;
							//equivalenceMap[label] = (int)upprNeighbour;
						}
						label++;
					}
					//left and upper label -> copy label from the left
					if ((leftNeighbour != 0) && (upprNeighbour != 0)) {

						(*ptrBw) = leftNeighbour;
						runlen++;
						//if current label is invalid (leftneighbour) -> assign upper neighbour as invalid
						if (invalidLabels[(int)leftNeighbour - 1] == col) {
							invalidLabels[(int)upprNeighbour - 1] = col;
							//if upprNeighbour has an equivalent (and it is not the same RL) -> mark all as invalid
						}
						else if ((equivalenceLbl[1] == leftNeighbour) && (equivalenceLbl[0] != upprNeighbour)) {
							invalidLabels[(int)upprNeighbour - 1] = col;
							invalidLabels[equivalenceLbl[0] - 1] = col;
							invalidLabels[(int)leftNeighbour - 1] = col;
							//if lastupprleft is equivalant to current label -> assign all as invalid
						}
						else if ((equivalenceLbl[1] == leftNeighbour) && (equivalenceLbl[0] == lastupprleft) && (upprNeighbour != lastupprleft)) {
							invalidLabels[(int)upprNeighbour - 1] = col;
							invalidLabels[lastupprleft - 1] = col;
							invalidLabels[(int)leftNeighbour - 1] = col;
							//else assign as equivalent
						}
						else if (equivalenceLbl[1] != leftNeighbour) {
							equivalenceLbl[0] = (int)upprNeighbour;
							equivalenceLbl[1] = (int)leftNeighbour;

							//equivalenceMap[(int)upprNeighbour] = (int)leftNeighbour;
							lastupprleft = (int)leftNeighbour;
						}
					}

					if (rightNeighbour == 0) {
						//if (runlen > len) markiere ptrBw[col] als ungültig
						currentLen.push_back(runlen);
						//current runlength has an upper neighbour
						if (equivalenceLbl[1] == label - 1) {
							int uppr = equivalenceLbl[0];
							//int len = currentLen[uppr-1];
							//if runlength is longer than maxlenDiff * upprNeighbour delete runlenghts (cross points!)
							if ((((float)runlen > config()->maxLenDiff()*(float)(currentLen[uppr - 1])) ||
								(config()->maxLenDiff()*(float)runlen <= (float)(currentLen[uppr - 1]))) && (runlen > 5)) {

								invalidLabels[(int)(*ptrBw) - 1] = col;
								invalidLabels[uppr - 1] = col;

							}
							//if runlen <= 5 pixel apply a fixed threshold of 5 pixel
							if ((runlen <= 5) && (abs(currentLen[uppr - 1] - runlen) >= 4)) {
								invalidLabels[(int)(*ptrBw) - 1] = col;
								invalidLabels[uppr - 1] = col;
							}
						}
						//runlength greater maximal alloewd
						if (runlen > config()->maxLen()) {
							invalidLabels[(int)(*ptrBw) - 1] = col;
						}

						runlen = 1;
					}
				}
			}
			//equivalenceMap.clear();
			lastupprleft = 0;
			equivalenceLbl[0] = 0;
			equivalenceLbl[1] = 0;
		}


		if (invalidLabels.size() == 0) {
			invalidLabels.clear();
		}
		currentLen.clear();

		//second pass - delete all invalid labels
		for (int row = 0; row<height; row++) {
			float* ptrBw = tmp32F.ptr<float>(row);
			unsigned char* result = horizontalDSCC.ptr<unsigned char>(row);
			for (int col = 0; col<width; col++) {
				//search for ptrBw[col] in valid list
				//if in list ptrBw[col] = 0;
				if (ptrBw[col] == 0) continue;
				if (invalidLabels[(int)ptrBw[col] - 1] > 0)
					result[col] = 0;
				else if (ptrBw[col] != 0)
					result[col] = 255;
			}
		}

		//DkIP::imwrite("horizontalDSCC617.png", horizontalDSCC);

		invalidLabels.clear();

		return horizontalDSCC;
	}

	void LineTrace::filter(cv::Mat& hDSCCImg, cv::Mat& vDSCCImg) {


		rdf::Blobs binBlobsH;
		binBlobsH.setImage(hDSCCImg);
		binBlobsH.compute();

		binBlobsH.setBlobs(rdf::BlobManager::instance().filterMar(config()->maxAspectRatio(), config()->minWidth(), binBlobsH));
		if (!std::isinf(mAngle)) 
			binBlobsH.setBlobs(rdf::BlobManager::instance().filterAngle((float)mAngle, config()->maxSlopeRotat(), binBlobsH));

		hLines = rdf::BlobManager::instance().lines(binBlobsH);
		hDSCCImg = rdf::BlobManager::instance().drawBlobs(binBlobsH);


		rdf::Blobs binBlobsV;
		binBlobsV.setImage(vDSCCImg);
		binBlobsV.compute();

		binBlobsV.setBlobs(rdf::BlobManager::instance().filterMar(config()->maxAspectRatio(), config()->minWidth(), binBlobsV));
		if (!std::isinf(mAngle))
			binBlobsV.setBlobs(rdf::BlobManager::instance().filterAngle((float)mAngle, config()->maxSlopeRotat(), binBlobsV));

		vLines = rdf::BlobManager::instance().lines(binBlobsV);
		vDSCCImg = rdf::BlobManager::instance().drawBlobs(binBlobsV);

	}

	/// <summary>
	/// Summary of the class.
	/// </summary>
	/// <returns>A String containing all parameter values.</returns>
	QString LineTrace::toString() const {

		QString msg = debugName();
		msg += "number of detected horizontal lines: " + QString::number(hLines.size());
		msg += "number of detected vertical lines: " + QString::number(vLines.size());
		msg += config()->toString();

		return msg;
	}

	//void LineTrace::load(const QSettings& settings) {

	//	//mErodeMaskSize = settings.value("erodeMaskSize", mErodeMaskSize).toInt();
	//}

	//void LineTrace::save(QSettings& settings) const {

	//	//settings.setValue("erodeMaskSize", mErodeMaskSize);
	//}

	/// <summary>
	/// Returns the line image.
	/// </summary>
	/// <returns>A binary CV_8UC1 image containing the lines.</returns>
	cv::Mat LineTrace::lineImage() const {
		return mLineImg;
	}



	LineTraceConfig::LineTraceConfig() 	{
		mModuleName = "LineTrace";
	}

	float LineTraceConfig::maxSlopeRotat() const {
		return mMaxSlopeRotat;
	}

	void LineTraceConfig::setMaxSlopeRotat(float s) {
		mMaxSlopeRotat = s;
	}

	float LineTraceConfig::maxLenDiff() const {
		return mMaxLenDiff;
	}

	void LineTraceConfig::setMaxLenDiff(float l) {
		mMaxLenDiff = l;
	}

	float LineTraceConfig::maxAspectRatio() const {
		return mMaxAspectRatio;
	}

	void LineTraceConfig::setMaxAspectRatio(float a) {
		mMaxAspectRatio = a;
	}

	int LineTraceConfig::minWidth() const	{
		return mMinWidth;
	}

	void LineTraceConfig::setMinWidth(int w) {
		mMinWidth = w;
	}

	int LineTraceConfig::maxLen() const {
		return mMaxLen;
	}

	void LineTraceConfig::setMaxLen(int l)	{
		mMaxLen = l;
	}

	int LineTraceConfig::minArea() const {
		return mMinArea;
	}

	void LineTraceConfig::setMinArea(int a) {
		mMinArea = a;
	}

	int LineTraceConfig::rippleLen() const 	{
		return mRippleLen;
	}

	void LineTraceConfig::setRippleLen(int r) {
		mRippleLen = r;
	}

	float LineTraceConfig::rippleArea() const {
		return mRippleArea;
	}

	void LineTraceConfig::setRippleArea(float a) {
		mRippleArea = a;
	}

	float LineTraceConfig::maxGap() const {
		return mMaxGap;
	}

	void LineTraceConfig::setMaxGap(float g) {
		mMaxGap = g;
	}

	float LineTraceConfig::maxSlopeDiff() const {
		return mMaxSlopeDiff;
	}

	void LineTraceConfig::setMaxSlopeDiff(float s)	{
		mMaxSlopeDiff = s;
	}

	float LineTraceConfig::maxAngleDiff() const {
		return mMaxAngleDiff;
	}

	void LineTraceConfig::setMaxAngleDiff(float a)	{
		mMaxAngleDiff = a;
	}

	int LineTraceConfig::minLenSecondRun() const {
		return mMinLenSecondRun;
	}

	void LineTraceConfig::setMinLenSecondRun(int r)
	{
		mMinLenSecondRun = r;
	}

	float LineTraceConfig::maxDistExtern() const {
		return mMaxDistExtern;
	}

	void LineTraceConfig::setMaxDistExtern(float d)	{
		mMaxDistExtern = d;
	}

	float LineTraceConfig::maxAngleDiffExtern() const {
		return mMaxAngleDiffExtern;
	}

	void LineTraceConfig::setMaxAngleDiffExtern(float a) {
		mMaxAngleDiffExtern = a;
	}

	QString LineTraceConfig::toString() const {
		QString msg;
		msg += "  mMaxSlopeRotat: " + QString::number(mMaxSlopeRotat);
		msg += "  mMaxAspectRatio: " + QString::number(mMaxAspectRatio);
		msg += "  mMinWidth: " + QString::number(mMinWidth);
		msg += "  mMaxLen: " + QString::number(mMaxLen);
		msg += "  mMaxGap: " + QString::number(mMaxGap);
		msg += "  mMaxSlopeDiff: " + QString::number(mMaxSlopeDiff);
		msg += "  mMinLenSecondRun: " + QString::number(mMinLenSecondRun);

		return msg;
	}

	void LineTraceConfig::load(const QSettings & settings) 	{
		
		mMaxSlopeRotat = settings.value("maxSlopeRotat", mMaxSlopeRotat).toFloat();
		mMaxLenDiff = settings.value("maxLenDiff", mMaxLenDiff).toFloat();
		mMaxAspectRatio = settings.value("maxAspectRatio", mMaxAspectRatio).toFloat();
		mMinWidth = settings.value("minWidth", mMinWidth).toInt();
		mMaxLen = settings.value("maxLen", mMaxLen).toInt();
		mMinArea = settings.value("minArea", mMinArea).toInt();
		mRippleLen = settings.value("rippleLen", mRippleLen).toInt();
		mRippleArea = settings.value("rippleArea", mRippleArea).toFloat();
		mMaxGap = settings.value("maxGap", mMaxGap).toFloat();
		mMaxSlopeDiff = settings.value("maxSlopeDiff", mMaxSlopeDiff).toFloat();
		mMaxAngleDiff = settings.value("maxAngleDiff", mMaxAngleDiff).toFloat();
		mMinLenSecondRun = settings.value("minLenSecondRun", mMinLenSecondRun).toInt();

		mMaxDistExtern = settings.value("maxDistExtern", mMaxDistExtern).toFloat();
		mMaxAngleDiffExtern = settings.value("maxAngleDiffExtern", mMaxAngleDiffExtern).toFloat();
	}

	void LineTraceConfig::save(QSettings & settings) const	{
		
		settings.setValue("maxSlopeRotat", mMaxSlopeRotat);
		settings.setValue("maxLenDiff", mMaxLenDiff);
		settings.setValue("maxAspectRatio", mMaxAspectRatio);
		settings.setValue("minWidth", mMinWidth);
		settings.setValue("maxLen", mMaxLen);
		settings.setValue("minArea", mMinArea);
		settings.setValue("rippleLen", mRippleLen);
		settings.setValue("rippleArea", mRippleArea);
		settings.setValue("maxGap", mMaxGap);
		settings.setValue("maxSlopeDiff", mMaxSlopeDiff);
		settings.setValue("maxAngleDiff", mMaxAngleDiff);
		settings.setValue("minLenSecondRun", mMinLenSecondRun);

		settings.setValue("maxDistExtern", mMaxDistExtern);
		settings.setValue("maxAngleDiffExtern", mMaxAngleDiffExtern);
	}

	ReadLSD::ReadLSD(const cv::Mat & img, const cv::Mat & mask)
	{
		mSrcImg = img;
		mMask = mask;

		if (mMask.empty()) {
			mMask = cv::Mat(mSrcImg.size(), CV_8UC1, cv::Scalar(255));
		}

		mConfig = QSharedPointer<ReadLSDConfig>::create();
	}

	bool ReadLSD::isEmpty() const
	{
		return mSrcImg.empty();
	}

	bool ReadLSD::compute()
	{
		/* angle tolerance */
		double prec = CV_PI * config()->angleThr() / 180.0;
		if (prec < 0.0) mWarning << "isaligned: 'prec' must be positive.";
		double p = config()->angleThr() / 180.0;
		double rho = config()->quant() / sin(prec); /* gradient magnitude threshold */
		double logNT = 0;
		int minRegSize = 0;

		cv::Mat scaledImg = mSrcImg;
		//scale image if necessary
		if (config()->scale() != 1.0) {
			//scale image
			//TODO: in original source code a gaussian filter with config()->sigmaScale()
			//and a sampling of the original image is used:
			//cv::GaussianBlur(mSrcImg, scaledImg, cv::Size(0,0), config()->sigmaScale(), config()->sigmaScale());
			//cv::resize(scaledImg, scaledImg, cv::Size(), (double)config()->scale(), (double)config()->scale(), cv::INTER_NEAREST;
			
			//here cv::resize is used with bilinear interpolation
			cv::resize(mSrcImg, scaledImg, cv::Size(), (double)config()->scale(), (double)config()->scale(), cv::INTER_LINEAR);
		}

		//compute Gradients
		GradientVector gr(scaledImg);
		if (!gr.compute()) {
			mWarning << "could not compute gradients in ReadLSD compute...";
			return false;
		}
		mMagImg = gr.magImg();
		mRadImg = gr.radImg();

		/* Number of Tests - NT

		The theoretical number of tests is Np.(XY)^(5/2)
		where X and Y are number of columns and rows of the image.
		Np corresponds to the number of angle precisions considered.
		As the procedure 'rect_improve' tests 5 times to halve the
		angle precision, and 5 more times after improving other factors,
		11 different precision values are potentially tested. Thus,
		the number of tests is
		11 * (X*Y)^(5/2)
		whose logarithm value is
		log10(11) + 5/2 * (log10(X) + log10(Y)).
		*/
		logNT = 5.0 * (log10((double)scaledImg.cols) + log10((double)scaledImg.rows)) / 2.0
			+ log10(11.0);
		minRegSize = (int)(-logNT / log10(p)); /* minimal number of points in region
												 that can give a meaningful event*/
		
		
		cv::Mat idxMat;
		cv::Mat rM = mMagImg.reshape(1, 1);
		//if this is too slow use histogram binning as proposed in
		//the original version of LSD implementation
		cv::sortIdx(rM, idxMat, CV_SORT_EVERY_ROW + CV_SORT_DESCENDING);

		
		mRegionImg = cv::Mat(scaledImg.size(), CV_64FC1);
		mRegionImg.setTo(0);
		QVector<cv::Point> region;
		int regionIdx = 1;

		int *ptrIdx = idxMat.ptr<int>(0);
		for (int colIdx = 0; colIdx < rM.cols; colIdx++) {

			int y = ptrIdx[colIdx] / mMagImg.cols;
			int x = ptrIdx[colIdx] % mMagImg.cols;

			double angle = regionGrow(x, y, region, regionIdx, rho, prec);
			qDebug() << "new angle is: " << angle;

			if (region.size() < minRegSize) {
				region.clear();
				//do not use region, but it is labelled in mRegionImg
				regionIdx++;
				continue;
			}

			/* construct rectangular approximation for the region */
			region2Rect(region, mMagImg, angle, prec, p);

			regionIdx++;
			region.clear();
		}





		return false;
	}

	double ReadLSD::regionGrow(int x, int y, QVector<cv::Point>& region, int regionIdx, double thr, double prec)
	{
		region.push_back(cv::Point(x, y));
		double angle = mRadImg.at<double>(y, x);

		for (int regSizeIdx = 0; regSizeIdx < region.size(); regSizeIdx++) {

			int xt = region[regSizeIdx].x;
			int yt = region[regSizeIdx].y;
			mRegionImg.at<double>(yt, xt) = regionIdx;
			double sumdx = cos(angle);
			double sumdy = sin(angle);

			for (int xx = xt - 1; xx <= xt + 1; xx++ ) {
				for (int yy = yt - 1; yy <= yt + 1; yy++) {
					double regIdx = mRegionImg.at<double>(yy, xx);
					double magnitude = mMagImg.at<double>(yy, xx);
					double cmpAngle = mRadImg.at<double>(yy, xx);

					if (xx >= 0 && yy >= 0 && xx < mRegionImg.cols && yy < mRegionImg.rows &&
						regIdx == 0 && isAligned(cmpAngle,angle, prec) && magnitude < thr) {

						mRegionImg.at<double>(yy, xx) = (double)regionIdx;
						region.push_back(cv::Point(xx, yy));
						sumdx += cos(mMagImg.at<double>(yy, xx));
						sumdy += sin(mMagImg.at<double>(yy, xx));
						angle = atan2(sumdy, sumdx);
					}
				}
			}
		}

		return angle;
	}

	void ReadLSD::region2Rect(QVector<cv::Point>& region, const cv::Mat & magImg, double angle, double prec, double p)	{

		double x, y, dx, dy, l, w, theta, weight, sum, l_min, l_max, w_min, w_max;
		int i;

		if (region.isEmpty()) {
			mWarning << "empty region in region2rect";
			return;
		}
		if (region.size() == 1) {
			mWarning << "region in region2rect contains only 1 pixel";
			return;
		}
		/* center of the region:

		It is computed as the weighted sum of the coordinates
		of all the pixels in the region. The norm of the gradient
		is used as the weight of a pixel. The sum is as follows:
		cx = \sum_i G(i).x_i
		cy = \sum_i G(i).y_i
		where G(i) is the norm of the gradient of pixel i
		and x_i,y_i are its coordinates.
		*/
		x = y = sum = 0.0;
		
		for (i = 0; i < region.size(); i++) {
			weight = magImg.at<double>(region[i].y, region[i].x);
			x += (double)region[i].x*weight;
			y += (double)region[i].y*weight;
			sum += weight;
		}
		if (sum <= 0.0) {
			mWarning << "region2rect: weights sum equal to zero.";
			return;
		}
		x /= sum;
		y /= sum;
		theta = getTheta(region, magImg, angle, prec, x, y);

		/* length and width:

		'l' and 'w' are computed as the distance from the center of the
		region to pixel i, projected along the rectangle axis (dx,dy) and
		to the orthogonal axis (-dy,dx), respectively.

		The length of the rectangle goes from l_min to l_max, where l_min
		and l_max are the minimum and maximum values of l in the region.
		Analogously, the width is selected from w_min to w_max, where
		w_min and w_max are the minimum and maximum of w for the pixels
		in the region.
		*/
		dx = cos(theta);
		dy = sin(theta);
		l_min = l_max = w_min = w_max = 0.0;
		for (i = 0; i<region.size(); i++)
		{
			l = ((double)region[i].x - x) * dx + ((double)region[i].y - y) * dy;
			w = -((double)region[i].x - x) * dy + ((double)region[i].y - y) * dx;

			if (l > l_max) l_max = l;
			if (l < l_min) l_min = l;
			if (w > w_max) w_max = w;
			if (w < w_min) w_min = w;
		}

		/* store values of final rectangle*/
		//rec->x1 = x + l_min * dx;
		//rec->y1 = y + l_min * dy;
		//rec->x2 = x + l_max * dx;
		//rec->y2 = y + l_max * dy;
		//rec->width = w_max - w_min;
		//rec->x = x;
		//rec->y = y;
		//rec->theta = theta;
		//rec->dx = dx;
		//rec->dy = dy;
		//rec->prec = prec;
		//rec->p = p;

		/* we impose a minimal width of one pixel

		A sharp horizontal or vertical step would produce a perfectly
		horizontal or vertical region. The width computed would be
		zero. But that corresponds to a one pixels width transition in
		the image.
		*/
		//if (rec->width < 1.0) rec->width = 1.0

	}

	double ReadLSD::getTheta(QVector<cv::Point>& region, const cv::Mat & magImg, double angle, double prec, double x, double y)	{
		double lambda, theta, weight;
		double Ixx = 0.0;
		double Iyy = 0.0;
		double Ixy = 0.0;
		int i;

		/* check parameters */
		if (region.size() <= 1) {
			mWarning << "invalid Region";
			return std::numeric_limits<double>::infinity();
		}

		if (prec < 0.0) {
			mWarning << ("get_theta: 'prec' must be positive.");
			return std::numeric_limits<double>::infinity();
		}

		/* compute inertia matrix */
		for (i = 0; i<region.size(); i++) {
			weight = magImg.at<double>((int)y, (int)x);
			Ixx += ((double)region[i].y - y) * ((double)region[i].y - y) * weight;
			Iyy += ((double)region[i].x - x) * ((double)region[i].x - x) * weight;
			Ixy -= ((double)region[i].x - x) * ((double)region[i].y - y) * weight;
		}
		if (doubleEqual(Ixx, 0.0) && doubleEqual(Iyy, 0.0) && doubleEqual(Ixy, 0.0)) {
			mWarning << "get_theta: null inertia matrix.";
			return std::numeric_limits<double>::infinity();
		}

		/* compute smallest eigenvalue */
		lambda = 0.5 * (Ixx + Iyy - sqrt((Ixx - Iyy)*(Ixx - Iyy) + 4.0*Ixy*Ixy));

		/* compute angle */
		theta = fabs(Ixx)>fabs(Iyy) ? atan2(lambda - Ixx, Ixy) : atan2(Ixy, lambda - Iyy);

		/* The previous procedure doesn't cares about orientation,
		so it could be wrong by 180 degrees. Here is corrected if necessary. */
		if (rdf::Algorithms::absAngleDiff(theta, angle) > prec) theta += M_PI;

		return theta;
	}

	/** Compare doubles by relative error.

	The resulting rounding error after floating point computations
	depend on the specific operations done. The same number computed by
	different algorithms could present different rounding errors. For a
	useful comparison, an estimation of the relative rounding error
	should be considered and compared to a factor times EPS. The factor
	should be related to the cumulated rounding error in the chain of
	computation. Here, as a simplification, a fixed factor is used.
	*/
	bool ReadLSD::doubleEqual(double a, double b) {

		double abs_diff, aa, bb, abs_max;
		double relativeErrorFactor = 100;

		/* trivial case */
		if (a == b) return true;

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
		return (abs_diff / abs_max) <= (relativeErrorFactor * DBL_EPSILON);
	}


	QSharedPointer<ReadLSDConfig> ReadLSD::config() const	{
		return qSharedPointerDynamicCast<ReadLSDConfig>(mConfig);
	}

	QString ReadLSD::toString() const {
		return QString();
	}

	bool ReadLSD::checkInput() const {

		if (mSrcImg.empty()) return false;

		return true;
	}


	/** Computes -log10(NFA).

	NFA stands for Number of False Alarms:
	@f[
	\mathrm{NFA} = NT \cdot B(n,k,p)
	@f]

	- NT       - number of tests
	- B(n,k,p) - tail of binomial distribution with parameters n,k and p:
	@f[
	B(n,k,p) = \sum_{j=k}^n
	\left(\begin{array}{c}n\\j\end{array}\right)
	p^{j} (1-p)^{n-j}
	@f]

	The value -log10(NFA) is equivalent but more intuitive than NFA:
	- -1 corresponds to 10 mean false alarms
	-  0 corresponds to 1 mean false alarm
	-  1 corresponds to 0.1 mean false alarms
	-  2 corresponds to 0.01 mean false alarms
	-  ...

	Used this way, the bigger the value, better the detection,
	and a logarithmic scale is used.

	@param n,k,p binomial parameters.
	@param logNT logarithm of Number of Tests

	The computation is based in the gamma function by the following
	relation:
	@f[
	\left(\begin{array}{c}n\\k\end{array}\right)
	= \frac{ \Gamma(n+1) }{ \Gamma(k+1) \cdot \Gamma(n-k+1) }.
	@f]
	We use efficient algorithms to compute the logarithm of
	the gamma function.

	To make the computation faster, not all the sum is computed, part
	of the terms are neglected based on a bound to the error obtained
	(an error of 10% in the result is accepted).
	*/
	double ReadLSD::nfa(int n, int k, double p, double logNT) {
		
		double tolerance = 0.1;       /* an error of 10% in the result is accepted */
		double log1term, term, bin_term, mult_term, bin_tail, err, p_term;
		int i;

		/* check parameters */
		if (n < 0 || k<0 || k>n || p <= 0.0 || p >= 1.0) {
			mWarning << "nfa: wrong n, k or p values";
		}

		/* trivial cases */
		if (n == 0 || k == 0) return -logNT;
		if (n == k) return -logNT - (double)n * log10(p);

		/* probability term */
		p_term = p / (1.0 - p);

		/* compute the first term of the series */
		/*
		binomial_tail(n,k,p) = sum_{i=k}^n bincoef(n,i) * p^i * (1-p)^{n-i}
		where bincoef(n,i) are the binomial coefficients.
		But
		bincoef(n,k) = gamma(n+1) / ( gamma(k+1) * gamma(n-k+1) ).
		We use this to compute the first term. Actually the log of it.
		*/
		log1term = Algorithms::logGamma((double)n + 1.0) - Algorithms::logGamma((double)k + 1.0)
			- Algorithms::logGamma((double)(n - k) + 1.0)
			+ (double)k * log(p) + (double)(n - k) * log(1.0 - p);
		term = exp(log1term);

		/* in some cases no more computations are needed */
		if (Algorithms::doubleEqual(term, 0.0))              /* the first term is almost zero */
		{
			if ((double)k > (double)n * p)     /* at begin or end of the tail?  */
				return -log1term / M_LN10 - logNT;  /* end: use just the first term  */
			else
				return -logNT;                      /* begin: the tail is roughly 1  */
		}

		/* compute more terms if needed */
		bin_tail = term;
		for (i = k + 1; i <= n; i++)
		{
			/*
			As
			term_i = bincoef(n,i) * p^i * (1-p)^(n-i)
			and
			bincoef(n,i)/bincoef(n,i-1) = n-1+1 / i,
			then,
			term_i / term_i-1 = (n-i+1)/i * p/(1-p)
			and
			term_i = term_i-1 * (n-i+1)/i * p/(1-p).
			1/i is stored in a table as they are computed,
			because divisions are expensive.
			p/(1-p) is computed only once and stored in 'p_term'.
			*/
			bin_term = (double)(n - i + 1) * (i<mTabSize ?
				(mInv[i] != 0.0 ? mInv[i] : (mInv[i] = 1.0 / (double)i)) :
				1.0 / (double)i);

			mult_term = bin_term * p_term;
			term *= mult_term;
			bin_tail += term;
			if (bin_term<1.0)
			{
				/* When bin_term<1 then mult_term_j<mult_term_i for j>i.
				Then, the error on the binomial tail when truncated at
				the i term can be bounded by a geometric series of form
				term_i * sum mult_term_i^j.                            */
				err = term * ((1.0 - pow(mult_term, (double)(n - i + 1))) /
					(1.0 - mult_term) - 1.0);

				/* One wants an error at most of tolerance*final_result, or:
				tolerance * abs(-log10(bin_tail)-logNT).
				Now, the error that can be accepted on bin_tail is
				given by tolerance*final_result divided by the derivative
				of -log10(x) when x=bin_tail. that is:
				tolerance * abs(-log10(bin_tail)-logNT) / (1/bin_tail)
				Finally, we truncate the tail if the error is less than:
				tolerance * abs(-log10(bin_tail)-logNT) * bin_tail        */
				if (err < tolerance * fabs(-log10(bin_tail) - logNT) * bin_tail) break;
			}
		}
		return -log10(bin_tail) - logNT;
	}

	bool ReadLSD::isAligned(double thetaTest, double theta, double prec) {

		/* pixels whose level-line angle is not defined
		are considered as NON-aligned */
		//if (thetaTest == -1024) return 0;  /* there is no need to call the function
		//										 'double_equal' here because there is
		//										 no risk of problems related to the
		//										 comparison doubles, we are only
		//										 interested in the exact NOTDEF value */

												 /* it is assumed that 'theta' and 'a' are in the range [-pi,pi] */
		theta -= thetaTest;
		if (theta < 0.0) theta = -theta;
		if (theta > 3.0/2.0*CV_PI)
		{
			theta -= 2*CV_PI;
			if (theta < 0.0) theta = -theta;
		}
		return theta <= prec;
	}

	//bool ReadLSD::isAligned(int x, int y, const cv::Mat &img, double theta) {
	//	double a;

	//	/* check parameters */
	//	if (img.empty())
	//		mWarning << "isaligned: invalid image 'angles'.";

	//	if (x < 0 || y < 0 || x >= (int)img.cols || y >= (int)img.rows)
	//		mWarning << "isaligned: (x,y) out of the image.";
	//	if (prec < 0.0) mWarning << "isaligned: 'prec' must be positive.";

	//	/* angle at pixel (x,y) */
	//	a = img.at<double>(y, x);
	//	//a = angles->data[x + y * angles->xsize];

	//	return isAligned(a, theta);
	//}


	ReadLSDConfig::ReadLSDConfig()	{
		mModuleName = "ReadLSD";
	}

	double ReadLSDConfig::scale() const {
		return mScale;
	}

	void ReadLSDConfig::setScale(double s) {
		mScale = s;
	}

	double ReadLSDConfig::sigmaScale() const {
		return mSigmaScale;
	}

	void ReadLSDConfig::setSigmaScale(double s) {
		mSigmaScale = s;
	}

	double ReadLSDConfig::angleThr() const {
		return mAngleThr;
	}

	void ReadLSDConfig::setAngleThr(double a) {
		mAngleThr = a;
	}

	double ReadLSDConfig::logEps() const {
		return mLogEps;
	}

	void ReadLSDConfig::setLogeps(double l) {
		mLogEps = l;
	}

	double ReadLSDConfig::density() const {
		return mDensityThr;
	}

	void ReadLSDConfig::setDensity(double d) {
		mDensityThr = d;
	}

	int ReadLSDConfig::bins() const	{
		return mNBins;
	}

	void ReadLSDConfig::setBins(int b)	{
		mNBins = b;
	}

	double ReadLSDConfig::quant() const	{
		return mQuant;
	}

	void ReadLSDConfig::setQuant(double q)	{
		mQuant = q;
	}

	QString ReadLSDConfig::toString() const	{

		QString msg;
		msg += "  scale: " + QString::number(mScale);
		msg += "  sigmaScale: " + QString::number(mSigmaScale);
		msg += "  angleThr: " + QString::number(mAngleThr);
		msg += "  logEps: " + QString::number(mLogEps);
		msg += "  densityThr: " + QString::number(mDensityThr);
		msg += "  nBins: " + QString::number(mNBins);
		msg += "  quant: " + QString::number(mQuant);

		return msg;
	}

	void ReadLSDConfig::load(const QSettings & settings) {
		mScale = settings.value("scale", mScale).toDouble();
		mSigmaScale = settings.value("sigmaScale", mSigmaScale).toDouble();
		mAngleThr = settings.value("angleThr", mAngleThr).toDouble();
		mLogEps = settings.value("logEps", mLogEps).toDouble();
		mDensityThr = settings.value("densityThr", mDensityThr).toDouble();
		mNBins = settings.value("nBins", mNBins).toInt();
		mQuant = settings.value("quant", mQuant).toDouble();

	}

	void ReadLSDConfig::save(QSettings & settings) const {
		settings.setValue("scale", mScale);
		settings.setValue("sigmaScale", mSigmaScale);
		settings.setValue("angleThr", mAngleThr);
		settings.setValue("logEps", mLogEps);
		settings.setValue("densityThr", mDensityThr);
		settings.setValue("nBins", mNBins);
		settings.setValue("quant", mQuant);
	}

}