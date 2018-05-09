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


#pragma warning(push, 0)	// no warnings from includes
#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QImage>
#include <QFileInfo>
#pragma warning(pop)

#include "Utils.h"
#include "Settings.h"
#include "DebugUtils.h"
#include "DebugMarkus.h"
#include "DebugFlo.h"
#include "DebugStefan.h"
#include "DebugThomas.h"
#include "PageParser.h"
#include "Shapes.h"

#if defined(_MSC_BUILD) && !defined(QT_NO_DEBUG_OUTPUT) // fixes cmake bug - really release uses subsystem windows, debug and release subsystem console
#pragma comment (linker, "/SUBSYSTEM:CONSOLE")
#else
#pragma comment (linker, "/SUBSYSTEM:WINDOWS")
#endif

void applyDebugSettings(rdf::DebugConfig& dc);
bool testFunction();

int main(int argc, char** argv) {

	// check opencv version
	qInfo().nospace() << "I am using OpenCV " << CV_MAJOR_VERSION << "." << CV_MINOR_VERSION << "." << CV_VERSION_REVISION;

	QCoreApplication::setOrganizationName("TU Wien");
	QCoreApplication::setOrganizationDomain("http://www.cvl.tuwien.ac.at/cvl");
	QCoreApplication::setApplicationName("READ Framework");
	rdf::Utils::instance().initFramework();

#ifdef WIN32
	QApplication app(argc, (char**)argv);		// enable QPainter
#else
	QCoreApplication app(argc, (char**)argv);	// enable headless
#endif

	// CMD parser --------------------------------------------------------------------
	QCommandLineParser parser;


	parser.setApplicationDescription("Welcome to the CVL READ Framework testing application.");
	parser.addHelpOption();
	parser.addVersionOption();
	parser.addPositionalArgument("imagepath", QObject::tr("Path to an input image."));

	// xml path
	QCommandLineOption xmlOpt(QStringList() << "x" << "xml", QObject::tr("Path to PAGE xml. If provided, we make use of the information"), "path");
	parser.addOption(xmlOpt);

	// output image
	QCommandLineOption outputOpt(QStringList() << "o" << "output", QObject::tr("Path to output image."), "path");
	parser.addOption(outputOpt);

	// developer
	QCommandLineOption modeOpt(QStringList() << "m" << "mode", QObject::tr("Mode defines the methodology. For Baseline detection use [-m layout]"), "name");
	parser.addOption(modeOpt);

	// settings filename
	QCommandLineOption settingOpt(QStringList() << "s" << "setting", QObject::tr("Settings filepath."), "filepath");
	parser.addOption(settingOpt);

	// settings classifier
	QCommandLineOption classifierOpt(QStringList() << "c" << "classifier", QObject::tr("Classifier file path."), "filepath");
	parser.addOption(classifierOpt);

	// feature cache path
	QCommandLineOption featureCachePathOpt(QStringList() << "f" << "feature cache", QObject::tr("Feature cache path for training."), "filepath");
	parser.addOption(featureCachePathOpt);

	// label config path
	QCommandLineOption labelConfigPathOpt(QStringList() << "l" << "label config", QObject::tr("Label config path for training."), "filepath");
	parser.addOption(labelConfigPathOpt);

	// table template path
	QCommandLineOption xmlTableOpt(QStringList() << "t" << "table template", QObject::tr("Path to PAGE xml of table template. Table must be specified"), "templatepath");
	parser.addOption(xmlTableOpt);


	parser.process(*QCoreApplication::instance());
	// CMD parser --------------------------------------------------------------------

	// stop processing if little tests are preformed
	if (testFunction())
		return 0;

	// load settings
	rdf::Config& config = rdf::Config::instance();
	
	// load user defined settings
	if (parser.isSet(settingOpt)) {
		QString sName = parser.value(settingOpt);
		config.setSettingsFile(sName);
		config.load();
	}

	// create debug config
	rdf::DebugConfig dc;

	if (parser.positionalArguments().size() > 0)
		dc.setImagePath(parser.positionalArguments()[0].trimmed());

	// add output path	
	if (parser.isSet(outputOpt))
		dc.setOutputPath(parser.value(outputOpt));

	// add xml path	
	if (parser.isSet(xmlOpt))
		dc.setXmlPath(parser.value(xmlOpt));

	// add classifier path	
	if (parser.isSet(classifierOpt))
		dc.setClassifierPath(parser.value(classifierOpt));

	// add feature cache path
	if (parser.isSet(featureCachePathOpt))
		dc.setFeatureCachePath(parser.value(featureCachePathOpt));

	// add label config path
	if (parser.isSet(labelConfigPathOpt))
		dc.setLabelConfigPath(parser.value(labelConfigPathOpt));

	// add table template
	if (parser.isSet(xmlTableOpt))
		dc.setTableTemplate(parser.value(xmlTableOpt));

	// apply debug settings - convenience if you don't want to always change the cmd args
	applyDebugSettings(dc);

	if (!dc.imagePath().isEmpty()) {

		// flos section
		if (parser.isSet(modeOpt) && parser.value(modeOpt) == "binarization") {
			// TODO do what ever you want
			qDebug() << "starting binarization ...";
			rdf::BinarizationTest test(dc);
			test.binarizeTest();
		}
		else if (parser.isSet(modeOpt) && parser.value(modeOpt) == "table") {
			qDebug() << "starting table matching ... (not yet)";
			//TODO table
			rdf::TableProcessing tableproc(dc);
			tableproc.match();

		}

		// stefans section
		else if (parser.isSet(modeOpt) && parser.value(modeOpt) == "stefan") {
			qDebug() << "loading stefan's debug code";

			rdf::TestWriterRetrieval twr = rdf::TestWriterRetrieval();
			twr.run();
		}
		// layout section
		else if (parser.isSet(modeOpt) && parser.value(modeOpt) == "layout") {
			qDebug() << "Starting layout analysis...";

			rdf::LayoutTest lt(dc);
			lt.layoutToXml();
		}
		// thomas
		else if (parser.isSet(modeOpt) && parser.value(modeOpt) == "thomas") {
			qDebug() << "thomas";
			rdf::ThomasTest test(dc);
			test.test();
		}
		// my section
		else {
			//rdf::XmlTest test(dc);
			//test.parseXml();
			//test.linesToXml();

			rdf::LayoutTest lt(dc);
			lt.testComponents();
		}

	}
	else {
		qInfo() << "Please specify an input image...";
		parser.showHelp();
	}

	// save settings
	config.save();
	return 0;	// thanks
}

void applyDebugSettings(rdf::DebugConfig& dc) {

	if (dc.imagePath().isEmpty()) {

		dc.setImagePath("C:/read/test/sizes/synthetic-test-small.png");
		dc.setImagePath("C:/read/test/sizes/synthetic-test.png");
		dc.setImagePath("C:/read/test/d6.5/0056_S_Alzgern_011-01_0056-crop.JPG");
		dc.setImagePath("C:/read/baseline-evaluation/BL_English/Images/ior!p!241!37_8_feb_1793_pp_594-610_f001v.jpg");
		
		//dc.setImagePath("C:/temp/chris/test2.png");


		qInfo() << dc.imagePath() << "added as image path";
	}

	if (dc.outputPath().isEmpty()) {
		dc.setOutputPath(rdf::Utils::createFilePath(dc.imagePath(), "-result", "tif"));
		qInfo() << dc.outputPath() << "added as output path";
	}

	if (dc.classifierPath().isEmpty()) {
		dc.setClassifierPath("C:/read/configs/test/test-two-classes/test-model.json");
		qInfo() << dc.classifierPath() << "added as classifier path";
	} 

	if (dc.labelConfigPath().isEmpty()) {
		dc.setLabelConfigPath("C:/read/configs/test/test-two-classes/test-config.json");
		qInfo() << dc.labelConfigPath() << "added as label config path";
	} 

	if (dc.featureCachePath().isEmpty()) {
		dc.setFeatureCachePath("C:/read/configs/test/test-two-classes/test-features.json");
		qInfo() << dc.featureCachePath() << "added as feature cache path";
	} 

	if (dc.xmlPath().isEmpty()) {
		QString xmlPath = rdf::PageXmlParser::imagePathToXmlPath(dc.imagePath());
		dc.setXmlPath(xmlPath);		// overwrite
		//dc.setXmlPath(rdf::Utils::createFilePath(xmlPath, "-result"));

		//dc.setXmlPath("C:/temp/T_Aigen_am_Inn_001_0056.xml");
		qInfo() << dc.xmlPath() << "added as XML path";
	} 

	// add your debug overwrites here...
}

bool testFunction() {

	// tests the line distance to point function
	//rdf::Vector2D l1(1916, 1427);
	//rdf::Vector2D l2(1931, 1859);
	//rdf::Vector2D l3(1915, 834);
	//rdf::Vector2D l4(1952, 3846);

	//rdf::Line l(l3,l4);

	//qDebug() << l.distance(l2) << "is the distance";

	return false;
}