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

#pragma once

#pragma warning(push, 0)	// no warnings from includes
// Qt Includes
#include <QPolygon>
#include <QVector>
#include "opencv2/core/core.hpp"
#pragma warning(pop)

#ifndef DllCoreExport
#ifdef DLL_CORE_EXPORT
#define DllCoreExport Q_DECL_EXPORT
#else
#define DllCoreExport Q_DECL_IMPORT
#endif
#endif

#pragma warning(disable: 4251)
// Qt defines

namespace rdf {

class DllCoreExport Blob {

public:
	Blob(const QVector<cv::Point>& outerC, const QVector<QVector<cv::Point> >& innerC);

	bool isEmpty() const;

	//void read(const QString& pointList);
	//QString write() const;

	//int size() const;

	void setBlob(const QVector<cv::Point>& outerC, const QVector<QVector<cv::Point> >& innerC);
	QVector<cv::Point> outerContour() const;
	QVector<QVector<cv::Point> > innerContours() const;
	QVector<cv::Vec4i> hierarchy() const;
	bool drawBlob(cv::Mat imgSrc, cv::Scalar color = cv::Scalar(255, 25, 255));

protected:

	QVector<cv::Point> mOuterContour;
	QVector<QVector<cv::Point> > mInnerContours;
	
private:
	QVector<cv::Vec4i> mHierarchy;
};

};