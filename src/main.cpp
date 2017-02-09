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

	QCoreApplication::setOrganizationName("TU Wien");
	QCoreApplication::setOrganizationDomain("http://www.caa.tuwien.ac.at/cvl");
	QCoreApplication::setApplicationName("READ Framework");
	rdf::Utils::instance().initFramework();

	QApplication app(argc, (char**)argv);

	// CMD parser --------------------------------------------------------------------
	QCommandLineParser parser;

	parser.setApplicationDescription("READ Framework testing application.");
	parser.addHelpOption();
	parser.addVersionOption();
	parser.addPositionalArgument("image", QObject::tr("An input image."));

	// output image
	QCommandLineOption outputOpt(QStringList() << "o" << "output", QObject::tr("Path to output image."), "path");
	parser.addOption(outputOpt);

	// output image
	QCommandLineOption xmlOpt(QStringList() << "x" << "xml", QObject::tr("Path to PAGE xml."), "path");
	parser.addOption(xmlOpt);

	// output image
	QCommandLineOption devOpt(QStringList() << "d" << "developer", QObject::tr("Developer name."), "name");
	parser.addOption(devOpt);

	// settings filename
	QCommandLineOption settingOpt(QStringList() << "s" << "setting", QObject::tr("Settings filename."), "filename");
	parser.addOption(settingOpt);

	// settings classifier
	QCommandLineOption classifierOpt(QStringList() << "c" << "classifier", QObject::tr("Classifier file path."), "filepath");
	parser.addOption(classifierOpt);

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
	}
	config.load();

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

	// apply debug settings - convenience if you don't want to always change the cmd args
	applyDebugSettings(dc);

	if (!dc.imagePath().isEmpty()) {

		// flos section
		if (parser.isSet(devOpt) && parser.value(devOpt) == "flo") {
			// TODO do what ever you want
			qDebug() << "starting flos debug code ...";
			rdf::BinarizationTest test(dc);
			test.binarizeTest();
		}
		// stefans section
		else if (parser.isSet(devOpt) && parser.value(devOpt) == "stefan") {
			// TODO do what ever you want
			qDebug() << "loading stefan's debug code";

			rdf::TestWriterRetrieval twr = rdf::TestWriterRetrieval();
			twr.run();
		}
		// sebastians section
		else if (parser.isSet(devOpt) && parser.value(devOpt) == "sebastian") {
			qDebug() << "Starting layout analysis...";

			rdf::LayoutTest lt(dc);
			lt.layoutToXml();
		}
		// my section
		else {
			qDebug() << "Servus Markus...";
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
		//dc.setImagePath("D:/read/test/Best. 901 Nr. 112 00147.jpg");
		//dc.setImagePath("D:/read/test/M_Aigen_am_Inn_007_0336.jpg");
		dc.setImagePath("D:/read/test/00000197.jpg");
		dc.setImagePath("D:/read/test/M_Aigen_am_Inn_003-01_0001.jpg");
		//dc.setImagePath("D:/read/test/00075751.tif");
		//dc.setImagePath("D:/read/test/screenshot.png");
		//dc.setImagePath("D:/read/test/synthetic-test.png");
		//dc.setImagePath("D:/read/test/synthetic-test-single-line.png");
		//dc.setImagePath("D:/read/data/Herbarium/George_Forrest_Herbarium_Specimens/E00000017-c.jpg");
	
		//dc.setImagePath("D:/read/test/M_Aigen_am_Inn_007_0021.jpg");
		//dc.setImagePath("D:/read/test/56_csrc.jpg");
		//dc.setImagePath("D:/read/test/102_csrc.jpg");

		// debug images
		//dc.setImagePath("D:/read/test/two-lines-connected.jpg");
		//dc.setImagePath("D:/read/test/M_Aigen_am_Inn_007_0336-crop.jpg");
		//dc.setImagePath("D:/read/test/00075751-crop.tif");
		//dc.setImagePath("D:/read/test/M_Aigen_am_Inn_007_0084-crop.jpg");
		//dc.setImagePath("D:/read/test/M_Aigen_am_Inn_007_00842-crop.jpg");
		//dc.setImagePath("D:/read/test/debug-tiny[printed].png");
		//dc.setImagePath("D:/read/test/00000003.jpg");
	
		//dc.setImagePath("D:/read/test/sp-classification/00000158.tif");
		qInfo() << dc.imagePath() << "added as image path";
	}

	if (dc.outputPath().isEmpty()) {
		dc.setOutputPath(rdf::Utils::instance().createFilePath(dc.imagePath(), "-result", "tif"));
		qInfo() << dc.outputPath() << "added as output path";
	}

	if (dc.classifierPath().isEmpty()) {
		dc.setClassifierPath("D:/read/configs/model.json");
		qInfo() << dc.classifierPath() << "added as classifier path";
	} 

	if (dc.labelConfigPath().isEmpty()) {
		dc.setLabelConfigPath("D:/read/configs/config-prima.json");
		qInfo() << dc.labelConfigPath() << "added as label config path";
	} 

	if (dc.featureCachePath().isEmpty()) {
		dc.setFeatureCachePath("D:/read/configs/features-prima.json");
		qInfo() << dc.featureCachePath() << "added as feature cache path";
	} 

	if (dc.xmlPath().isEmpty()) {
		QString xmlPath = rdf::PageXmlParser::imagePathToXmlPath(dc.imagePath());
		dc.setXmlPath(rdf::Utils::instance().createFilePath(xmlPath, "-result"));
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