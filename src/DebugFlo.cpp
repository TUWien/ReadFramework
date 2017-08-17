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

#include "DebugFlo.h"
#include "Image.h"
#include "Utils.h"
#include "Binarization.h"
#include "SkewEstimation.h"
#include "Algorithms.h"
#include "ImageProcessor.h"
#include "Blobs.h"
#include "Shapes.h"
#include "LineTrace.h"
#include "FormAnalysis.h"
#include "PageParser.h"

#pragma warning(push, 0)	// no warnings from includes
// Qt Includes
#include <QDebug>
#include <QDir>
#include <QImage>
#include <QFileInfo>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv/highgui.h>
#include <opencv2/ml.hpp>
#pragma warning(pop)


namespace rdf {

	BinarizationTest::BinarizationTest(const DebugConfig& config) {

		mConfig = config;
	}

	void BinarizationTest::binarizeTest() {

		qDebug() << mConfig.imagePath() << "loaded";

		rdf::Timer dt;
		QImage img;
		img.load(mConfig.imagePath());
		cv::Mat inputImg = rdf::Image::qImage2Mat(img);

		qDebug() << "image converted...";
		if (inputImg.empty()) {
			qDebug() << "image is empty...";
			return;
		}

		
		//test Otsu
		//cv::Mat binImg = rdf::Algorithms::threshOtsu(inputImg);
		//flip image
		//inputImg = inputImg.t();
		//flip(inputImg, inputImg, 0);
		cv::Mat inputG;
		if (inputImg.channels() != 1) cv::cvtColor(inputImg, inputG, CV_RGB2GRAY);
		//if (inputImg.depth() != CV_8U) inputImg.convertTo(inputImg, CV_8U, 255);
		//findContours test
		//std::vector<std::vector<cv::Point> > contours;
		//std::vector<cv::Vec4i> hierarchy;
		//cv::findContours(inputImg, contours, hierarchy, cv::RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE);



		cv::Mat mask = IP::estimateMask(inputG);
		//Image::save(mask, "D:\\tmp\\mask.tif");
		//Image::imageInfo(inputImg, "input");


		////test LSD
		////-------------------------------------------------------------------------------------
		//ReadLSD lsd(inputG);
		////ReadLSD(inputG, mask);

		//lsd.compute();

		////end LSD
		////-------------------------------------------------------------------------------------



		//test registration
		//-------------------------------------------------------------------------------------
		QImage imgTemplate;
		//TODO: change path to template
		QString templPath = "D:\\projects\\READ\\formTest\\5117-087-0010.jpg";
		imgTemplate.load(templPath);
		qDebug() << templPath << "loaded template";
		cv::Mat imgTempl = rdf::Image::qImage2Mat(imgTemplate);
		cv::Mat imgTemplG;
		if (imgTempl.channels() != 1) cv::cvtColor(imgTempl, imgTemplG, CV_RGB2GRAY);
		cv::Mat maskTempl = rdf::IP::estimateMask(imgTemplG);
		FormFeatures formTempl(imgTemplG);
		formTempl.compute();

		//FormFeatures cmpImg(inputG);
		//cmpImg.compute();
		//if (cmpImg.compareWithTemplate(formTempl)) {
		//	qDebug() << "Match is true";
		//} else {
		//	qDebug() << "Match is false";
		//}

		//cv::Mat detLineImg = inputImg.clone();
		////rdf::LineTrace::generateLineImage(cmpImg.horLines(), cmpImg.verLines(), inputG);
		////detLineImg = cmpImg.getMatchedLineImg(detLineImg);
		//detLineImg = cmpImg.getMatchedLineImg(imgTempl, cmpImg.offset());
		//end test registration
		//-------------------------------------------------------------------------------------		
		
		//test binary linetracer
		//-------------------------------------------------------------------------------------

		//rdf::BaseBinarizationSu testBin(inputImg);
		//testBin.setPreFiltering(false);
		//testBin.compute();
		//cv::Mat binImg = testBin.binaryImage();
		//rdf::LineTrace linetest(binImg);
		//linetest.compute();
		//Image::save(binImg, "D:\\tmp\\test.tif");

		////cv::Mat lImg = linetest.lineImage();
		////cv::Mat synLine = linetest.generatedLineImage();
		////Image::save(synLine, "D:\\tmp\\synLine.tif");
		//end test binary linetracer
		//-------------------------------------------------------------------------------------

		//test skew estimation
		//-------------------------------------------------------------------------------------
		//rdf::BaseSkewEstimation skewTest;

		//skewTest.setImages(inputImg/*, mask*/);
		//bool skewComp = skewTest.compute();
		//if (!skewComp) {
		//	qDebug() << "could not compute skew";
		//}

		//double skewAngle = skewTest.getAngle();
		//skewAngle = -skewAngle / 180.0 * CV_PI;

		//QVector<QVector4D> selLines = skewTest.getSelectedLines();
		//for (int iL = 0; iL < selLines.size(); iL++) {
		//	QVector4D line = selLines[iL];

		//	cv::line(inputImg, cv::Point((int)line.x(),(int)line.y()), cv::Point((int)line.z(),(int)line.w()), cv::Scalar(255, 255, 255), 3);

		//}

		//cv::Mat rotatedImage = rdf::Algorithms::rotateImage(inputImg, skewAngle);

		////int nSamples = 20;
		////cv::RNG rng;
		////cv::Mat lBound(1, 1, CV_64FC1);
		////lBound.setTo(0);
		////cv::Mat uBound(1, 1, CV_64FC1);
		////uBound.setTo(nSamples - 1);

		////cv::Mat randRows(nSamples, 1, CV_32FC1);
		////rng.fill(randRows, cv::RNG::UNIFORM, lBound, uBound);
		////qDebug() << Image::printImage(randRows, "test");

		////cv::Mat tmp = inputImg.clone();
		////cv::bilateralFilter(inputImg, tmp, 5, 90, 90);
		////inputImg = tmp;
		////cv::medianBlur(inputImg, tmp, 41);
		////cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(7,7));
		////cv::erode(inputImg, tmp, kernel);
		////cv::dilate(tmp, tmp, kernel);
		////Image::save(inputImg, "D:\\tmp\\filterImg.tif");

		////rdf::BinarizationSuAdapted testBin(inputImg);
		////testBin.compute();
		////cv::Mat binImg = testBin.binaryImage();

		////Image::save(binImg, "D:\\tmp\\binImg.tif");

		//////binImg = Algorithms::preFilterArea(binImg, 10);

		////rdf::LineTrace linetest(binImg);
		////linetest.setMinLenSecondRun(40);
		//////linetest.setMaxAspectRatio(0.2f);
		//////linetest.setAngle(0.0f);
		////linetest.compute();

		////cv::Mat lImg = linetest.lineImage();

		////cv::Mat synLine = linetest.generatedLineImage();
		////Image::save(synLine, "D:\\tmp\\synLine.tif");



		////rdf::Blobs binBlobs;
		////binBlobs.setImage(binImg);
		////binBlobs.compute();

		////QVector<rdf::Blob> blobs = binBlobs.blobs();

		////binBlobs.setBlobs(rdf::BlobManager::instance().filterArea(20, binBlobs));
		////binBlobs.setBlobs(rdf::BlobManager::instance().filterMar(0.3f,200, binBlobs));
		////binBlobs.setBlobs(rdf::BlobManager::instance().filterAngle(0,));

		////qDebug() << "blobs #: " << binBlobs.blobs().size();

		////cv::Mat testImg;
		////testImg = rdf::BlobManager::instance().drawBlobs(binBlobs);



		////rdf::BinarizationSuFgdWeight testBin(inputImg);
		////testBin.compute();
		////cv::Mat binImg = testBin.binaryImage();

		////rdf::Image::imageInfo(binImg, "binImg");
		////qDebug() << testBin << " in " << dt;
		
		//QImage resultImg = rdf::Image::mat2QImage(detLineImg);
		QImage resultImg;

		if (!mConfig.outputPath().isEmpty()) {
			qDebug() << "saving to" << mConfig.outputPath();

			//binImgQt = binImgQt.convertToFormat(QImage::Format_RGB888);
			//binImgQt.save(mConfig.outputPath());
			rdf::Image::save(resultImg, mConfig.outputPath());
			
		} else {
			qDebug() << "no save path";
		}
	}



	TableTest::TableTest(const DebugConfig & config) {

		mConfig = config;

	}

	bool TableTest::match() const {

		cv::Mat imgForm;
		rdf::PageXmlParser parser;

		if (!load(imgForm))
			return false;


		cv::Mat imgFormG = imgForm;
		if (imgForm.channels() != 1)
			cv::cvtColor(imgForm, imgFormG, CV_RGB2GRAY);
		//cv::Mat maskTempl = rdf::Algorithms::estimateMask(imgTemplG);
		rdf::FormFeatures formF(imgFormG);
		formF.setFormName("testForm");
		formF.setSize(imgFormG.size());

		if (mConfig.tableTemplate().isEmpty())
			return false;

		//TODO check if tableTemplate is correct
		QFileInfo tmpInfo(mConfig.tableTemplate()); //tmpInfo.filePath();
		formF.setTemplateName(mConfig.tableTemplate());

		QSharedPointer<rdf::FormFeaturesConfig> tmpConfig(new rdf::FormFeaturesConfig());
		//(*tmpConfig) = mFormConfig;
		tmpConfig->setTemplDatabase(mConfig.tableTemplate());
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

		if (!resultImg.empty()) {
			qDebug() << "Match template...";
			formF.matchTemplate();

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