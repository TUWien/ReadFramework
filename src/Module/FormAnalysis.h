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
#include "LineTrace.h"

#pragma warning(push, 0)	// no warnings from includes
#include <QObject>

#include "opencv2/core/core.hpp"
#pragma warning(pop)

// TODO: add DllExport magic

// Qt defines

namespace rdf {

	class DllModuleExport FormFeaturesConfig : public ModuleConfig {

	public:
		FormFeaturesConfig();

		float threshLineLenRation() const;
		void setThreshLineLenRation(float s);


		QString toString() const override;

	private:
		void load(const QSettings& settings) override;
		void save(QSettings& settings) const override;

		float mThreshLineLenRatio = 0.6f;

	};


	class DllModuleExport FormFeatures : public Module {

	public:
		FormFeatures();
		FormFeatures(const cv::Mat& img, const cv::Mat& mask);

		void setInputImg(const cv::Mat& img);
		void setMask(const cv::Mat& mask);
		bool isEmpty() const override;
		bool compute() override;
		bool computeBinaryInput();
		bool compareWithTemplate(const FormFeatures& fTempl);
		QVector<rdf::Line> horLines() const;
		QVector<rdf::Line> verLines() const;

		QSharedPointer<FormFeaturesConfig> config() const;

		cv::Mat binaryImage() const;
		void setEstimateSkew(bool s);
		//void setThreshLineLenRatio(float l);
		//void setThresh(int thresh);
		//int thresh() const;
		QString toString() const override;


	protected:

		float errLine(const cv::Mat& distImg, const rdf::Line l, cv::Point offset = cv::Point(0,0));

	private:
		cv::Mat mSrcImg;
		cv::Mat mMask;
		cv::Mat mBwImg;
		bool mEstimateSkew = false;
		double mPageAngle = 0.0;

		QVector<rdf::Line> mHorLines;
		QVector<rdf::Line> mVerLines;

		cv::Size mSizeSrc;

		// parameters

		bool checkInput() const override;

		//void load(const QSettings& settings) override;
		//void save(QSettings& settings) const override;
	};

};