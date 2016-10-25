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

#pragma warning(push, 0)	// no warnings from includes
#include <QDebug>
#include <QImage>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#pragma warning(pop)

namespace rdf {

XmlTest::XmlTest(const DebugConfig& config) {

	mConfig = config;
}

void XmlTest::parseXml() {

	rdf::Timer dt;

	// test image loading
	QImage img(mConfig.imagePath());
	cv::Mat imgCv = Image::instance().qImage2Mat(img);

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
	cv::Mat imgCv = Image::instance().qImage2Mat(img);

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

	rdf::Timer dt;

	qDebug() << "image path: " << mConfig.imagePath();

	// load image
	QImage img(mConfig.imagePath());
	cv::Mat imgCv = Image::instance().qImage2Mat(img);

	computeComponents(imgCv);

	qInfo() << "total computation time:" << dt;
}

void LayoutTest::computeComponents(const cv::Mat & src) const {

	cv::Mat img = src.clone();
	//cv::resize(src, img, cv::Size(), 0.25, 0.25, CV_INTER_AREA);

	Timer dt;

	// find super pixels
	rdf::SuperPixel superPixel(img);
	
	if (!superPixel.compute())
		qWarning() << "could not compute super pixel!";

	QVector<QSharedPointer<Pixel> > sp = superPixel.getSuperPixels();

	// find local orientation per pixel
	rdf::LocalOrientation lo(sp);
	if (!lo.compute())
		qWarning() << "could not compute local orientation";

	// smooth estimation
	rdf::GraphCutOrientation pse(sp, Rect(Vector2D(), Vector2D(img.size())));
	
	if (!pse.compute())
		qWarning() << "could not compute set orientation";

	// find tab stops
	rdf::TabStopAnalysis tabStops(sp);
	if (!tabStops.compute())
		qWarning() << "could not compute text block segmentation!";

	// find text lines
	rdf::TextLineSegmentation textLines(Rect(img), sp);
	textLines.addLines(tabStops.tabStopLines(30));	// TODO: fix parameter
	if (!textLines.compute())
		qWarning() << "could not compute text block segmentation!";

	qInfo() << "algorithm computation time" << dt;

	// drawing
	//cv::Mat rImg(img.rows, img.cols, CV_8UC1, cv::Scalar::all(150));
	cv::Mat rImg = img.clone();

	//// draw edges
	//rImg = textBlocks.draw(rImg);
	//rImg = lo.draw(rImg, "1012", 256);
	//rImg = lo.draw(rImg, "507", 128);
	//rImg = lo.draw(rImg, "507", 64);

	//// save super pixel image
	//rImg = superPixel.drawSuperPixels(rImg);
	//rImg = tabStops.draw(rImg);
	rImg = textLines.draw(rImg);
	QString maskPath = rdf::Utils::instance().createFilePath(mConfig.outputPath(), "-tabStops");
	rdf::Image::instance().save(rImg, maskPath);
	qDebug() << "results written to" << maskPath;
}

}