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

#include "DebugMarkus.h"
#include "PageParser.h"
#include "Utils.h"
#include "Image.h"
#include "Binarization.h"
#include "LineTrace.h"
#include "Elements.h"
#include "Settings.h"

#include "SuperPixel.h"
#include "TabStopAnalysis.h"
#include "TextLineSegmentation.h"
#include "PageSegmentation.h"
#include "SuperPixelClassification.h"
#include "SuperPixelTrainer.h"
#include "LayoutAnalysis.h"

#pragma warning(push, 0)	// no warnings from includes
#include <QDebug>
#include <QImage>
#include <QFileInfo>

#include <QJsonObject>

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/ml/ml.hpp>
#pragma warning(pop)

namespace rdf {

XmlTest::XmlTest(const DebugConfig& config) {

	mConfig = config;
}

void XmlTest::parseXml() {

	rdf::Timer dt;

	// test image loading
	QImage img(mConfig.imagePath());
	cv::Mat imgCv = Image::qImage2Mat(img);

	if (!imgCv.empty())
		qInfo() << mConfig.imagePath() << "loaded...";
	else
		qInfo() << mConfig.imagePath() << "NOT loaded...";

	// parse xml
	PageXmlParser parser;
	parser.read(mConfig.xmlPath());

	qDebug() << mConfig.xmlPath() << "parsed in " << dt;
}

void XmlTest::linesToXml() {

	rdf::Timer dt;

	qDebug() << "image path: " << mConfig.imagePath();

	// load image
	QImage img(mConfig.imagePath());
	cv::Mat imgCv = Image::qImage2Mat(img);

	// binarize
	rdf::BinarizationSuAdapted binarizeImg(imgCv, cv::Mat());
	binarizeImg.compute();
	cv::Mat bwImg = binarizeImg.binaryImage();
	qInfo() << "binarised in" << dt;

	// find lines
	rdf::LineTrace lt(bwImg);
	lt.setAngle(0.0);
	lt.compute();
	QVector<rdf::Line> allLines = lt.getLines();
	qInfo() << allLines.size() << "lines detected in" << dt;

	// init parser
	PageXmlParser parser;
	parser.read(mConfig.xmlPath());

	auto root = parser.page()->rootRegion();

	// test writing lines
	for (const rdf::Line& cL : allLines) {

		QSharedPointer<rdf::SeparatorRegion> pSepR(new rdf::SeparatorRegion());
		pSepR->setLine(cL.line());
		root->addUniqueChild(pSepR);
	}

	parser.write(PageXmlParser::imagePathToXmlPath(mConfig.outputPath()), parser.page());
	
	qInfo() << mConfig.imagePath() << "lines computed and written in" << dt;
}

// Layout Test --------------------------------------------------------------------
LayoutTest::LayoutTest(const DebugConfig & config) {
	mConfig = config;
}

void LayoutTest::testComponents() {

	Timer dt;

	// test image loading
	QImage img(mConfig.imagePath());
	cv::Mat imgCv = Image::qImage2Mat(img);

	if (!imgCv.empty())
		qInfo() << mConfig.imagePath() << "loaded...";
	else
		qInfo() << mConfig.imagePath() << "NOT loaded...";

	// switch tests
	//testFeatureCollector(imgCv);
	//testTrainer();
	//pageSegmentation(imgCv);
	//testLayout(imgCv);
	layoutToXml();

	qInfo() << "total computation time:" << dt;
}

void LayoutTest::layoutToXml() const {

	QImage imgQt(mConfig.imagePath());
	cv::Mat img = Image::qImage2Mat(imgQt);

	Timer dt;
	QString loadXmlPath = rdf::PageXmlParser::imagePathToXmlPath(mConfig.imagePath());

	rdf::PageXmlParser parser;
	parser.read(loadXmlPath);
	auto pe = parser.page();

	rdf::LayoutAnalysis la(img);
	la.setTextRegions(Region::allRegions(pe->rootRegion()));

	if (!la.compute())
		qWarning() << "could not compute layout analysis";

	// drawing --------------------------------------------------------------------
	cv::Mat rImg = img.clone();

	// save super pixel image
	//rImg = superPixel.drawSuperPixels(rImg);
	//rImg = tabStops.draw(rImg);
	rImg = la.draw(rImg);
	QString dstPath = rdf::Utils::instance().createFilePath(mConfig.outputPath(), "-textlines");
	rdf::Image::save(rImg, dstPath);
	qDebug() << "debug image saved: " << dstPath;

	// write to XML --------------------------------------------------------------------
	pe->setCreator(QString("CVL"));
	pe->setImageSize(QSize(img.rows, img.cols));
	pe->setImageFileName(QFileInfo(mConfig.imagePath()).fileName());

	pe->setRootRegion(la.textBlockSet().toTextRegion());

	parser.write(mConfig.xmlPath(), pe);
	qDebug() << "results written to" << mConfig.xmlPath();

	qInfo() << "layout analysis computed in" << dt;
}

void LayoutTest::testFeatureCollector(const cv::Mat & src) const {
	
	rdf::Timer dt;

	// parse xml
	PageXmlParser parser;
	parser.read(mConfig.xmlPath());

	// test loading of label lookup
	LabelManager lm = LabelManager::read(mConfig.labelConfigPath());
	qInfo().noquote() << lm.toString();

	// compute super pixels
	SuperPixel sp(src);

	if (!sp.compute())
		qCritical() << "could not compute super pixels!";

	// feed the label lookup
	SuperPixelLabeler spl(sp.getMserBlobs(), Rect(src));
	spl.setLabelManager(lm);
	spl.setFilePath(mConfig.imagePath());

	// set the ground truth
	if (parser.page())
		spl.setRootRegion(parser.page()->rootRegion());

	if (!spl.compute())
		qCritical() << "could not compute SuperPixel labeling!";

	SuperPixelFeature spf(src, spl.set());
	if (!spf.compute())
		qCritical() << "could not compute SuperPixel features!";

	FeatureCollectionManager fcm(spf.features(), spf.set());
	fcm.write(mConfig.featureCachePath());
	
	FeatureCollectionManager testFcm = FeatureCollectionManager::read(mConfig.featureCachePath());

	for (int idx = 0; idx < testFcm.collection().size(); idx++) {

		if (testFcm.collection()[idx].label() != fcm.collection()[idx].label())
			qWarning() << "wrong labels!" << testFcm.collection()[idx].label() << "vs" << fcm.collection()[idx].label();
		else
			qInfo() << testFcm.collection()[idx].label() << "is fine...";
	}

	// drawing
	cv::Mat rImg = src.clone();

	// save super pixel image
	//rImg = superPixel.drawSuperPixels(rImg);
	//rImg = tabStops.draw(rImg);
	rImg = spl.draw(rImg);
	rImg = spf.draw(rImg);
	QString dstPath = rdf::Utils::instance().createFilePath(mConfig.outputPath(), "-textlines");
	rdf::Image::save(rImg, dstPath);
	qDebug() << "debug image saved: " << dstPath;

	qDebug() << "image path: " << mConfig.imagePath();

}

void LayoutTest::testTrainer() {

	//cv::Mat testM(10, 10, CV_8UC1);
	//
	//for (int rIdx = 0; rIdx < testM.rows; rIdx++) {
	//	unsigned char* ptr = testM.ptr<unsigned char>(rIdx);
	//	for (int cIdx = 0; cIdx < testM.cols; cIdx++) {
	//		ptr[cIdx] = cIdx*rIdx+cIdx;
	//	}
	//}
	//
	//QJsonObject jo = Image::matToJson(testM);
	//cv::Mat t2 = Image::jsonToMat(jo);

	//cv::Scalar s = cv::sum(cv::abs(testM - t2));
	//if (s[0] != 0)
	//	qWarning() << "inconsistent json2Mat I/O";
	//else
	//	qInfo() << "json to mat is just fine...";

	
	Timer dt;
	FeatureCollectionManager fcm = FeatureCollectionManager::read(mConfig.featureCachePath());
	

	// train classifier
	SuperPixelTrainer spt(fcm);

	if (!spt.compute())
		qCritical() << "could not train data...";

	spt.write(mConfig.classifierPath());
	
	// read back the model
	QSharedPointer<SuperPixelModel> model = SuperPixelModel::read(mConfig.classifierPath());

	auto f = model->model();
	if (f->isTrained())
		qDebug() << "the classifier I loaded is trained...";
	
	//qDebug() << fcm.numFeatures() << "SuperPixels trained in" << dt;
}

void LayoutTest::testLayout(const cv::Mat & src) const {

	// TODOS
	// - line spacing needs smoothing -> graphcut
	// - DBScan is very sensitive to the line spacing
	
	// Workflow:
	// - implement noise/text etc classification on SuperPixel level
	// - smooth labels using graphcut
	// - perform everything else without noise pixels
	// Training:
	// - open mode (whole image only contains e.g. machine printed)
	// - baseline mode -> overlap with superpixel

	cv::Mat img = src.clone();
	//cv::resize(src, img, cv::Size(), 0.25, 0.25, CV_INTER_AREA);

	Timer dt;

	// find super pixels
	//rdf::SuperPixel superPixel(img);
	rdf::ScaleSpaceSuperPixel superPixel(img);
	
	if (!superPixel.compute())
		qWarning() << "could not compute super pixel!";

	PixelSet sp = superPixel.superPixels();

	// find local orientation per pixel
	rdf::LocalOrientation lo(sp.pixels());
	if (!lo.compute())
		qWarning() << "could not compute local orientation";

	// smooth estimation
	rdf::GraphCutOrientation pse(sp.pixels());
	
	if (!pse.compute())
		qWarning() << "could not compute set orientation";
	
	// drawing
	cv::Mat nImg = img.clone();

	//// draw edges
	//rImg = textBlocks.draw(rImg);
	//// save super pixel image
	nImg = superPixel.draw(nImg);
	//rImg = tabStops.draw(rImg);
	//rImg = spc.draw(rImg);
	QString imgPathN = rdf::Utils::instance().createFilePath(mConfig.outputPath(), "-nomacs", "png");
	rdf::Image::save(nImg, imgPathN);
	qDebug() << "debug image added" << imgPathN;

	// pixel labeling
	QSharedPointer<SuperPixelModel> model = SuperPixelModel::read(mConfig.classifierPath());

	SuperPixelClassifier spc(src, sp);
	spc.setModel(model);

	if (!spc.compute())
		qWarning() << "could not classify SuperPixels";

	//// find tab stops
	//rdf::TabStopAnalysis tabStops(sp);
	//if (!tabStops.compute())
	//	qWarning() << "could not compute text block segmentation!";


	// find text lines
	rdf::TextLineSegmentation textLines(sp.pixels());
	//textLines.addLines(tabStops.tabStopLines(30));	// TODO: fix parameter

	if (!textLines.compute(img))
		qWarning() << "could not compute text line segmentation!";

	qInfo() << "algorithm computation time" << dt;

	// drawing
	cv::Mat rImg = img.clone();
	rImg = textLines.draw(rImg);

	//// draw edges
	//rImg = textBlocks.draw(rImg);
	//// save super pixel image
	//rImg = superPixel.draw(rImg);
	//rImg = tabStops.draw(rImg);
	rImg = spc.draw(rImg);
	rImg = textLines.draw(rImg);
	QString imgPath = rdf::Utils::instance().createFilePath(mConfig.outputPath(), "-tlc");
	rdf::Image::save(rImg, imgPath);
	qDebug() << "debug image added" << imgPath;

	// draw a second image ---------------------------------
	rImg = img.clone();

	rImg = spc.draw(rImg);
	imgPath = rdf::Utils::instance().createFilePath(mConfig.outputPath(), "-sp");
	rdf::Image::save(rImg, imgPath);
	qDebug() << "debug image added" << imgPath;

	//// write XML -----------------------------------
	//QString loadXmlPath = rdf::PageXmlParser::imagePathToXmlPath(mConfig.imagePath());

	//rdf::PageXmlParser parser;
	//parser.read(loadXmlPath);
	//auto pe = parser.page();
	//pe->setCreator(QString("CVL"));
	//pe->setImageSize(QSize(img.rows, img.cols));
	//pe->setImageFileName(QFileInfo(mConfig.imagePath()).fileName());

	//// start writing content
	//auto ps = PixelSet::fromEdges(PixelSet::connect(sp, Rect(0, 0, img.cols, img.rows)));

	//if (!ps.empty()) {
	//	QSharedPointer<Region> textRegion = QSharedPointer<Region>(new Region());
	//	textRegion->setType(Region::type_text_region);
	//	textRegion->setPolygon(ps[0]->convexHull());
	//	
	//	for (auto tl : textLines.textLines()) {
	//		textRegion->addUniqueChild(tl);
	//	}

	//	pe->rootRegion()->addUniqueChild(textRegion);
	//}

	//parser.write(mConfig.xmlPath(), pe);
	//qDebug() << "results written to" << mConfig.xmlPath();

}

void LayoutTest::pageSegmentation(const cv::Mat & src) const {

	// TODOS
	// - line spacing needs smoothing -> graphcut
	// - DBScan is very sensitive to the line spacing

	// Workflow:
	// - implement noise/text etc classification on SuperPixel level
	// - smooth labels using graphcut
	// - perform everything else without noise pixels
	// Training:
	// - open mode (whole image only contains e.g. machine printed)
	// - baseline mode -> overlap with superpixel

	cv::Mat img = src.clone();
	//cv::resize(src, img, cv::Size(), 0.25, 0.25, CV_INTER_AREA);

	Timer dt;

	// find super pixels
	rdf::PageSegmentation pageSeg(img);

	if (!pageSeg.compute())
		qWarning() << "could not compute page segmentation!";

	qInfo() << "algorithm computation time" << dt;

	// drawing
	cv::Mat rImg = img.clone();

	// save super pixel image
	rImg = pageSeg.draw(rImg);
	QString maskPath = rdf::Utils::instance().createFilePath(mConfig.outputPath(), "-page-seg");
	rdf::Image::save(rImg, maskPath);
	qDebug() << "debug image added" << maskPath;
}

}