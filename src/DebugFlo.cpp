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
 [1] http://www.cvl.tuwien.ac.at/cvl/
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
#include <QSettings>
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



	TableProcessing::TableProcessing(const DebugConfig & config) {

		mConfig = config;

	}

	bool TableProcessing::match() const {

		cv::Mat imgForm;
		rdf::PageXmlParser parser;

		if (!load(imgForm)) {
			qWarning() << "could not load image for table processing ... ";
			return false;
		}

		cv::Mat imgFormG = imgForm;

		if (imgForm.channels() != 1)
			cv::cvtColor(imgForm, imgFormG, CV_RGB2GRAY);
		else {
			cv::cvtColor(imgForm, imgForm, CV_GRAY2RGB);
		}
		
		//cv::Mat maskTempl = rdf::Algorithms::estimateMask(imgTemplG);

		rdf::FormFeatures formF(imgFormG);
		QFileInfo tableFile = QFileInfo(mConfig.imagePath());
		formF.setFormName(tableFile.fileName());
		formF.setSize(imgFormG.size());

		if (mConfig.tableTemplate().isEmpty()) {
			qWarning() << "no table template is set - please specify a template with --t ... ";
			return false;
		}

		//TODO check if tableTemplate is correct
		//QFileInfo templateInfo(mConfig.tableTemplate());
		formF.setTemplateName(mConfig.tableTemplate());

		QSharedPointer<rdf::FormFeaturesConfig> tmpConfig(new rdf::FormFeaturesConfig());
		//(*tmpConfig) = mFormConfig;
		tmpConfig->setTemplDatabase(mConfig.tableTemplate());
		tmpConfig->setVariationThrLower(mFormConfig.variationThrLower());
		tmpConfig->setVariationThrUpper(mFormConfig.variationThrUpper());
		tmpConfig->setSaveChilds(mFormConfig.saveChilds());
		formF.setConfig(tmpConfig);

		//rdf::FormFeatures formTemplate;
		QSharedPointer<rdf::FormFeatures> formTemplate(new rdf::FormFeatures());
		if (!formF.readTemplate(formTemplate)) {
			qWarning() << "could not read form template";
			qInfo() << "please provide a correct table template with --t";
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

		} else {
			qWarning() << "could not estimate alignement - abort " << mConfig.imagePath();
		}

		//Save output to xml
		QString loadXmlPath = rdf::PageXmlParser::imagePathToXmlPath(mConfig.imagePath());

		if (QFileInfo(mConfig.xmlPath()).exists())
			loadXmlPath = mConfig.xmlPath();

		rdf::PageXmlParser parserOut;
		bool newXML = parserOut.read(loadXmlPath);
		auto pe = parserOut.page();

		if (!newXML) {
			//xml is newly created
			cv::Size imgSize = imgFormG.size();
			QSize imgQtSize;
			imgQtSize.setWidth(imgFormG.cols);
			imgQtSize.setHeight(imgFormG.rows);

			pe->setImageFileName(tableFile.fileName());
			pe->setImageSize(imgQtSize);
			pe->setCreator("CVL");
			pe->setDateCreated(QDateTime::currentDateTime());
		}

		QSharedPointer<rdf::TableRegion> t = formF.tableRegion();
		pe->rootRegion()->addUniqueChild(t);
		formF.setSeparators(pe->rootRegion());

		//save pageXml
		parserOut.write(loadXmlPath, pe);

		return true;
	}

	void TableProcessing::setTableConfig(const rdf::FormFeaturesConfig & tableConfig) 	{
		mFormConfig = tableConfig;
	}


	bool TableProcessing::load(cv::Mat& img) const {

		QImage qImg = Image::load(mConfig.imagePath());

		if (qImg.isNull()) {
			qWarning() << "could not load image from" << mConfig.imagePath();
			return false;
		}

		// convert image
		img = Image::qImage2Mat(qImg);

		return true;
	}

	bool TableProcessing::load(rdf::PageXmlParser& parser) const {

		// load XML
		parser.read(mConfig.xmlPath());

		// fail if the XML was not loaded
		if (parser.loadStatus() != PageXmlParser::status_ok) {
			qWarning() << "could not load XML from" << mConfig.xmlPath();
			return false;
		}

		return true;
	}


	LineProcessing::LineProcessing(const DebugConfig & config) 	{
		mConfig = config;
	}

	bool LineProcessing::computeBinaryInput() 	{
		
		if (mSrcImg.empty() || mSrcImg.depth() != CV_8U) {
			qWarning() << "image is empty or illegal image depth: " << mSrcImg.depth();
			return false;
		}

		if (!mMask.empty() && mMask.depth() != CV_8U && mMask.channels() != 1) {
			qWarning() << "illegal image depth or channel for mask: " << mMask.depth();
			return false;
		}

		BinarizationSuAdapted binarizeImg(mSrcImg, mMask);
		binarizeImg.compute();
		mBwImg = binarizeImg.binaryImage();
		if (mPreFilter)
			mBwImg = IP::preFilterArea(mBwImg, preFilterArea);

		return true;
	}

	bool LineProcessing::lineTrace() 	{

		cv::Mat imgLine;
		rdf::PageXmlParser parser;

		if (!load(imgLine)) {
			qWarning() << "could not load image for line processing ... ";
			return false;
		}

		cv::Mat imgLineG = imgLine;

		if (imgLine.channels() != 1)
			cv::cvtColor(imgLine, imgLineG, CV_RGB2GRAY);
		else {
			cv::cvtColor(imgLine, imgLine, CV_GRAY2RGB);
		}

		//calculate lines
	
		if (mBwImg.empty()) {
			//cv::normalize(imgLineG, imgimgLineG 255, 0, cv::NORM_MINMAX);
			mSrcImg = imgLineG;

			if (!computeBinaryInput()) {
				qWarning() << "binary image was not set and could not be calculated";
				return false;
			}

			qDebug() << "binary image was not set - was calculated";
		}

		
		if (mEstimateSkew) {
			BaseSkewEstimation skewE(imgLineG, mMask);
			skewE.compute();
			mPageAngle = skewE.getAngle();
		}
		//rdf::Image::save(mBwImg, "C:\\tmp\\test2.png");
		//compute Lines
		LineTrace lt(mBwImg, mMask);
		lt.setAngle(mPageAngle);

		lt.compute();
		//mBwImg = lt.lineImage();
		mLines = lt.getLines();
		//rdf::Image::save(mBwImg, "C:\\tmp\\test3.png");
		//mHorLines = lt.getHLines();
		//mVerLines = lt.getVLines();

		//Save output to xml
		QString loadXmlPath = rdf::PageXmlParser::imagePathToXmlPath(mConfig.imagePath());

		if (QFileInfo(mConfig.xmlPath()).exists())
			loadXmlPath = mConfig.xmlPath();

		rdf::PageXmlParser parserOut;
		bool newXML = parserOut.read(loadXmlPath);
		auto pe = parserOut.page();

		if (!newXML) {
			//xml is newly created
			cv::Size imgSize = imgLineG.size();
			QSize imgQtSize;
			imgQtSize.setWidth(imgLineG.cols);
			imgQtSize.setHeight(imgLineG.rows);

			pe->setImageFileName(mConfig.imagePath());
			pe->setImageSize(imgQtSize);
			pe->setCreator("CVL");
			pe->setDateCreated(QDateTime::currentDateTime());
		}

		//add Lines
		for (int i = 0; i < mLines.size(); i++) {
			QSharedPointer<rdf::SeparatorRegion> pSepR(new rdf::SeparatorRegion());
			pSepR->setLine(mLines[i].qLine());

			pe->rootRegion()->addUniqueChild(pSepR);
		}


		//save pageXml
		parserOut.write(loadXmlPath, pe);

		return true;

	}

	bool LineProcessing::load(cv::Mat & img) const 	{

		QImage qImg = Image::load(mConfig.imagePath());

		if (qImg.isNull()) {
			qWarning() << "could not load image from" << mConfig.imagePath();
			return false;
		}

		// convert image
		img = Image::qImage2Mat(qImg);

		return true;
	}

	bool LineProcessing::load(rdf::PageXmlParser & parser) const 	{
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