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

#if defined(_MSC_BUILD) && !defined(QT_NO_DEBUG_OUTPUT) // fixes cmake bug - really release uses subsystem windows, debug and release subsystem console
#pragma comment (linker, "/SUBSYSTEM:CONSOLE")
#else
#pragma comment (linker, "/SUBSYSTEM:WINDOWS")
#endif


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

	parser.process(app);
	// CMD parser --------------------------------------------------------------------

	// load settings
	rdf::Config& config = rdf::Config::instance();
	
	// load user defined settings
	if (parser.isSet(settingOpt)) {
		QString sName = parser.value(settingOpt);
		config.setSettingsFile(sName);
	}
	config.load();

	if (!parser.positionalArguments().empty()) {

		QString imgPath = parser.positionalArguments()[0].trimmed();
		
		rdf::DebugConfig dc;
		dc.setImagePath(imgPath);

		// add output path	
		if (parser.isSet(outputOpt))
			dc.setOutputPath(parser.value(outputOpt));

		// add xml path	
		if (parser.isSet(xmlOpt))
			dc.setXmlPath(parser.value(xmlOpt));

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
		}
		// my section
		else {
			qDebug() << "Servus Markus...";
			rdf::XmlTest test(dc);
			test.parseXml();
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