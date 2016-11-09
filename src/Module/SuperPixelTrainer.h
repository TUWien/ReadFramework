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

	QString featureFilePath() const;
	QString labelConfigFilePath() const;
	int maxNumFeaturesPerImage() const;
	int minNumFeaturesPerClass() const;
	int maxNumFeaturesPerClass() const;

	virtual QString toString() const override;

protected:

	void load(const QSettings& settings) override;
	void save(QSettings& settings) const override;

	QString mFeatureFilePath;
	QString mLabelConfigFilePath;
	int mMaxNumFeaturesPerImage = 10000;	// 1e4
	int mMinNumFeaturesPerClass = 1000000;	// 1e6
	int mMaxNumFeaturesPerClass = 10000;	// 1e4;
};

class DllModuleExport LabelManager {

public:
	LabelManager();

	bool isEmpty() const;
	int size() const;
	static LabelManager read(const QString& filePath);

	void add(const LabelInfo& label);
	bool contains(const LabelInfo& label) const;
	bool containsId(const LabelInfo& label) const;

	QString toString() const;

	LabelInfo find(const QString& str) const;
	LabelInfo find(const Region& r) const;
	LabelInfo find(int id) const;

protected:
	QVector<LabelInfo> mLookups;
};

class DllModuleExport FeatureCollection {

public:
	FeatureCollection(const cv::Mat& descriptors = cv::Mat(), const LabelInfo& label = LabelInfo());

	QJsonObject toJson() const;
	static FeatureCollection read(QJsonObject& jo);

	void append(const cv::Mat& descriptor);
	LabelInfo label() const;

	static QVector<FeatureCollection> split(const cv::Mat& descriptors, const PixelSet& set);
	static QString jsonKey();

protected:
	cv::Mat mDesc;
	LabelInfo mLabel;
};

class DllModuleExport FeatureCollectionManager {

public:
	FeatureCollectionManager(const cv::Mat& descriptors = cv::Mat(), const PixelSet& set = PixelSet());

	void write(const QString& filePath) const;
	static FeatureCollectionManager read(const QString& filePath);
	void add(const FeatureCollection& collection);

	QVector<FeatureCollection> collection() const;

protected:
	QVector<FeatureCollection> mCollection;

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

	PixelSet set() const;

private:
	QVector<QSharedPointer<MserBlob> > mBlobs;
	QSharedPointer<Region> mGtRegion;
	Rect mImgRect;
	LabelManager mManager;

	// results
	PixelSet mSet;

	bool checkInput() const override;
	PixelSet labelBlobs(const cv::Mat& labelImg, const QVector<QSharedPointer<MserBlob> >& blobs) const;
	
};

};