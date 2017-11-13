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

#include "TableTest.h"
#include "FormAnalysis.h"		// tested
#include "PageParser.h"
#include "Image.h"
#include "Utils.h"
#include "ImageProcessor.h"


#pragma warning(push, 0)	// no warnings from includes
#include <QImage>
#include <QFileInfo>
#include <QDir>

#include <opencv2/ml.hpp>
#pragma warning(pop)

namespace rdf {

	TableTest::TableTest(const TestConfig & config) {

		mConfig = config;

	}

	bool TableTest::match(bool eval) const {

		cv::Mat imgForm;
		rdf::PageXmlParser parser;

		if (!load(imgForm))
			return false;

		//if (!load(parser))
		//return false;

		//cv::Mat imgForm = rdf::Image::qImage2Mat(imgCv);

		cv::Mat imgFormG = imgForm;
		if (imgForm.channels() != 1)
			cv::cvtColor(imgForm, imgFormG, CV_RGB2GRAY);
		//cv::Mat maskTempl = rdf::Algorithms::estimateMask(imgTemplG);
		rdf::FormFeatures formF(imgFormG);
		formF.setFormName("testForm");
		formF.setSize(imgFormG.size());

		QFileInfo tmpInfo(mConfig.templateXmlPath()); //tmpInfo.filePath();
		formF.setTemplateName(mConfig.templateXmlPath());

		QSharedPointer<rdf::FormFeaturesConfig> tmpConfig(new rdf::FormFeaturesConfig());
		//(*tmpConfig) = mFormConfig;
		tmpConfig->setTemplDatabase(mConfig.templateXmlPath());
		tmpConfig->setVariationThrLower(0.5);
		tmpConfig->setVariationThrUpper(0.55);
		formF.setConfig(tmpConfig);

		//rdf::FormFeatures formTemplate;
		QSharedPointer<rdf::FormFeatures> formTemplate(new rdf::FormFeatures());
		if (!formF.readTemplate(formTemplate)) {
			qWarning() << "no template set - aborting";
			qInfo() << "please provide a template Plugins > Read Config > Form Analysis > lineTemplPath";
			return false;
		}


		if (!formF.compute()) {
			qWarning() << "could not compute form template " << mConfig.imagePath();
			qInfo() << "could not compute form template";
			return false;
		}

		qDebug() << "Compute rough alignment...";
		formF.estimateRoughAlignment();

		cv::Mat drawImg = imgForm.clone();
		cv::cvtColor(drawImg, drawImg, CV_RGBA2BGR);
		cv::Mat resultImg;// = imgForm;

		resultImg = formF.drawAlignment(drawImg);
		bool matchSuccess = false;
		if (!resultImg.empty()) {
			qDebug() << "Match template...";
			matchSuccess = formF.matchTemplate();

		}

		if (!matchSuccess) {
			qWarning() << "could not match template " << mConfig.imagePath();
			return matchSuccess;
		}

		//calculate eval section
		if (eval) {

			rdf::FormEvaluation formEval;
			formEval.setSize(formF.sizeImg());
			//if (!formEval.setTemplate(mConfig.templateXmlPath())) {
			if (!formEval.setTemplate(mConfig.xmlPath())) {
				qWarning() << "could not find template for evaluation " << mConfig.templateXmlPath();
				qInfo() << "could not find template for evaluation";

				return false;
			}

			formEval.setTable(formF.tableRegion());


			formEval.computeEvalTableRegion();
			formEval.computeEvalCells();

			double tableJI = formEval.tableJaccard();
			double tableM = formEval.tableMatch();

			QVector<double> cellJI = formEval.cellJaccards();
			double meanCellJI = formEval.meanCellJaccard();

			QVector<double> cellM = formEval.cellMatches();
			double meanCellM = formEval.meanCellMatch();

			double missedCells = formEval.missedCells();
			double underSeg = formEval.underSegmented();
			//QVector<double> underSegCells = formEval.underSegmentedC();
			
			bool evalResultFail = false;
			if (tableJI < 0.9) {
				qDebug() << "tableJI failed...";
				evalResultFail = true;
			}
			if (tableM < 0.9) {
				qDebug() << "tableM failed...";
				evalResultFail = true;
			}
			if (meanCellM < 0.95) {
				qDebug() << "meanCellM failed...";
				evalResultFail = true;
			}
			if (meanCellJI < 0.95) {
				qDebug() << "meanCellJI failed...";
				evalResultFail = true;
			}
			if (missedCells > 0.05) {
				qDebug() << "missedCells failed...";
				evalResultFail = true;
			}
			if (underSeg > 0.05) {
				qDebug() << "underSeg failed...";
				evalResultFail = true;
			}


			if (evalResultFail) {
				qDebug() << "evalTable failed...";
				return false;
			}

		}
		return true;
	}


	bool TableTest::load(cv::Mat& img) const {

		QImage qImg = Image::load(mConfig.imagePath());

		if (qImg.isNull()) {
			qWarning() << "could not load image from" << mConfig.imagePath();
			return false;
		}

		// convert image
		img = Image::qImage2Mat(qImg);

		return true;
	}

	bool TableTest::load(rdf::PageXmlParser& parser) const {

		// load XML
		parser.read(mConfig.xmlPath());

		// fail if the XML was not loaded
		if (parser.loadStatus() != PageXmlParser::status_ok) {
			qWarning() << "could not load XML from" << mConfig.xmlPath();
			return false;
		}

		return true;
	}

}