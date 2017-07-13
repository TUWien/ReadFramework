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

#include "BaselineTest.h"

#include "Image.h"
#include "Utils.h"
#include "PageParser.h"
#include "Elements.h"
#include "LayoutAnalysis.h"
#include "Settings.h"

#pragma warning(push, 0)	// no warnings from includes
#include <QImage>
#pragma warning(pop)

namespace rdf {

BaselineTest::BaselineTest(const TestConfig & config) : mConfig(config) {
}

bool BaselineTest::baselineTest() const {

	QImage img = Image::load(mConfig.imagePath());

	if (img.isNull()) {
		qWarning() << "could not load image from" << mConfig.imagePath();
		return false;
	}

	Timer dt;

	// convert image
	cv::Mat imgCv = Image::qImage2Mat(img);

	// load XML
	rdf::PageXmlParser parser;
	parser.read(mConfig.xmlPath());
	
	// fail if the XML was not loaded
	if (parser.loadStatus() != PageXmlParser::status_ok) {
		qWarning() << "could not load XML from" << mConfig.xmlPath();
		return false;
	}

	// test the layout module
	layoutToXml(imgCv, parser);

	//eval();

	qInfo() << "total computation time:" << dt;

	return true;
}

bool BaselineTest::layoutToXml(const cv::Mat& img, const PageXmlParser& parser) const {

	Timer dt;

	auto pe = parser.page();

	// compute without xml data
	rdf::LayoutAnalysis la(img);

	if (!la.compute()) {
		qWarning() << "could not compute layout analysis";
		return false;
	}

	auto tbs = la.textBlockSet();
	auto sl = la.stopLines();

	// test layout with xml
	rdf::LayoutAnalysis laXml(img);
	laXml.setRootRegion(pe->rootRegion());
	laXml.config()->saveDefaultSettings(Config::instance().settings());	// save default layout settings

	if (!laXml.compute()) {
		qWarning() << "could not compute layout analysis";
		return false;
	}

	// cannot test drawing: it's headless
	//// test drawing
	//cv::Mat rImg = img.clone();
	//rImg = laXml.draw(rImg);

	// check layout analysis with empty image
	cv::Mat emptyImg;
	rdf::LayoutAnalysis lae(emptyImg);

	if (lae.compute()) {
		qWarning() << "layout with empty image returned true in compute";
		return false;
	}

	qInfo() << "layout analysis computed in" << dt;
	return true;
}

}