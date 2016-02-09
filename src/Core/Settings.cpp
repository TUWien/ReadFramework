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

#include "Settings.h"

#pragma warning(push, 0)	// no warnings from includes
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>
#include <QSettings>
#pragma warning(pop)

namespace rdf {

// GlobalSettings --------------------------------------------------------------------
GlobalSettings::GlobalSettings() {

	mName = "GlobalSettings";
	defaultSettings();
}

void GlobalSettings::load(QSharedPointer<QSettings> settings) {

	settings->beginGroup(mName);

	workingDir = settings->value("workingDir", workingDir).toString();
	xmlSubDir = settings->value("xmlSubDir", xmlSubDir).toString();
	settingsFileName = settings->value("settingsFileName", settingsFileName).toString();
	// add loading here...

	settings->endGroup();
}

void GlobalSettings::save(QSharedPointer<QSettings> settings, const GenericSettings & init, bool force) const {

	const GlobalSettings& initS = dynamic_cast<const GlobalSettings &>(init);

	settings->beginGroup(mName);

	if (!force && workingDir != initS.workingDir)
		settings->setValue("workingDir", workingDir);
	if (!force && xmlSubDir != initS.xmlSubDir)
		settings->setValue("xmlSubDir", xmlSubDir);
	if (!force && settingsFileName != initS.settingsFileName)
		settings->setValue("settingsFileName", settingsFileName);

	// add saving here...

	settings->endGroup();
}

void GlobalSettings::defaultSettings() {

	workingDir = "";
	xmlSubDir = "page";
	settingsFileName = "rdf-settings.nfo";
}

// Config --------------------------------------------------------------------
Config::Config() {

	mSettings = isPortable() ? QSharedPointer<QSettings>(new QSettings(createSettingsFilePath(), QSettings::IniFormat)) : QSharedPointer<QSettings>(new QSettings());
	load();
}

Config& Config::instance() { 

	static QSharedPointer<Config> inst;
	if (!inst)
		inst = QSharedPointer<Config>(new Config());
	return *inst; 
}

QSettings& Config::settings() {
	return *mSettings;
}

GlobalSettings& Config::global() {
	return instance().globalIntern();
}

GlobalSettings& Config::globalIntern() {
	return mGlobal;
}

bool Config::isPortable() const {

	QFileInfo settingsFile = createSettingsFilePath();
	return settingsFile.isFile() && settingsFile.exists();
}

void Config::setSettingsFile(const QString& fileName) {

	if (fileName.isEmpty())
		return;

	QString settingsPath = createSettingsFilePath(fileName);
	
	if (QFileInfo(settingsPath).exists()) {
		mSettings = QSharedPointer<QSettings>(new QSettings(settingsPath, QSettings::IniFormat));
		mGlobal.settingsFileName = fileName;
	}

}

QString Config::createSettingsFilePath(const QString& fileName) const {

	QString settingsName = (!fileName.isEmpty()) ? fileName : mGlobal.settingsFileName;

	return QFileInfo(QCoreApplication::applicationDirPath(), settingsName).absoluteFilePath();
}

void Config::load() {

	mGlobal.load(mSettings);
	mGlobalInit = mGlobal;
}

void Config::save() const {

	bool force = false;	// not really needed?!
	mGlobal.save(mSettings, mGlobalInit, force);
}

}