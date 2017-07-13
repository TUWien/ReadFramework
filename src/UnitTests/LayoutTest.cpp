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

#include "LayoutTest.h"

#include "Image.h"
#include "Utils.h"
#include "PageParser.h"
#include "Elements.h"
#include "LayoutAnalysis.h"
#include "Settings.h"
#include "SuperPixel.h"
#include "SuperPixelClassification.h"
#include "SuperPixelTrainer.h"
#include "SuperPixelScaleSpace.h"

#pragma warning(push, 0)	// no warnings from includes
#include <QImage>

#include <opencv2/ml/ml.hpp>
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

// -------------------------------------------------------------------- SuperPixelTest 
SuperPixelTest::SuperPixelTest(const TestConfig & config) : mConfig(config) {
}

bool SuperPixelTest::testSuperPixel() const {
	
	QImage img = Image::load(mConfig.imagePath());

	if (img.isNull()) {
		qWarning() << "could not load image from" << mConfig.imagePath();
		return false;
	}

	Timer dt;

	// convert image
	cv::Mat src = Image::qImage2Mat(img);

	// default (MSER) super pixels
	rdf::SuperPixel sp(src);
	if (!sp.compute()) {
		qWarning() << "cannot compute SuperPixels";
		return false;
	}

	// line super pixels
	rdf::LineSuperPixel lsp(src);
	if (!lsp.compute()) {
		qWarning() << "cannot compute SuperPixels";
		return false;
	}

	// grid super pixels (with scale space)
	rdf::ScaleSpaceSuperPixel<rdf::GridSuperPixel> gsp(src);
	if (!gsp.compute()) {
		qWarning() << "cannot compute SuperPixels";
		return false;
	}

	return true;
}

bool SuperPixelTest::collectFeatures() const {
	
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

	// test loading of label lookup
	rdf::LabelManager lm = rdf::LabelManager::read(mConfig.labelConfigPath());
	qInfo().noquote() << lm.toString();

	// compute super pixels
	rdf::SuperPixel sp(imgCv);

	if (!sp.compute()) {
		qCritical() << "could not compute super pixels!";
		return false;
	}

	// feed the label lookup
	rdf::SuperPixelLabeler spl(sp.getMserBlobs(), rdf::Rect(imgCv));
	spl.setLabelManager(lm);
	spl.setFilePath(mConfig.imagePath());	// parse filepath for gt
	
	// set the ground truth
	if (parser.page())
		spl.setRootRegion(parser.page()->rootRegion());


	if (!spl.compute()) {
		qCritical() << "could not compute SuperPixel labeling!";
		return false;
	}

	rdf::SuperPixelFeature spf(imgCv, spl.set());
	if (!spf.compute()) {
		qCritical() << "could not compute SuperPixel features!";
		return false;
	}

	rdf::FeatureCollectionManager fcm(spf.features(), spf.set());
	fcm.write(mConfig.featureCachePath());

	qDebug() << "feature collection takes" << dt;
	return true;
}

bool SuperPixelTest::train() const {

	rdf::FeatureCollectionManager fcm;

	const QString& fPath = mConfig.featureCachePath();
	rdf::FeatureCollectionManager cFc = rdf::FeatureCollectionManager::read(fPath);
	fcm.merge(cFc);

	// train classifier
	rdf::SuperPixelTrainer spt(fcm);

	if (!spt.compute()) {
		qCritical() << "could not train data...";
		return false;
	}

	spt.write(mConfig.classifierPath());

	// test - read back the model
	auto model = rdf::SuperPixelModel::read(mConfig.classifierPath());

	auto f = model->model();
	if (f && f->isTrained()) {
		qDebug() << "the classifier I loaded is trained...";
		return true;
	}

	qCritical() << "could not save classifier to:" << mConfig.classifierPath();
	return false;
}

}