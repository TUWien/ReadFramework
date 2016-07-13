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
	int idx = 0;

	qDebug() << "image path: " << mConfig.imagePath();

	QImage img(mConfig.imagePath());
	cv::Mat imgCv = Image::instance().qImage2Mat(img);

	rdf::BinarizationSuAdapted binarizeImg(imgCv, cv::Mat());
	binarizeImg.compute();

	//SuperPixel sp(imgCv);
	//sp.compute();
	//
	//if (!mConfig.outputPath().isEmpty())
	//	cv::imwrite(mConfig.outputPath().toStdString(), sp.binaryImage());

	// test lines XML
	cv::Mat bwImg = binarizeImg.binaryImage();

	rdf::LineTrace lt(bwImg);
	lt.setAngle(0.0);

	lt.compute();

	QVector<rdf::Line> alllines = lt.getLines();

	// init parser
	PageXmlParser parser;
	parser.read(mConfig.xmlPath());

	auto root = parser.page()->rootRegion();

	// test writing lines
	for (int i = 0; i < alllines.size(); i++) {

		QSharedPointer<rdf::SeparatorRegion> pSepR(new rdf::SeparatorRegion());
		pSepR->setLine(alllines[i].line());

		root->addUniqueChild(pSepR);
	}

	// loop for sampling time...
	parser.write(PageXmlParser::imagePathToXmlPath(mConfig.outputPath()), parser.page());

	//// test lines XML
	//rdf::BinarizationSuAdapted binarizeImg(imgCv);
	//binarizeImg.compute();
	//cv::Mat bwImg = binarizeImg.binaryImage();

	//rdf::LineTrace lt(bwImg);
	//lt.setAngle(0.0);

	//lt.compute();

	////cv::Mat lImg = lt.lineImage();
	////cv::Mat synLine = lt.generatedLineImage();
	////QVector<rdf::Line> hlines = lt.getHLines();
	////QVector<rdf::Line> vlines = lt.getVLines();
	//QVector<rdf::Line> alllines = lt.getLines();

	////save lines to xml
	////TODO - check if it is working...
	//rdf::PageXmlParser parser;
	//parser.read(mConfig.xmlPath());
	//auto pe = parser.page();
	//auto root = pe->rootRegion();
	////pe->setCreator(QString("CVL"));

	//for (int i = 0; i < alllines.size(); i++) {

	//	QSharedPointer<rdf::SeparatorRegion> pSepR(new rdf::SeparatorRegion());
	//	pSepR->setLine(alllines[i].line());

	//	if (root)
	//		root->addUniqueChild(pSepR);
	//}

	//parser.write(PageXmlParser::imagePathToXmlPath(mConfig.outputPath()), pe);


	qDebug() << mConfig.xmlPath() << "parsed" << idx << "x in " << dt << "mean" << dt.stringifyTime(dt.elapsed());

}

}