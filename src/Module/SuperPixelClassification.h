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

#include "BaseModule.h"
#include "PixelSet.h"

#pragma warning(push, 0)	// no warnings from includes

#pragma warning(pop)

#ifndef DllModuleExport
#ifdef DLL_MODULE_EXPORT
#define DllModuleExport Q_DECL_EXPORT
#else
#define DllModuleExport Q_DECL_IMPORT
#endif
#endif

// Qt defines

namespace rdf {

// read defines
class Region;

class DllModuleExport SuperPixelLabelerConfig : public ModuleConfig {

public:
	SuperPixelLabelerConfig();

	virtual QString toString() const override;

protected:

	//void load(const QSettings& settings) override;
	//void save(QSettings& settings) const override;
};

/// <summary>
/// This class is used for mapping classes (e.g. handwriting, decoration)
/// </summary>
class DllModuleExport LabelLookup {

public:
	LabelLookup();
	bool operator==(const LabelLookup& l1) const;
	DllModuleExport friend QDataStream& operator<<(QDataStream& s, const LabelLookup& v);
	DllModuleExport friend QDebug operator<< (QDebug d, const LabelLookup& v);

	bool isNull() const;
	bool contains(const QString& key) const;

	QColor color() const;
	int id() const;
	QString name() const;

	QString toString() const;

	static LabelLookup fromString(const QString& str);
	static QString jsonKey();
	static int color2Id(const QColor& col);

protected:

	int mId = -1;
	QString mName = "unknown";
	QStringList mAlias;

};

class DllModuleExport LabelManager {

public:
	LabelManager();

	bool isEmpty() const;
	int size() const;
	static LabelManager read(const QString& filePath);

	void add(const LabelLookup& label);
	bool contains(const LabelLookup& label) const;
	bool containsId(const LabelLookup& label) const;

	QString toString() const;

	LabelLookup find(const QString& str) const;
	LabelLookup find(const Region& r) const;

protected:
	QVector<LabelLookup> mLookups;
};

class DllModuleExport SuperPixelLabeler : public Module {

public:
	SuperPixelLabeler(const QVector<QSharedPointer<MserBlob> >& blobs, const Rect& imgRect);

	bool isEmpty() const override;
	bool compute() override;
	QSharedPointer<SuperPixelLabelerConfig> config() const;

	cv::Mat draw(const cv::Mat& img) const;
	QString toString() const override;

	void setRootRegion(const QSharedPointer<Region>& region);
	void setLabelManager(const LabelManager& manager);
	QImage createLabelImage(const Rect& imgRect) const;

private:
	QVector<QSharedPointer<MserBlob> > mBlobs;
	QSharedPointer<Region> mGtRegion;
	Rect mImgRect;
	LabelManager mManager;

	bool checkInput() const override;
};

class DllModuleExport SuperPixelClassificationConfig : public ModuleConfig {

public:
	SuperPixelClassificationConfig();

	virtual QString toString() const override;
	
	int maxSide() const;

protected:

	int mMaxSide = 200;

	//void load(const QSettings& settings) override;
	//void save(QSettings& settings) const override;
};

class DllModuleExport SuperPixelClassification : public Module {

public:
	SuperPixelClassification(const cv::Mat& img, const PixelSet& set);

	bool isEmpty() const override;
	bool compute() override;
	QSharedPointer<SuperPixelClassificationConfig> config() const;

	cv::Mat draw(const cv::Mat& img) const;
	QString toString() const override;

private:
	cv::Mat mImg;

	PixelSet mSet;

	bool checkInput() const override;
};

};