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

#pragma once

#include "Shapes.h"

#pragma warning(push, 0)	// no warnings from includes
#include <QSharedPointer>
#include <opencv2/core.hpp>
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
class QSettings;

namespace rdf {

// read defines
class Pixel;
class PixelEdge;

/// <summary>
/// Contains basic algorithms to manipulate images.
/// </summary>
class DllCoreExport Algorithms {

public:

	static int doubleEqual(double a, double b);
	static double absAngleDiff(double a, double b);
	static double signedAngleDiff(double a, double b);

	// convenience functions
	static double min(const QVector<double>& vec);
	static double max(const QVector<double>& vec);

	static double normAngleRad(double angle, double startIvl = 0.0, double endIvl = 2.0*CV_PI);
	static double angleDist(double angle1, double angle2, double maxAngle = 2.0*CV_PI);

	// template functions --------------------------------------------------------------------
	
	/// <summary>
	/// Computes robust statistical moments (quantiles).
	/// </summary>
	/// <param name="valuesIn">The statistical set (samples).</param>
	/// <param name="momentValue">The statistical moment value (0.5 = median, 0.25 and 0.75 = quartiles).</param>
	/// <param name="interpolated">A flag if the value should be interpolated if the length of the list is even.</param>
	/// <returns>The statistical moment.</returns>
	template <typename numFmt>
	static double statMoment(const QList<numFmt>& valuesIn, double momentValue, bool interpolated = true) {

		// no stat moment if we have 1 value
		if (valuesIn.size() == 1)
			return valuesIn[0];
		
		// return mean if we have two & interpolation is turned on
		if (valuesIn.size() == 2 && interpolated) {
			return (valuesIn[0] + valuesIn[1]) / 2.0;
		}
		else if (valuesIn.size() == 2)
			return valuesIn[0];

		QList<numFmt> values = valuesIn;
		qSort(values);

		size_t lSize = values.size();
		double moment = -1;
		unsigned int momIdx = cvCeil(lSize*momentValue);
		unsigned int idx = 1;

		// find the statistical moment
		for (auto val : values) {

			// skip
			if (idx < momIdx) {
				idx++;
				continue;
			}

			// compute mean between this and the next element
			if (lSize % 2 == 0 && momIdx < lSize && interpolated)
				moment = ((double)val + values[idx+1])*0.5;
			else
				moment = (double)val;
			break;
		}

		return moment;
	}

};

/// <summary>
/// Implements robust line fitting algorithms.
/// </summary>
class DllCoreExport LineFitting {

public:
	LineFitting(const QVector<Vector2D>& pts);

	Line fitLineLMS() const;
	Line fitLine() const;

protected:
	// parameters:
	int mNumSets = 1000;	// # random sets generated
	int mSetSize = 2;		// if 2, lines are directly returned
	double mEps = 0.1;		// if LMS is smaller than that, we break
	double mMinLength = 2;	// minimum line length

	QVector<Vector2D> mPts;

	void sample(const QVector<Vector2D>& pts, QVector<Vector2D>& set, int setSize = 2) const;
	double medianResiduals(const QVector<Vector2D>& pts, const Line& line) const;
};

// pixel distance functions
namespace PixelDistance {
	DllCoreExport double euclidean(const Pixel* px1, const Pixel* px2);
	DllCoreExport double mahalanobis(const Pixel* px1, const Pixel* px2);
	DllCoreExport double bhattacharyya(const Pixel* px1, const Pixel* px2);
	DllCoreExport double angleWeighted(const Pixel* px1, const Pixel* px2);

	DllCoreExport typedef double (*PixelDistanceFunction)(const Pixel* px1, const Pixel* px2);

	DllCoreExport double spacingWeighted(const PixelEdge* edge);
	DllCoreExport double orientationWeighted(const PixelEdge* edge);
	DllCoreExport double euclidean(const PixelEdge* edge);
	DllCoreExport typedef double (*EdgeWeightFunction)(const PixelEdge* edge);
}

}
