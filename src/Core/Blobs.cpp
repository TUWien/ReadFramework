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

#include "Blobs.h"
#include "Algorithms.h"

#pragma warning(push, 0)	// no warnings from includes
// Qt Includes
#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv/highgui.h"
#include <QSharedPointer>
#include <QDebug>
#pragma warning(pop)

namespace rdf {

Blob::Blob(const QVector<cv::Point>& outerC, const QVector<QVector<cv::Point> >& innerC) {
	mOuterContour = outerC;
	mInnerContours = innerC;
}

bool Blob::isEmpty() const {

	return (mOuterContour.size() > 0);
}

void Blob::setBlob(const QVector<cv::Point>& outerC, const QVector<QVector<cv::Point> >& innerC) {
	mOuterContour = outerC;
	mInnerContours = innerC;
}

QVector<cv::Point> Blob::outerContour() const {
	return mOuterContour;
}

QVector<QVector<cv::Point> > Blob::innerContours() const {
	return mInnerContours;
}

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

float Blob::blobOrientation() const {

	double u00, u11, u01, u10, u20, u02, num, den;
	float o = 0.0f;
	cv::Moments m;

	if (!mOuterContour.isEmpty()) {
		m = moments(cv::Mat(mOuterContour.toStdVector()));
		u00 = m.m00;

		if (m.m00 <= 0)
			o = 0;
		else {

			u10 = m.m10 / u00;
			u01 = m.m01 / u00;

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

bool Blob::drawBlob(cv::Mat imgSrc, cv::Scalar color) const {

	if (isEmpty()) return false;
	//if (mHierarchy.isEmpty()) mHierarchy = hierarchy();
	QVector<cv::Vec4i> h = hierarchy();

	QVector<QVector<cv::Point> > contours;
	contours.append(mOuterContour);
	contours.append(mInnerContours);

	cv::drawContours(imgSrc, contours.toStdVector() , 0, color, CV_FILLED, 8, h.toStdVector(), 0, cv::Point());

	return true;
}

// ----------------------------- BlobManager ----------------------------------------------------------------------------

Blobs::Blobs() {

}

bool Blobs::isEmpty() const {

	return mBlobs.isEmpty();
}

bool Blobs::checkInput() const {

	if (mBwImg.empty() || mBwImg.channels() != 1 || mBwImg.depth() != CV_8U) return false;

	return true;
}

bool Blobs::setImage(const cv::Mat& bWImg) {

	mBwImg = bWImg;
	mSize = bWImg.size();
	return checkInput();

}

bool Blobs::compute() {
	if (!checkInput()) return false;

	if (mBlobs.isEmpty()) {

		std::vector<std::vector<cv::Point> > contours;
		std::vector<cv::Vec4i> hierarchy;
		cv::findContours(mBwImg, contours, hierarchy, CV_RETR_CCOMP, mApproxMethod);


		for (int outerIdx = 0; outerIdx >= 0; outerIdx = hierarchy[outerIdx][0]) {

			QVector<cv::Point> outerContour;
			QVector<QVector<cv::Point> > innerContours;

			outerContour.fromStdVector(contours[outerIdx]);
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

	return true;
}

void Blobs::deleteBlobs() {

	mBlobs.clear();
}

// ---------- BlobManager ----------------------------------------------------------------------------------------------


BlobManager::BlobManager() {

}

BlobManager& BlobManager::instance() {

	static QSharedPointer<BlobManager> inst;
	if (!inst)
		inst = QSharedPointer<BlobManager>(new BlobManager());
	return *inst;

}

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

QVector<Blob> BlobManager::filterMar(int maxAspectRatio, int minWidth, const Blobs& blobs) const {

	QVector<Blob> filtered;
	//QVector<Blob> blobcopy = blobs.blobs();

	for (const Blob& blob : blobs.blobs()) {

		cv::RotatedRect rotRect = cv::minAreaRect(cv::Mat(blob.outerContour().toStdVector()));

	
		float currWidth = rotRect.size.height > rotRect.size.width ? rotRect.size.height : rotRect.size.width;
		float currRatio = 0;

		if ((rotRect.size.width != 0) && (rotRect.size.height != 0))
			currRatio = rotRect.size.height > rotRect.size.width ? rotRect.size.width / rotRect.size.height : rotRect.size.height / rotRect.size.width;

		if ((currWidth >= minWidth) && (currRatio <= maxAspectRatio))
			filtered.append(blob);

	}

	return filtered;
}

QVector<Blob> BlobManager::filterAngle(float angle, float maxAngleDiff, const Blobs& blobs) const {

	QVector<Blob> filtered;
	float o;

	for (const Blob& blob : blobs.blobs()) {

		o = blob.blobOrientation();

		float a = Algorithms::instance().normAngleRad((float)angle, 0.0f, (float)CV_PI);
		a = a >(float)CV_PI*0.5f ? (float)CV_PI - a : a;
		float angleNewLine = Algorithms::instance().normAngleRad(o, 0.0f, (float)CV_PI);
		angleNewLine = angleNewLine > (float)CV_PI*0.5f ? (float)CV_PI - angleNewLine : angleNewLine;

		float diffangle = fabs(a - (float)angleNewLine);

		a = a > (float)CV_PI*0.25f ? (float)CV_PI*0.5f - a : a;
		angleNewLine = angleNewLine > (float)CV_PI*0.25f ? (float)CV_PI*0.5f - angleNewLine : angleNewLine;

		diffangle = diffangle < fabs(a - (float)angleNewLine) ? diffangle : fabs(a - (float)angleNewLine);

		if (diffangle > maxAngleDiff / 180.0f*(float)CV_PI)
			filtered.append(blob);
	}

	return filtered;
}

cv::Mat BlobManager::drawBlobs(const Blobs& blobs, cv::Scalar color) const {

	cv::Mat newBWImg(blobs.size(), CV_8UC1);

	for (const Blob& blob : blobs.blobs()) {

		blob.drawBlob(newBWImg, color);

	}

	return newBWImg;
}

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

		QLine tmp(QPoint(p1X,p1Y), QPoint(p2X,p2Y));
		Line newLine(tmp, thickness);

		blobLines.append(newLine);
	}

	return blobLines;
}


}