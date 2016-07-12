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

		//Image::instance().save(hDSCCImg, "D:\\tmp\\hdscc.tif");
		//Image::instance().save(vDSCCImg, "D:\\tmp\\vdscc.tif");
		//return true;

		mLineImg = cv::Mat(mSrcImg.rows, mSrcImg.cols, CV_8UC1);
		cv::bitwise_or(hDSCCImg, vDSCCImg, mLineImg);

		//Image::instance().save(mLineImg, "D:\\tmp\\mLineImg.tif");

		//if (mDAngle != 361.0f) {
		if (!isinf(mAngle)) {
			QVector<rdf::Line> tmp;
			tmp.append(hLines);
			tmp.append(vLines);

			hLines = filterLineAngle(tmp, (float)mAngle, config()->maxSlopeRotat());
			vLines = filterLineAngle(tmp, (float)(mAngle+CV_PI*0.5f), config()->maxSlopeRotat());
		}

		//Image::instance().save(hDSCCImg, "D:\\tmp\\hdscc.tif");
		//Image::instance().save(vDSCCImg, "D:\\tmp\\vdscc.tif");

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

			cv::Point pStart(l.startPoint().x(), l.startPoint().y());
			cv::Point pEnd(l.endPoint().x(), l.endPoint().y());

			cv::line(synLinImg, pStart, pEnd, cv::Scalar(255), (int)l.thickness(), 8, 0);
		}

		for (auto l : vLines) {

			cv::Point pStart(l.startPoint().x(), l.startPoint().y());
			cv::Point pEnd(l.endPoint().x(), l.endPoint().y());

			cv::line(synLinImg, pStart, pEnd, cv::Scalar(255), (int)l.thickness(), 8, 0);
		}

		return synLinImg;
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
				float dist = cLine.minDistance(cmpLine);

				if (dist > config()->maxGap()) continue;

				float diffAngle = cLine.diffAngle(cmpLine);
				if (diffAngle > qDegreesToRadians(config()->maxSlopeDiff())) continue;
				
				//orientation of new line differ in orientation with connecting lines by maxSlopeDiff degree
				//difference in orientation between gapline and cline/cmpline. the larger deviation is taken
				rdf::Line newLine, gapLine;
				newLine = cLine.merge(cmpLine);

				float angle1 = (float)newLine.diffAngle(cLine);
				float angle2 = (float)newLine.diffAngle(cmpLine);
				float angle = angle1 > angle2 ? angle1 : angle2;

				float weight = 1.0f - (dist / config()->maxGap());
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

			cv::Point pStart(l.startPoint().x(), l.startPoint().y());
			cv::Point pEnd(l.endPoint().x(), l.endPoint().y());

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
			
			float a = Algorithms::instance().normAngleRad((float)angle, 0.0f, (float)CV_PI);

			float angleNewLine = Algorithms::instance().normAngleRad((float)l.angle(), 0.0f, (float)CV_PI);

			float diffangle = (float)cv::min(fabs(Algorithms::instance().normAngleRad(a, 0, (float)CV_PI) - Algorithms::instance().normAngleRad(angleNewLine, 0, (float)CV_PI))
				, (float)CV_PI - fabs(Algorithms::instance().normAngleRad(a, 0, (float)CV_PI) - Algorithms::instance().normAngleRad(angleNewLine, 0, (float)CV_PI)));


			if (diffangle < angleDiff / 180.0f*(float)CV_PI)
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
		if (!isinf(mAngle)) 
			binBlobsH.setBlobs(rdf::BlobManager::instance().filterAngle((float)mAngle, config()->maxSlopeRotat(), binBlobsH));

		hLines = rdf::BlobManager::instance().lines(binBlobsH);
		hDSCCImg = rdf::BlobManager::instance().drawBlobs(binBlobsH);


		rdf::Blobs binBlobsV;
		binBlobsV.setImage(vDSCCImg);
		binBlobsV.compute();

		binBlobsV.setBlobs(rdf::BlobManager::instance().filterMar(config()->maxAspectRatio(), config()->minWidth(), binBlobsV));
		if (!isinf(mAngle))
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
		mModuleName = "LineTraceConfig";
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

}