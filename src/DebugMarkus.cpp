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

#pragma warning(push, 0)	// no warnings from includes
#include <QDebug>
#pragma warning(pop)

namespace rdf {

XmlTest::XmlTest(const DebugConfig& config) {

	mConfig = config;
}

void XmlTest::parseXml() {

	rdf::Timer dt;
	int idx = 0;

	// loop for sampling time...
	for ( ; idx < 1; idx++) {

		PageXmlParser parser;
		parser.read(mConfig.xmlPath());
		
		parser.write(PageXmlParser::imagePathToXmlPath(mConfig.outputPath()), parser.page());
	}

	qDebug() << mConfig.xmlPath() << "parsed" << idx << "x in " << dt << "mean" << dt.stringifyTime(dt.elapsed()/idx);

}

}