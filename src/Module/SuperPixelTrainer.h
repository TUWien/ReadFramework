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

#include "BaseModule.h"
#include "PixelSet.h"

#pragma warning(push, 0)	// no warnings from includes

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
		class TrainData;
		class RTrees;
	}
}

// Qt defines

namespace rdf {

// read defines
class Region;
class RootRegion;
class PageElement;

/// <summary>
/// FeatureCollection maps one LabelInfo to its features.
/// In addition it handles the I/O using Json.
/// </summary>
class DllCoreExport FeatureCollection {

public:
	FeatureCollection(const cv::Mat& descriptors = cv::Mat(), const LabelInfo& label = LabelInfo());
	friend DllCoreExport bool operator==(const FeatureCollection& fcl, const FeatureCollection& fcr);

	QJsonObject toJson(const QString& filePath = "") const;
	static FeatureCollection read(QJsonObject& jo, const QString& filePath = "");

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
class DllCoreExport FeatureCollectionManager {

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
class DllCoreExport SuperPixelLabelerConfig : public ModuleConfig {

public:
	SuperPixelLabelerConfig();

	QString featureFilePath() const;
	QString labelConfigFilePath() const;
	int maxNumFeaturesPerImage() const;
	int minNumFeaturesPerClass() const;
	int maxNumFeaturesPerClass() const;
	QString backgroundLabelName() const;

	virtual QString toString() const override;

protected:

	void load(const QSettings& settings) override;
	void save(QSettings& settings) const override;

	QString mFeatureFilePath;
	QString mLabelConfigFilePath;
	QString mBackgroundLabelName = "";		// class name of background pixels (if empty, they are treated as unknown i.e. not trained)
	int mMaxNumFeaturesPerImage = 1000000;	// 1e6
	int mMinNumFeaturesPerClass = 10000;	// 1e4
	int mMaxNumFeaturesPerClass = 10000;	// 1e4;

};

/// <summary>
/// Converts GT information to label images.
/// This class is used to assign a GT label
/// to each SuperPixel computed in an image.
/// </summary>
/// <seealso cref="Module" />
class DllCoreExport SuperPixelLabeler : public Module {

public:
	SuperPixelLabeler(const QVector<QSharedPointer<MserBlob> >& blobs, const Rect& imgRect);
	SuperPixelLabeler(const PixelSet& set = PixelSet(), const Rect& imgRect = Rect());

	bool isEmpty() const override;
	bool compute() override;
	QSharedPointer<SuperPixelLabelerConfig> config() const;

	cv::Mat draw(const cv::Mat& img, bool drawPixels = true) const;
	QString toString() const override;

	void setFilePath(const QString& filePath);
	void setRootRegion(const QSharedPointer<RootRegion>& region);
	void setLabelManager(const LabelManager& manager);
	void setPage(const QSharedPointer<PageElement>& page);
	QImage createLabelImage(const Rect& imgRect, bool visualize = false) const;

	PixelSet set() const;

private:
	QVector<QSharedPointer<MserBlob> > mBlobs;

	QSharedPointer<RootRegion> mGtRegion;
	QSharedPointer<PageElement> mPage;
	Rect mImgRect;
	LabelManager mManager;
	QString mGlobalName;

	// results
	PixelSet mSet;

	bool checkInput() const override;
	PixelSet labelBlobs(const cv::Mat& labelImg, const QVector<QSharedPointer<MserBlob> >& blobs) const;
	PixelSet labelPixels(const cv::Mat& labelImg, const PixelSet& set) const;
	QString parseLabel(const QString& filePath) const;

	void setBackgroundLabelName(const QString& name);
};

class DllCoreExport SuperPixelTrainerConfig : public ModuleConfig {

public:
	SuperPixelTrainerConfig();

	QStringList featureCachePaths() const;
	QString modelPath() const;

	virtual QString toString() const override;

	void setNumTrees(int numTrees);
	int numTrees() const;

protected:

	QStringList mFeatureCachePaths;
	QString mModelPath;
	int mNumTrees = 150;

	void load(const QSettings& settings) override;
	void save(QSettings& settings) const override;
};

/// <summary>
/// Converts GT information to label images.
/// This class is used to assign a GT label
/// to each SuperPixel computed in an image.
/// </summary>
/// <seealso cref="Module" />
class DllCoreExport SuperPixelTrainer : public Module {

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

}
