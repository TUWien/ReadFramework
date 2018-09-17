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

#include "Blobs.h"
#include "Algorithms.h"

#pragma warning(push, 0)	// no warnings from includes
// Qt Includes
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
//#include "opencv/highgui.h"
#include <QSharedPointer>
#include <QDebug>
#pragma warning(pop)

namespace rdf {


/// <summary>
/// Initializes a new instance of the <see cref="Blob"/> class.
/// </summary>
Blob::Blob() {

}

/// <summary>
/// Initializes a new instance of the <see cref="Blob"/> class.
/// </summary>
/// <param name="outerC">The outer contour of the blob.</param>
/// <param name="innerC">The inner contours of the blob (can be more than one).</param>
Blob::Blob(const QVector<cv::Point>& outerC, const QVector<QVector<cv::Point> >& innerC) {
	mOuterContour = outerC;
	mInnerContours = innerC;
}

/// <summary>
/// Determines whether this instance is empty.
/// </summary>
/// <returns>True if emtpy.</returns>
bool Blob::isEmpty() const {

	return (mOuterContour.size() == 0);
}

/// <summary>
/// Sets the blob.
/// </summary>
/// <param name="outerC">The outer contour.</param>
/// <param name="innerC">The inner contours.</param>
void Blob::setBlob(const QVector<cv::Point>& outerC, const QVector<QVector<cv::Point> >& innerC) {
	mOuterContour = outerC;
	mInnerContours = innerC;
}

/// <summary>
/// Returns the outer contour.
/// </summary>
/// <returns>The outer contour.</returns>
QVector<cv::Point> Blob::outerContour() const {
	return mOuterContour;
}

/// <summary>
/// Returns the inner contours.
/// </summary>
/// <returns>The inner contours.</returns>
QVector<QVector<cv::Point> > Blob::innerContours() const {
	return mInnerContours;
}

/// <summary>
/// Calculates the hierarchy for the blob such that it can be used with openCV functions.
/// </summary>
/// <returns>The OpenCV hierarchy.</returns>
QVector<cv::Vec4i> Blob::hierarchy() const {
	QVector<cv::Vec4i> tmp;

	if (mInnerContours.isEmpty()) {
		tmp.append(cv::Vec4i(-1, -1, -1, -1));
	}
	else {
		tmp.append(cv::Vec4i(-1, -1, 1, -1));
		for (int i = 1; i < mInnerContours.size(); i++) {
			tmp.append(cv::Vec4i(i + 1, i==1 ? -1 : i-1, -1, 0));
		}
		int prevIdx = mInnerContours.size() == 1 ? -1 : mInnerContours.size() - 1;
		tmp.append(cv::Vec4i(-1, prevIdx, -1, 0));
	}
	//parent:	   -1,-1, 1,-1
	//first child:	2,-1,-1,0
	//second child: 3, 1,-1,0
	//third child:	4, 2,-1,0
	//fourth child:-1, 3,-1,0

	return tmp;
}

/// <summary>
/// The blobs orientation based on moments.
/// </summary>
/// <returns>The orientation of the blob in rad.</returns>
float Blob::blobOrientation() const {

	double u00, u11, u20, u02, num, den;
	float o = 0.0f;
	cv::Moments m;

	if (!mOuterContour.isEmpty()) {
		m = moments(cv::Mat(mOuterContour.toStdVector()));
		u00 = m.m00;

		if (m.m00 <= 0)
			o = 0;
		else {

			//u10 = m.m10 / u00;
			//u01 = m.m01 / u00;

			u11 = -(m.m11 - m.m10 * m.m01 / u00) / u00;
			u20 = (m.m20 - m.m10 * m.m10 / u00) / u00;
			u02 = (m.m02 - m.m01 * m.m01 / u00) / u00;

			num = 2 * u11;
			den = u20 - u02;// + sqrt((u20 - u02)*(u20 - u02) + 4*u11*u11);

			if (num != 0 && den != 00)
			{
				//o = (float)(180.0 + (180.0 / CV_PI) * atan( num / den ));
				o = 0.5f*(float)(atan(num / den));

				if (den < 0) {
					o += num > 0 ? (float)CV_PI / 2.0f : (float)-CV_PI / 2.0f;;
				}
			}
			else if (den == 0 && num > 0)
				o = (float)CV_PI / 4.0f;
			else if (den == 0 && num < 0)
				o = (float)-CV_PI / 4.0f;
			//covered with else
			//else if (num == 0 && den > 0)
			//	o = 0;
			else if (num == 0 && den < 0)
				o = (float)-CV_PI / 2.0f;
			else
				o = 0.0f;
		}
	}

	return o;
}

/// <summary>
/// Draws the Blob.
/// </summary>
/// <param name="imgSrc">The img source where the blob is drawn.</param>
/// <param name="color">The color of the blob.</param>
/// <returns>True if the blob is drawn.</returns>
bool Blob::drawBlob(cv::Mat& imgSrc, cv::Scalar color, int maxLevel) const {

	if (isEmpty()) return false;
	//if (mHierarchy.isEmpty()) mHierarchy = hierarchy();
	QVector<cv::Vec4i> h = hierarchy();

	//for (auto i : h) {
	//	qDebug() << i[0] << " " << i[1] << " " << i[2] << " " << i[3];
	//}

	//QVector<QVector<cv::Point> > contours;
	std::vector<std::vector<cv::Point> > contours;
	contours.push_back(mOuterContour.toStdVector());
	for (auto ic : mInnerContours) {
		contours.push_back(ic.toStdVector());
	}

	//contours.append(mInnerContours);

	//qDebug() << "contourSize: " << contours.size();
	//qDebug() << "contourSize 0: " << contours[0].size();
	//qDebug() << "OutercontourSize: " << mOuterContour.size();
	//qDebug() << "InnercontourSize: " << mInnerContours.size();
	//qDebug() << "image size: " << imgSrc.size().width << " " << imgSrc.size().height;

	cv::drawContours(imgSrc, contours , 0, color, CV_FILLED, 8, h.toStdVector(), maxLevel, cv::Point());

	return true;
}

// ----------------------------- BlobManager ----------------------------------------------------------------------------
/// <summary>
/// Initializes a new instance of the <see cref="Blobs"/> class.
/// </summary>
Blobs::Blobs() {

}

/// <summary>
/// Determines whether this instance is empty (no blobs).
/// </summary>
/// <returns>True if the instance is empty.</returns>
bool Blobs::isEmpty() const {

	return mBlobs.isEmpty();
}

bool Blobs::checkInput() const {

	if (mBwImg.empty() || mBwImg.channels() != 1 || mBwImg.depth() != CV_8U) return false;

	return true;
}

/// <summary>
/// Sets the image.
/// </summary>
/// <param name="bWImg">The binary img CV_8UC1.</param>
/// <returns>True if the image could be set.</returns>
bool Blobs::setImage(const cv::Mat& bWImg) {

	mBwImg = bWImg.clone(); //clone is needed, because findContours changes the image
	mSize = bWImg.size();
	return checkInput();

}

/// <summary>
/// Computes all blobs of the binary image.
/// </summary>
/// <returns>True if the blobs could be computed.</returns>
bool Blobs::compute() {
	if (!checkInput()) return false;

	if (mBlobs.isEmpty()) {

		std::vector<std::vector<cv::Point> > contours;
		std::vector<cv::Vec4i> hierarchy;
		cv::findContours(mBwImg, contours, hierarchy, CV_RETR_CCOMP, mApproxMethod);

		if (!hierarchy.empty()) {

			for (int outerIdx = 0; outerIdx >= 0; outerIdx = hierarchy[outerIdx][0]) {

				QVector<cv::Point> outerContour;
				QVector<QVector<cv::Point> > innerContours;

				//outerContour = outerContour.fromStdVector(contours[outerIdx]);
				outerContour = QVector<cv::Point>::fromStdVector(contours[outerIdx]);

				int firstChild = hierarchy[outerIdx][2];

				if (firstChild != -1) {

					for (int innerIdx = firstChild; innerIdx >= 0; innerIdx = hierarchy[innerIdx][0]) {
						innerContours.append(QVector<cv::Point>::fromStdVector(contours[innerIdx]));
					}
				}

				Blob newBlob(outerContour, innerContours);
				mBlobs.append(newBlob);
			}
		}
		else {
			qInfo() << "no blobs found";
		}
	}

	return true;
}

/// <summary>
/// Deletes the blobs.
/// </summary>
void Blobs::deleteBlobs() {

	mBlobs.clear();
}

/// <summary>
/// Return all blobs.
/// </summary>
/// <returns>The blobs.</returns>
QVector<Blob> Blobs::blobs() const {
	return mBlobs;
}

/// <summary>
/// Sets the blobs.
/// </summary>
/// <param name="blobs">The blobs.</param>
void Blobs::setBlobs(const QVector<Blob>& blobs) {
	mBlobs.clear(); mBlobs = blobs;
}

/// <summary>
/// Returns the size of the image.
/// </summary>
/// <returns>The size of the image.</returns>
cv::Size Blobs::size() const {
	return mSize;
}

// ---------- BlobManager ----------------------------------------------------------------------------------------------


BlobManager::BlobManager() {

}

/// <summary>
/// Creates an instance or return the instance.
/// </summary>
/// <returns>The instance of BlobManager.</returns>
BlobManager& BlobManager::instance() {

	static QSharedPointer<BlobManager> inst;
	if (!inst)
		inst = QSharedPointer<BlobManager>(new BlobManager());
	return *inst;

}

/// <summary>
/// Filters blobs according to a minimal area.
/// Removes all blobs with an area smaller than area.
/// </summary>
/// <param name="area">The area threshold.</param>
/// <param name="blobs">The blobs.</param>
/// <returns>The filtered blob vector</returns>
QVector<Blob> BlobManager::filterArea(int threshArea, const Blobs& blobs) const {

	//QVector<Blob> tmp = blobs.blobs();
	QVector<Blob> filtered;

	for (const Blob& blob : blobs.blobs()) {

		int blobArea = (int) std::fabs(cv::contourArea(blob.outerContour().toStdVector()));

		if (blobArea > threshArea) {
			filtered.append(blob);
		}
	}

	return filtered;
}

/// <summary>
/// Filters blobs according to the size and the aspect ratio.
/// </summary>
/// <param name="maxAspectRatio">The maximum aspect ratio.</param>
/// <param name="minWidth">The minimum width.</param>
/// <param name="blobs">The blobs.</param>
/// <returns>The filtered blob vector.</returns>
QVector<Blob> BlobManager::filterMar(float maxAspectRatio, int minWidth, const Blobs& blobs) const {

	QVector<Blob> filtered;
	//QVector<Blob> blobcopy = blobs.blobs();

	for (const Blob& blob : blobs.blobs()) {

		cv::RotatedRect rotRect = cv::minAreaRect(cv::Mat(blob.outerContour().toStdVector()));

	
		float currWidth = rotRect.size.height > rotRect.size.width ? rotRect.size.height : rotRect.size.width;
		float currRatio = 0.0f;

		if ((rotRect.size.width != 0) && (rotRect.size.height != 0))
			currRatio = rotRect.size.height > rotRect.size.width ? rotRect.size.width / rotRect.size.height : rotRect.size.height / rotRect.size.width;

		//if (currWidth >= minWidth)
		//	qDebug() << "currWidth: " << currWidth << " currRatio " << currRatio;

		if ((currWidth >= minWidth) && (currRatio <= maxAspectRatio))
			filtered.append(blob);

	}

	return filtered;
}


/// <summary>
/// Filters blobs according to their orientation.
/// </summary>
/// <param name="angle">The specified orientation angle in rad.</param>
/// <param name="maxAngleDiff">The maximum angle difference in degree.</param>
/// <param name="blobs">The blobs.</param>
/// <returns>The filtered blob vector</returns>
QVector<Blob> BlobManager::filterAngle(double angle, double maxAngleDiff, const Blobs& blobs) const {

	QVector<Blob> filtered;
	float o;

	for (const Blob& blob : blobs.blobs()) {

		o = blob.blobOrientation();

		double a = Algorithms::normAngleRad(angle, 0.0, CV_PI);
		a = a > CV_PI*0.5 ? CV_PI - a : a;
		double angleNewLine = Algorithms::normAngleRad(o, 0.0, CV_PI);
		angleNewLine = angleNewLine > CV_PI*0.5 ? CV_PI - angleNewLine : angleNewLine;

		double diffangle = std::abs(a - angleNewLine);

		a = a > CV_PI*0.25 ? CV_PI*0.5 - a : a;
		angleNewLine = angleNewLine > CV_PI*0.25 ? CV_PI*0.5 - angleNewLine : angleNewLine;

		diffangle = diffangle < std::abs(a - angleNewLine) ? diffangle : std::abs(a - angleNewLine);

		if (diffangle <= maxAngleDiff / 180.0*CV_PI)
			filtered.append(blob);
	}

	return filtered;
}

/// <summary>
/// Draws the blobs and returns a CV_8UC1 image.
/// </summary>
/// <param name="blobs">The blobs.</param>
/// <param name="color">The color of the blobs.</param>
/// <returns>A CV_8UC1 image containing all blobs.</returns>
cv::Mat BlobManager::drawBlobs(const Blobs& blobs, cv::Scalar color) const {

	cv::Mat newBWImg(blobs.size(), CV_8UC1);
	newBWImg.setTo(0);

	for (const Blob& blob : blobs.blobs()) {

		blob.drawBlob(newBWImg, color);

	}

	return newBWImg;
}

/// <summary>
/// Assumes all blobs as lines and fit a Line based on the minimum area rectangle.
/// </summary>
/// <param name="blobs">The blobs.</param>
/// <returns>The line vector.</returns>
QVector<Line> BlobManager::lines(const Blobs& blobs) const {

	QVector<Line> blobLines;
	float xVec, yVec;
	int p1X, p1Y, p2X, p2Y;
	float orientation, currWidth;
	float thickness;

	for (const Blob& blob : blobs.blobs()) {
		cv::RotatedRect rotRect = cv::minAreaRect(cv::Mat(blob.outerContour().toStdVector()));

		currWidth = rotRect.size.height > rotRect.size.width ? rotRect.size.height : rotRect.size.width;
		thickness = rotRect.size.height < rotRect.size.width ? rotRect.size.height : rotRect.size.width;

		orientation = blob.blobOrientation();

		xVec = -(currWidth*0.5f*(float)cos(orientation));
		yVec = (currWidth*0.5f*(float)sin(orientation));

		p1X = (int) (rotRect.center.x - xVec);
		p1Y = (int) (rotRect.center.y - yVec);
		p2X = (int) (rotRect.center.x + xVec);
		p2Y = (int) (rotRect.center.y + yVec);

		if (thickness > 15) thickness = 15; //UFO bugfix

		QLine tmp = p1X < p2X ? QLine(QPoint(p1X,p1Y), QPoint(p2X,p2Y)) : QLine(QPoint(p2X, p2Y), QPoint(p1X, p1Y));
		Line newLine(tmp, thickness);

		blobLines.append(newLine);
	}

	return blobLines;
}

/// <summary>
/// Gets the biggest BLOB.
/// </summary>
/// <param name="blobs">The blobs.</param>
/// <returns>The biggest blob.</returns>
Blob BlobManager::getBiggestBlob(const Blobs& blobs) const {

	QVector<Blob> filtered;
	Blob biggestBlob;

	int maxArea = 0;

	for (const Blob& blob : blobs.blobs()) {

		int blobArea = (int)std::fabs(cv::contourArea(blob.outerContour().toStdVector()));

		if (blobArea > maxArea) {
			maxArea = blobArea;
			biggestBlob.setBlob(blob.outerContour(), blob.innerContours());
		}
	}

	return biggestBlob;
}


}