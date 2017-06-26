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
#include <QDebug>

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
	Histogram(double minVal, double maxVal, int numBins = 256);

	cv::Mat draw(const QPen& pen = QPen(), const QColor& bgCol = QColor(255, 255, 255));
	void draw(QPainter& p, const Rect& r) const;

	bool isEmpty() const;

	cv::Mat hist() const;
	double maxBin() const;
	double minBin() const;
	int numBins() const;
	int binIdx(double val) const;
	double value(int binIdx) const;

	template <typename Num>
	static Histogram fromData(const cv::Mat& data, int numBins = 256) {

		double minV = 0, maxV = 0;
		cv::minMaxLoc(data, &minV, &maxV);

		if (minV == maxV) {
			qWarning() << "min == max that's not good for creating a histogram - aborting";
			return Histogram();
		}

		Histogram h(minV, maxV, numBins);
		float* hmp = h.hist().ptr<float>();
		
		for (int rIdx = 0; rIdx < data.rows; rIdx++) {

			const Num* ptr = data.ptr<Num>(rIdx);

			for (int cIdx = 0; cIdx < data.cols; cIdx++) {

				// this is inherently dangerous & fast
				// it relies on the fact that binIdx is always within the range
				int bin = h.binIdx(ptr[cIdx]);
				hmp[bin]++;
			}
		}

		return h;
	}

protected:
	void draw(QPainter& p) const;
	double transformX(double val, const Rect& r) const;
	double transformY(double val, double minV, double maxV, const Rect& r) const;
	
	cv::Mat mHist;
	double mMinVal = 0;
	double mMaxVal = 0; 
};

// read defines
/// <summary>
/// Basic image class
/// </summary>
class DllCoreExport Image {

public:
	static cv::Mat qImage2Mat(const QImage& img);
	static QImage mat2QImage(const cv::Mat& img);
	static cv::Mat qPixmap2Mat(const QPixmap& img);
	static QPixmap mat2QPixmap(const cv::Mat& img);
	static cv::Mat qVector2Mat(const QVector<float>& data);

	static bool save(const QImage& img, const QString& savePath, int compression = -1);
	static bool save(const cv::Mat& img, const QString& savePath, int compression = -1);
	static bool alphaChannelUsed(const QImage& img);
	static void imageInfo(const cv::Mat& img, const QString name);
	static QString printImage(const cv::Mat& img, const QString name);
	static QJsonObject matToJson(const cv::Mat& img, bool compress = true);
	static cv::Mat jsonToMat(const QJsonObject& jo);

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
};

}
