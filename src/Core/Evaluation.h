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

#include "BaseImageElement.h"

#pragma warning(push, 0)	// no warnings from includes
#include <QVector>
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

namespace rdf {

// read defines

/// <summary>
/// Information class for SuperPixel labeling.
/// </summary>
/// <seealso cref="BaseElement" />
class DllCoreExport EvalInfo : public BaseElement {

public:
	EvalInfo(const QString& name = "unknown");

	void operator+=(const EvalInfo& o);

	void eval(int trueClassId, int predictedClassId, bool isBackground = false);

	void setName(const QString& name);
	QString name() const;

	double accuracy() const;
	double fscore() const;
	double precision() const;
	double recall() const;

	int count() const;

	static QString header();
	QString toString() const;

private:
	int mTp = 0;	// true positives
	int mTn = 0;	// true negative
	int mFp = 0;	// false positives
	int mFn = 0;	// false negatives
	
	QString mName;
};

class DllCoreExport EvalInfoManager {

public:
	EvalInfoManager(const QVector<EvalInfo>& evals);

	bool write(const QString& filePath) const;
	QString toString() const;

private:
	QVector<EvalInfo> mEvals;

	QByteArray toBuffer() const;
};

}