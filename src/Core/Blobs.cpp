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

#pragma warning(push, 0)	// no warnings from includes
// Qt Includes
#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv/highgui.h"
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
	//parent:	   -1,-1, 1,0
	//first child:	2,-1,-1,0
	//second child: 3, 1,-1,0
	//third child:	4, 2,-1,0
	//fourth child:-1, 3,-1,0

	return tmp;
}

bool Blob::drawBlob(cv::Mat imgSrc, cv::Scalar color) {

	if (isEmpty()) return false;
	if (mHierarchy.isEmpty()) mHierarchy = hierarchy();

	cv::drawContours(imgSrc, mInnerContours.toStdVector() , 0, color, CV_FILLED, 8, mHierarchy.toStdVector, 0, cv::Point());

	return true;
}

}