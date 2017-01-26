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

	class DllCoreExport FormFeaturesConfig : public ModuleConfig {

	public:
		FormFeaturesConfig();

		float threshLineLenRation() const;
		void setThreshLineLenRation(float s);

		float distThreshold() const;
		void setDistThreshold(float d);

		float errorThr() const;
		void setErrorThr(float e);

		int searchXOffset() const;
		int searchYOffset() const;

		QString templDatabase() const;
		void setTemplDatabase(QString s);


		QString toString() const override;

	private:
		void load(const QSettings& settings) override;
		void save(QSettings& settings) const override;

		QString mTemplDatabase;

		float mThreshLineLenRatio = 0.6f;
		float mDistThreshold = 30.0;
		float mErrorThr = 15.0;

		int mSearchXOffset = 200;
		int mSearchYOffset = 200;

	};

	class DllCoreExport FormFeatures : public Module {

	public:
		FormFeatures();
		FormFeatures(const cv::Mat& img, const cv::Mat& mask = cv::Mat());

		bool loadTemplateDatabase(QString db);
		QVector<rdf::FormFeatures> templatesDb() const;

		cv::Mat getMatchedLineImg(const cv::Mat& srcImg, const Vector2D& offset = Vector2D(0,0)) const;

		void setInputImg(const cv::Mat& img);
		void setMask(const cv::Mat& mask);
		bool isEmpty() const override;
		bool compute() override;
		bool computeBinaryInput();
		bool compareWithTemplate(const FormFeatures& fTempl);
		cv::Size sizeImg() const;
		void setSize(cv::Size s);
		QVector<rdf::Line> horLines() const;
		void setHorLines(const QVector<rdf::Line>& h);
		QVector<rdf::Line> horLinesMatched() const;
		QVector<rdf::Line> verLines() const;
		void setVerLines(const QVector<rdf::Line>& v);
		QVector<rdf::Line> verLinesMatched() const;
		cv::Point offset() const;
		double error() const;

		QSharedPointer<FormFeaturesConfig> config() const;

		cv::Mat binaryImage() const;
		void setEstimateSkew(bool s);
		//void setThreshLineLenRatio(float l);
		//void setThresh(int thresh);
		//int thresh() const;
		QString toString() const override;

		void setFormName(QString s);
		QString formName() const;

	protected:

		float errLine(const cv::Mat& distImg, const rdf::Line l, cv::Point offset = cv::Point(0,0));
		void findOffsets(const QVector<Line>& hT, const QVector<Line>& vT, QVector<int>& offX, QVector<int>& offY) const;
		

	private:
		cv::Mat mSrcImg;
		cv::Mat mMask;
		cv::Mat mBwImg;
		bool mEstimateSkew = false;
		bool mPreFilter = true;
		int preFilterArea = 10;
		double mPageAngle = 0.0;
		double mMinError = std::numeric_limits<double>::max();

		QVector<rdf::Line> mHorLines;
		QVector<rdf::Line> mVerLines;

		QVector<rdf::FormFeatures> mTemplates;

		QVector<rdf::Line> mHorLinesMatched;
		QVector<rdf::Line> mVerLinesMatched;
		cv::Point mOffset;
		cv::Size mSizeSrc;

		// parameters

		bool checkInput() const override;
		QString mFormName;

		//void load(const QSettings& settings) override;
		//void save(QSettings& settings) const override;
	};

};