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

#include "Image.h"

#pragma warning(push, 0)	// no warnings from includes
#include <QDebug>
#pragma warning(pop)

namespace rdf {

// Image --------------------------------------------------------------------
Image::Image() {
}

Image& Image::instance() { 

	static QSharedPointer<Image> inst;
	if (!inst)
		inst = QSharedPointer<Image>(new Image());
	return *inst; 
}

/**
* Converts a QImage to a Mat
* @param img formats supported: ARGB32 | RGB32 | RGB888 | Indexed8
* @return cv::Mat the corresponding Mat
**/ 
cv::Mat Image::qImage2Mat(const QImage& img) {

	cv::Mat mat2;
	QImage cImg;	// must be initialized here!	(otherwise the data is lost before clone())

	try {
		if (img.format() == QImage::Format_RGB32)
			qDebug() << "we have an RGB32 in memory...";

		if (img.format() == QImage::Format_ARGB32 || img.format() == QImage::Format_RGB32) {
			mat2 = cv::Mat(img.height(), img.width(), CV_8UC4, (uchar*)img.bits(), img.bytesPerLine());
			//qDebug() << "ARGB32 or RGB32";
		}
		else if (img.format() == QImage::Format_RGB888) {
			mat2 = cv::Mat(img.height(), img.width(), CV_8UC3, (uchar*)img.bits(), img.bytesPerLine());
			//qDebug() << "RGB888";
		}
		//// converting to indexed8 causes bugs in the qpainter
		//// see: http://qt-project.org/doc/qt-4.8/qimage.html
		//else if (img.format() == QImage::Format_Indexed8) {
		//	mat2 = Mat(img.height(), img.width(), CV_8UC1, (uchar*)img.bits(), img.bytesPerLine());
		//	//qDebug() << "indexed...";
		//}
		else {
			//qDebug() << "image flag: " << img.format();
			cImg = img.convertToFormat(QImage::Format_ARGB32);
			mat2 = cv::Mat(cImg.height(), cImg.width(), CV_8UC4, (uchar*)cImg.bits(), cImg.bytesPerLine());
			//qDebug() << "I need to convert the QImage to ARGB32";
		}

		mat2 = mat2.clone();	// we need to own the pointer
	}
	catch (...) {	// something went seriously wrong (e.g. out of memory)
					//DkNoMacs::dialog(QObject::tr("Sorry, could not convert image."));
		qDebug() << "[DkImage::qImage2Mat] could not convert image - something is seriously wrong down here...";

	}

	return mat2; 
}

/**
* Converts a cv::Mat to a QImage.
* @param img supported formats CV8UC1 | CV_8UC3 | CV_8UC4
* @return QImage the corresponding QImage
**/ 
QImage Image::mat2QImage(const cv::Mat& img) {

	QImage qImg;
	cv::Mat cvImg = img;

	// since Mat header is copied, a new buffer should be allocated (check this!)
	if (cvImg.depth() == CV_32F)
		cvImg.convertTo(cvImg, CV_8U, 255);

	if (cvImg.type() == CV_8UC1) {
		qImg = QImage(cvImg.data, (int)cvImg.cols, (int)cvImg.rows, (int)cvImg.step, QImage::Format_Indexed8);	// opencv uses size_t for scaling in x64 applications
	}
	if (cvImg.type() == CV_8UC3) {
		qImg = QImage(cvImg.data, (int)cvImg.cols, (int)cvImg.rows, (int)cvImg.step, QImage::Format_RGB888);
	}
	if (cvImg.type() == CV_8UC4) {
		qImg = QImage(cvImg.data, (int)cvImg.cols, (int)cvImg.rows, (int)cvImg.step, QImage::Format_ARGB32);
	}


	qImg = qImg.copy();

	return qImg;
}



}