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
#include <QSettings>
#include <QTime>

#include <opencv2/core/core.hpp>
#pragma warning(pop)

#pragma warning (disable: 4251)	// inlined Qt functions in dll interface

#ifndef DllCoreExport
#ifdef DLL_CORE_EXPORT
#define DllCoreExport Q_DECL_EXPORT
#else
#define DllCoreExport Q_DECL_IMPORT
#endif
#endif

#define DK_DEG2RAD	0.017453292519943
#define DK_RAD2DEG 	57.295779513082323

// Qt defines
class QSettings;

namespace rdf {	

// read defines
class DllCoreExport Utils {

public:
	static Utils& instance();

	void initFramework() const;
	void registerVersion() const;
	static double rand();

	QString createFilePath(const QString& filePath, const QString& attribute, const QString& newSuffix = QString()) const;
	QString baseName(const QString& filePath) const;

private:
	Utils();
	Utils(const Utils&);
};

class DllCoreExport ColorManager {

public:
	static ColorManager& instance();

	QColor getRandomColor(int idx = -1) const;
	QVector<QColor> colors() const;

private:
	ColorManager();
	ColorManager(const ColorManager&);
};


class DllCoreExport Converter {

public:
	static Converter& instance();

	QPolygon stringToPoly(const QString& pointList) const;
	QString polyToString(const QPolygon& poly) const;

	static QPointF cvPointToQt(const cv::Point& pt);
	static cv::Point2d qPointToCv(const QPointF& pt);

	static QRectF cvRectToQt(const cv::Rect& r);
	static cv::Rect2d qRectToCv(const QRectF& r);


private:
	Converter();
	Converter(const Converter&);
};


/**
* A small class which measures the time.
* This class is designed to measure the time of a method, especially
* intervals and the total time can be measured.
**/
class DllCoreExport Timer {

public:

	/**
	* Initializes the class and stops the clock.
	**/
	Timer();

	friend DllCoreExport QDataStream& operator<<(QDataStream& s, const Timer& t);
	friend DllCoreExport QDebug operator<< (QDebug d, const Timer &t);

	QString getTotal() const;
	virtual QDataStream& put(QDataStream& s) const;
	QString stringifyTime(int ct) const;
	int elapsed() const;
	void start();

protected:

	QTime mTimer;
};

};