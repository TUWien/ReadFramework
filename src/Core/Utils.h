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
#pragma warning (disable: 4714)	// force inline

#ifndef DllCoreExport
#ifdef DLL_CORE_EXPORT
#define DllCoreExport Q_DECL_EXPORT
#else
#define DllCoreExport Q_DECL_IMPORT
#endif
#endif

// some basic defines - yes, we try to not create too many macros...
#define DK_DEG2RAD	0.017453292519943
#define DK_RAD2DEG 	57.295779513082323

// converts a version (e.g. 3.1.0) to a comparable int
#define RDF_VERSION(major, minor, revision) (major << 16 | minor << 8 | revision)
#define RDF_OPENCV_VERSION RDF_VERSION(CV_MAJOR_VERSION, CV_MINOR_VERSION, CV_VERSION_REVISION)

// Qt defines
class QSettings;

namespace rdf {	

#define WHO_IS_CUTE "Anna"

// read defines
class DllCoreExport Utils {

public:
	static Utils& instance();
	
	void initFramework() const;
	void registerVersion() const;
	static int versionToInt(char major, char minor, char revision);
	static double rand();

	static QString appDataPath();
	static QString createFilePath(const QString& filePath, const QString& attribute, const QString& newSuffix = QString());
	static QString baseName(const QString& filePath);

	static QJsonObject readJson(const QString& filePath);
	static int64 writeJson(const QString& filePath, const QJsonObject& jo);
	static void initDefaultFramework();

	// little number thingies
	template<typename num>
	static num clamp(num val, num min, num max) {
	
		if (val < min)
			val = min;
		if (val > max)
			val = max;

		return val;
	};

private:
	Utils();
	Utils(const Utils&);
};

class DllCoreExport Converter {

public:
	static QPolygon stringToPoly(const QString& pointList);
	static QString polyToString(const QPolygon& poly);

	static QPointF cvPointToQt(const cv::Point& pt);
	static cv::Point2d qPointToCv(const QPointF& pt);

	static QRectF cvRectToQt(const cv::Rect& r);
	static cv::Rect2d qRectToCv(const QRectF& r);

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

/// <summary>
/// Flags turns enums into typesave flags
/// It is strongly related (copied) from 
/// Useage:
/// 	enum mDrawFlags {
///			draw_none				= 0x00,
///			draw_ellipse			= 0x01,
///			draw_stats				= 0x02,
///			draw_center				= 0x04,
///			};
///		typedef Flags<mDrawFlags> DrawFlags;
///
/// http://stackoverflow.com/questions/1448396/how-to-use-enums-as-flags-in-c/33971769#33971769
/// thanks @Fabio A.
/// </summary>
template <typename EnumType, typename Underlying = int>
class Flags {
    typedef Underlying Flags::* RestrictedBool;

public:
	Flags() : mFlags(Underlying()) {}

    Flags(EnumType f) :
        mFlags(1 << f) {}

    Flags(const Flags& o):
        mFlags(o.mFlags) {}

    Flags& operator |=(const Flags& f) {
        mFlags |= f.mFlags;
        return *this;
    }

    Flags& operator &=(const Flags& f) {
        mFlags &= f.mFlags;
        return *this;
    }

    friend Flags operator |(const Flags& f1, const Flags& f2) {
        return Flags(f1) |= f2;
    }

    friend Flags operator &(const Flags& f1, const Flags& f2) {
        return Flags(f1) &= f2;
    }

    Flags operator ~() const {
        Flags result(*this);
        result.mFlags = ~result.mFlags;
        return result;
    }

    operator RestrictedBool() const {
        return mFlags ? &Flags::mFlags : 0;
    }

    Underlying value() const {
        return mFlags;
    }

protected:
    Underlying  mFlags;
};

};