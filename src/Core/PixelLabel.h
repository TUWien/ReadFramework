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

#include "Drawer.h"
#include "BaseImageElement.h"

#pragma warning (disable: 4251)	// inlined Qt functions in dll interface

#pragma warning(push, 0)	// no warnings from includes
#include <QColor>
#include <QStringList>
#include <QVector>
#pragma warning(pop)

#ifndef DllCoreExport
#ifdef DLL_CORE_EXPORT
#define DllCoreExport Q_DECL_EXPORT
#else
#define DllCoreExport Q_DECL_IMPORT
#endif
#endif

namespace cv {
	namespace ml {
		class StatModel;
		class RTrees;
	}
}

// Qt defines
class QJsonObject;
class QPainter;

namespace rdf {

// read defines
class Pixel;
class Region;

/// <summary>
/// This class is used for mapping classes (e.g. handwriting, decoration)
/// </summary>
class DllCoreExport LabelInfo {

public:

	enum DefaultLabels {
		label_unknown = 0,	// unknown is not -1 because we only have unsigned bytes for labels

		label_end
	};

	LabelInfo(int id = label_unknown, const QString& mName = QString());
	bool operator== (const LabelInfo& l1) const;
	bool operator!= (const LabelInfo& l1) const;
	DllCoreExport friend QDataStream& operator<< (QDataStream& s, const LabelInfo& v);
	DllCoreExport friend QDebug operator<< (QDebug d, const LabelInfo& v);

	bool isNull() const;
	bool contains(const QString& key) const;

	int id() const;
	QString name() const;
	QColor color() const;
	QColor visColor() const;

	QString toString() const;

	static LabelInfo fromString(const QString& str);

	void toJson(QJsonObject& jo) const;
	static LabelInfo fromJson(const QJsonObject& jo);
	static QString jsonKey();
	static int color2Id(const QColor& col);

	// create default labels
	static LabelInfo unknownLabel();

protected:

	int mId = label_unknown;
	QString mName = "unknown";
	QStringList mAlias;
	QColor mVisColor = ColorManager::darkGray();
};

/// <summary>
/// This class manages all labels loaded.
/// It can be used to compare LabelInfo
/// objects, and load them.
/// </summary>
class DllCoreExport LabelManager {

public:
	LabelManager();

	bool isEmpty() const;
	int size() const;
	static LabelManager read(const QString& filePath);
	static LabelManager fromJson(const QJsonObject& jo);
	void toJson(QJsonObject& jo) const;

	void add(const LabelInfo& label);
	bool contains(const LabelInfo& label) const;
	bool containsId(const LabelInfo& label) const;

	QString toString() const;

	LabelInfo find(const QString& str) const;
	LabelInfo find(const Region& r) const;
	LabelInfo find(int id) const;

	static QString jsonKey();

	void draw(QPainter& p) const;

protected:
	QVector<LabelInfo> mLookups;
};

class DllCoreExport PixelLabel : public BaseElement {

public:
	PixelLabel(const QString& id = QString());

	bool isEvaluated() const;

	void setLabel(const LabelInfo& label);
	LabelInfo label() const;

	void setTrueLabel(const LabelInfo& label);
	LabelInfo trueLabel() const;

protected:
	LabelInfo mTrueLabel = LabelInfo::label_unknown;
	LabelInfo mLabel = LabelInfo::label_unknown;
};

class DllCoreExport SuperPixelModel {

public:
	SuperPixelModel(const LabelManager& labelManager = LabelManager(), const cv::Ptr<cv::ml::StatModel>& model = cv::Ptr<cv::ml::StatModel>());

	bool isEmpty() const;

	cv::Ptr<cv::ml::StatModel> model() const;
	LabelManager manager() const;

	QVector<PixelLabel> classify(const cv::Mat& features) const;

	bool write(const QString& filePath) const;
	static QSharedPointer<SuperPixelModel> read(const QString& filePath);

protected:
	cv::Ptr<cv::ml::StatModel> mModel;
	LabelManager mManager;

	static cv::Ptr<cv::ml::RTrees> readRTreesModel(QJsonObject& jo);
	void toJson(QJsonObject& jo) const;
};

}
