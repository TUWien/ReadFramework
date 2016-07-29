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

#include "SuperPixel.h"

#pragma warning(push, 0)	// no warnings from includes
#include <QDebug>
#include <QImage>
#include <opencv2/highgui/highgui.hpp>
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

	//// binarize
	//rdf::BinarizationSuAdapted binarizeImg(imgCv, cv::Mat());
	//binarizeImg.compute();
	//cv::Mat bwImg = binarizeImg.binaryImage();
	//qInfo() << "binarised in" << dt;

	computeComponents(imgCv);

	qInfo() << "components found in" << dt;
}

void LayoutTest::computeComponents(cv::Mat & img) const {

	rdf::SuperPixel sp(img);
	
	if (!sp.compute())
		qWarning() << "could not compute super pixel!";

	cv::Mat mask = sp.binaryImage();

	// save mask
	QString maskPath = rdf::Utils::instance().createFilePath(mConfig.outputPath(), "-mask");
	rdf::Image::instance().save(mask, maskPath);

}

}