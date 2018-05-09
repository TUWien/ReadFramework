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

#include "Settings.h"

#include "Utils.h"

#pragma warning(push, 0)	// no warnings from includes
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QDebug>
#pragma warning(pop)

namespace rdf {

// -------------------------------------------------------------------- DefaultSettings
DefaultSettings::DefaultSettings() : QSettings(Config::instance().settingsFilePath(), QSettings::IniFormat) {}


// GlobalSettings --------------------------------------------------------------------
GlobalConfig::GlobalConfig() : ModuleConfig("Global") {
}

QString GlobalConfig::toString() const {
	return ModuleConfig::toString();
}

QString GlobalConfig::workingDir() const {
	
	QFileInfo wd(mWorkingDir);

	if (wd.isDir() && wd.isWritable())
		return mWorkingDir;
	

	return QStandardPaths::writableLocation(QStandardPaths::TempLocation);
}

QString GlobalConfig::xmlSubDir() const {
	return mXmlSubDir;
}

void GlobalConfig::setNumScales(int ns) {
	mNumScales = ns;
}

int GlobalConfig::numScales() const {
	return mNumScales;
}

void GlobalConfig::load(const QSettings& settings) {

	mWorkingDir = settings.value("workingDir", workingDir()).toString();
	mXmlSubDir = settings.value("xmlSubDir", xmlSubDir()).toString();
}

void GlobalConfig::save(QSettings& settings) const {

	settings.setValue("workingDir", workingDir());
	settings.setValue("xmlSubDir", xmlSubDir());
	// add saving here...
}

// Config --------------------------------------------------------------------
Config::Config() {

	load();
}

Config& Config::instance() { 

	static QSharedPointer<Config> inst;
	if (!inst)
		inst = QSharedPointer<Config>(new Config());
	return *inst; 
}

GlobalConfig& Config::global() {
	return instance().mGlobal;
}

bool Config::isPortable() const {

	return settingsPath() == QCoreApplication::applicationDirPath();
}

void Config::setSettingsFile(const QString& filePath) {

	if (filePath.isEmpty())
		return;

	QFileInfo fileInfo(filePath);
	if (fileInfo.exists()) {
		mSettingsPath = fileInfo.absolutePath();
		mSettingsFileName = fileInfo.fileName();
	}
}

QString Config::settingsFilePath() const {

	return QFileInfo(settingsPath(), mSettingsFileName).absoluteFilePath();
}

QString Config::settingsPath() const {

	// check if the input path exists
	QFileInfo sf(mSettingsPath, mSettingsFileName);
	if (!mSettingsPath.isEmpty() && sf.exists())
		return sf.absolutePath();

	// check if we have a local settings file (portable)
	sf = QFileInfo(QCoreApplication::applicationDirPath(), mSettingsFileName);
	if (sf.exists())
		return sf.absolutePath();

	return Utils::appDataPath();
}

void Config::load() {

	QSettings s(settingsFilePath(), QSettings::IniFormat);
	mGlobal.loadSettings(s);
	qInfo() << "[READ] loading settings from" << settingsFilePath();
}

void Config::save() const {

	QSettings s(settingsFilePath(), QSettings::IniFormat);
	mGlobal.saveSettings(s);
}

}