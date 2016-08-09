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
#include <QSharedPointer>
#include <QImage>
#include <QString>
#include <QColor>
#include <QPen>

#include "opencv2/core/core.hpp"
#pragma warning(pop)

#pragma warning (disable: 4251)	// inlined Qt functions in dll interface

#ifndef DllCoreExport
#ifdef DLL_CORE_EXPORT
#define DllCoreExport Q_DECL_EXPORT
#else
#define DllCoreExport Q_DECL_IMPORT
#endif
#endif

// Qt defines
class QPixmap;

namespace rdf {

class Rect;

class DllCoreExport Histogram {

public:
	Histogram(const cv::Mat& values = cv::Mat());
	Histogram(const QVector<int>& values);

	cv::Mat draw(const QPen& pen = QPen(), const QColor& bgCol = QColor(255, 255, 255));
	void draw(QPainter& p, const Rect& r);

	cv::Mat hist() const;
	double max() const;

protected:
	void draw(QPainter& p);

	cv::Mat mHist;
};

// read defines
/// <summary>
/// Basic image class
/// </summary>
class DllCoreExport Image {

public:
	static Image& instance();

	cv::Mat qImage2Mat(const QImage& img);
	QImage mat2QImage(const cv::Mat& img);
	cv::Mat qPixmap2Mat(const QPixmap& img);
	QPixmap mat2QPixmap(const cv::Mat& img);
	cv::Mat qVector2Mat(const QVector<float>& data);

	bool save(const QImage& img, const QString& savePath, int compression = -1) const;
	bool save(const cv::Mat& img, const QString& savePath, int compression = -1) const;
	bool alphaChannelUsed(const QImage& img) const;
	void imageInfo(const cv::Mat& img, const QString name) const;
	QString printImage(const cv::Mat& img, const QString name) const;

	/// <summary>
	/// Prints the values of a cv::Mat to copy it to Matlab.
	/// </summary>
	/// <param name="src">The Mat to be printed.</param>
	/// <param name="varName">Name of the variable for matlab.</param>
	/// <returns>The String with all values formatted for matlab.</returns>
	template <typename numFmt>
	static QString printMat(const cv::Mat& src, const QString varName) {

		QString msg = varName;
		msg.append(" = [");	// matlab...

		int cnt = 0;

		for (int rIdx = 0; rIdx < src.rows; rIdx++) {

			const numFmt* srcPtr = src.ptr<numFmt>(rIdx);

			for (int cIdx = 0; cIdx < src.cols; cIdx++, cnt++) {


				msg.append(QString::number(srcPtr[cIdx]));
				msg.append( (cIdx < src.cols - 1) ? " " : "; " ); // next row matlab?

				if (cnt % 7 == 0)
					msg.append("...\n");
			}

		}
		msg.append("];");

		return msg;
	}

private:
	Image();
	Image(const Image&);
};

};