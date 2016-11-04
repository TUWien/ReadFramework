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

#include "Utils.h"
#include "BaseImageElement.h"

#pragma warning(push, 0)	// no warnings from includes
#include <QColor>
#include <QStringList>
#pragma warning(pop)

#ifndef DllCoreExport
#ifdef DLL_CORE_EXPORT
#define DllCoreExport Q_DECL_EXPORT
#else
#define DllCoreExport Q_DECL_IMPORT
#endif
#endif

// Qt defines
class QJsonObject;

namespace rdf {

// read defines
class Pixel;

/// <summary>
/// This class is used for mapping classes (e.g. handwriting, decoration)
/// </summary>
class DllCoreExport LabelLookup {

public:

	enum DefaultLabels {
		label_background = 0,
		label_ignore,
		label_unknown,	// unknown is not -1 because we only have unsigned bytes for labels

		label_end
	};

	LabelLookup(int id = label_unknown, const QString& mName = QString());
	bool operator== (const LabelLookup& l1) const;
	DllCoreExport friend QDataStream& operator<< (QDataStream& s, const LabelLookup& v);
	DllCoreExport friend QDebug operator<< (QDebug d, const LabelLookup& v);

	bool isNull() const;
	bool contains(const QString& key) const;

	int id() const;
	QString name() const;
	QColor color() const;
	QColor visColor() const;

	QString toString() const;

	static LabelLookup fromString(const QString& str);
	static LabelLookup fromJson(const QJsonObject& jo);
	static QString jsonKey();
	static int color2Id(const QColor& col);

	// create default labels
	static LabelLookup ignoreLabel();
	static LabelLookup unknownLabel();
	static LabelLookup backgroundLabel();

protected:

	int mId = label_unknown;
	QString mName = "unknown";
	QStringList mAlias;
	QColor mVisColor = ColorManager::darkGray();
};

class DllCoreExport PixelLabel : public BaseElement {

public:
	PixelLabel(const QString& id = QString());

	void setLabel(const LabelLookup& label);
	LabelLookup label() const;

	void setTrueLabel(const LabelLookup& label);
	LabelLookup trueLabel() const;

protected:
	LabelLookup mTrueLabel = LabelLookup::label_unknown;
	LabelLookup mLabel = LabelLookup::label_unknown;
};

};