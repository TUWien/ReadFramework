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

namespace cv {
	namespace ml {
		class TrainData;
		class RTrees;
	}
}

// Qt defines

namespace rdf {

// read defines
class Region;

/// <summary>
/// FeatureCollection maps one LabelInfo to its features.
/// In addition it handles the I/O using Json.
/// </summary>
class DllModuleExport FeatureCollection {

public:
	FeatureCollection(const cv::Mat& descriptors = cv::Mat(), const LabelInfo& label = LabelInfo());
	friend DllModuleExport bool operator==(const FeatureCollection& fcl, const FeatureCollection& fcr);

	QJsonObject toJson() const;
	static FeatureCollection read(QJsonObject& jo);

	void append(const cv::Mat& descriptor);
	LabelInfo label() const;
	void setDescriptors(const cv::Mat& desc);
	cv::Mat descriptors() const;
	int numDescriptors() const;

	static QVector<FeatureCollection> split(const cv::Mat& descriptors, const PixelSet& set);
	static QString jsonKey();

protected:
	cv::Mat mDesc;
	LabelInfo mLabel;
};

/// <summary>
/// Manages FeatureCollections.
/// Hence, each label (e.g. printed text) is stored
/// here along with it's features retrieved from
/// groundtruthed images.
/// </summary>
class DllModuleExport FeatureCollectionManager {

public:
	FeatureCollectionManager(const cv::Mat& descriptors = cv::Mat(), const PixelSet& set = PixelSet());

	bool isEmpty() const;

	void write(const QString& filePath) const;
	static FeatureCollectionManager read(const QString& filePath);
	
	void add(const FeatureCollection& collection);
	void merge(const FeatureCollectionManager& other);
	void normalize(int minFeaturesPerClass, int maxFeaturesPerClass);

	QVector<FeatureCollection> collection() const;

	int numFeatures() const;

	QString toString() const;
	cv::Ptr<cv::ml::TrainData> toCvTrainData(int maxSamples = -1) const;
	LabelManager toLabelManager() const;

protected:
	QVector<FeatureCollection> mCollection;

	cv::Mat allFeatures() const;
	cv::Mat allLabels() const;
};

/// <summary>
/// This class configures the feature collection process.
/// It controls I/O paths for feature labels and feature
/// cache files and the normalization process through the
/// read settings file.
/// </summary>
/// <seealso cref="ModuleConfig" />
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

/// <summary>
/// Converts GT information to label images.
/// This class is used to assign a GT label
/// to each SuperPixel computed in an image.
/// </summary>
/// <seealso cref="Module" />
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

class DllModuleExport SuperPixelTrainerConfig : public ModuleConfig {

public:
	SuperPixelTrainerConfig();

	QStringList featureCachePaths() const;
	QString modelPath() const;

	virtual QString toString() const override;

protected:

	QStringList mFeatureCachePaths;
	QString mModelPath;

	void load(const QSettings& settings) override;
	void save(QSettings& settings) const override;
};

/// <summary>
/// Converts GT information to label images.
/// This class is used to assign a GT label
/// to each SuperPixel computed in an image.
/// </summary>
/// <seealso cref="Module" />
class DllModuleExport SuperPixelTrainer : public Module {

public:
	SuperPixelTrainer(const FeatureCollectionManager& fcm);

	bool isEmpty() const override;
	bool compute() override;
	QSharedPointer<SuperPixelTrainerConfig> config() const;

	cv::Mat draw(const cv::Mat& img) const;
	QString toString() const override;

	// no read function -> see SuperPixelClassifier
	bool write(const QString& filePath) const;
	QSharedPointer<SuperPixelModel> model() const;

private:
	
	FeatureCollectionManager mFeatureManager;

	// results
	cv::Ptr<cv::ml::RTrees> mModel;

	bool checkInput() const override;
};

};