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

class DllModuleExport SuperPixelFeatureConfig : public ModuleConfig {

public:
	SuperPixelFeatureConfig();

	virtual QString toString() const override;

protected:

	//void load(const QSettings& settings) override;
	//void save(QSettings& settings) const override;
};

class DllModuleExport SuperPixelFeature : public Module {

public:
	SuperPixelFeature(const cv::Mat& img, const PixelSet& set);

	bool isEmpty() const override;
	bool compute() override;
	QSharedPointer<SuperPixelFeatureConfig> config() const;

	cv::Mat draw(const cv::Mat& img) const;
	QString toString() const override;

	cv::Mat features() const;
	PixelSet set() const;

private:
	cv::Mat mImg;
	PixelSet mSet;

	// output
	cv::Mat mDescriptors;

	bool checkInput() const override;
	void syncSuperPixels(const std::vector<cv::KeyPoint>& keyPointsOld, const std::vector<cv::KeyPoint>& keyPointsNew);
};

class DllModuleExport SuperPixelClassifierConfig : public ModuleConfig {

public:
	SuperPixelClassifierConfig();

	virtual QString toString() const override;
	
	void setClassifierPath(const QString& path);
	QString classifierPath() const;

protected:

	QString mClassifierPath;

	void load(const QSettings& settings) override;
	void save(QSettings& settings) const override;
};

class DllModuleExport SuperPixelClassifier : public Module {

public:
	SuperPixelClassifier(const cv::Mat& img, const PixelSet& set);

	bool isEmpty() const override;
	bool compute() override;
	QSharedPointer<SuperPixelClassifierConfig> config() const;

	cv::Mat draw(const cv::Mat& img) const;
	QString toString() const override;

	void setModel(const QSharedPointer<SuperPixelModel>& model);

private:
	cv::Mat mImg;
	PixelSet mSet;
	QSharedPointer<SuperPixelModel> mModel;

	bool checkInput() const override;
};

class DllModuleExport GraphCutLabels : public Module {

public:
	GraphCutLabels(const PixelSet& set);

	bool isEmpty() const override;
	bool compute() override;

	//QString toString() const override;
	//QSharedPointer<LocalOrientationConfig> config() const;

	// results - available after compute() is called
	PixelSet set() const;
	void setModel(const QSharedPointer<SuperPixelModel>& model);

private:

	// input/output
	PixelSet mSet;
	QSharedPointer<SuperPixelModel> mModel;
	double mScaleFactor = 1000.0;	// TODO: think about that

	bool checkInput() const override;

	void graphCut(const PixelGraph& graph);
	cv::Mat costs(int numLabels) const;
	cv::Mat labelDistMatrix(int numLabels) const;
};


};