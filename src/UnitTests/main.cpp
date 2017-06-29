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
#include "BaselineTest.h"

#if defined(_MSC_BUILD) && !defined(QT_NO_DEBUG_OUTPUT) // fixes cmake bug - really release uses subsystem windows, debug and release subsystem console
#pragma comment (linker, "/SUBSYSTEM:CONSOLE")
#else
#pragma comment (linker, "/SUBSYSTEM:WINDOWS")
#endif

int main(int argc, char** argv) {

	// check opencv version
	qInfo().nospace() << "I am using OpenCV " << CV_MAJOR_VERSION << "." << CV_MINOR_VERSION << "." << CV_VERSION_REVISION;

	QCoreApplication::setOrganizationName("TU Wien");
	QCoreApplication::setOrganizationDomain("http://www.caa.tuwien.ac.at/cvl");
	QCoreApplication::setApplicationName("READ Framework");
	rdf::Utils::instance().initFramework();

	QCoreApplication app(argc, (char**)argv);	// enable headless

	// CMD parser --------------------------------------------------------------------
	QCommandLineParser parser;

	parser.setApplicationDescription("READ Framework testing application.");
	parser.addHelpOption();
	parser.addVersionOption();

	// baseline test
	QCommandLineOption baseLineOpt(QStringList() << "b" << "baseline", QObject::tr("Test Baseline."));
	parser.addOption(baseLineOpt);

	// table test
	QCommandLineOption tableOpt(QStringList() << "t" << "table", QObject::tr("Test Table."));
	parser.addOption(tableOpt);

	parser.process(*QCoreApplication::instance());
	// CMD parser --------------------------------------------------------------------

	// load settings
	rdf::Config& config = rdf::Config::instance();
	
	// test baseline extraction
	if (parser.isSet(baseLineOpt)) {
		
		rdf::BaselineTest bt;
		
		if (!bt.baselineTest())
			return 1;	// fail the test
		
	} else if (parser.isSet(tableOpt)) {
		parser.showHelp();

	} else 	{
		qInfo() << "Please specify an input image...";
		parser.showHelp();
	}

	// save settings
	config.save();
	return 0;	// thanks
}
