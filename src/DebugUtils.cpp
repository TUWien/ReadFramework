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

#include "DebugUtils.h"

#include "PageParser.h"
#include "Settings.h"

#pragma warning(push, 0)	// no warnings from includes
#include <QDebug>
#pragma warning(pop)

namespace rdf {

// DebugConfig --------------------------------------------------------------------	
DebugConfig::DebugConfig() {

}

void DebugConfig::setImagePath(const QString & path) {
	mImagePath = path;
}

QString DebugConfig::imagePath() const {
	return mImagePath;
}

void DebugConfig::setXmlPath(const QString & path) {
	mXMLPath = path;
}

QString DebugConfig::xmlPath() const {

	if (mXMLPath.isEmpty())
		return PageXmlParser::imagePathToXmlPath(imagePath());

	return mXMLPath;
}

void DebugConfig::setOutputPath(const QString & path) {
	mOutputPath = path;
}

QString DebugConfig::outputPath() const {
	return mOutputPath;
}

void DebugConfig::setClassifierPath(const QString & path) {
	mClassifierPath = path;
}

QString DebugConfig::classifierPath() const {
	
	//if (mClassifierPath.isEmpty())
	//	return Config::instance().global().superPixelClassifierPath;

	return mClassifierPath;
}

void DebugConfig::setLabelConfigPath(const QString & path) {
	mLabelConfigPath = path;
}

QString DebugConfig::labelConfigPath() const {
	return mLabelConfigPath;
}

void DebugConfig::setFeatureCachePath(const QString & path) {
	mFeatureCachePath = path;
}

QString DebugConfig::featureCachePath() const {
	return mFeatureCachePath;
}

}