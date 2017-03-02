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

#include "Utils.h"

#pragma warning(push, 0)	// no warnings from includes
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>
#include <QSettings>
#include <QDebug>
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
	superPixelClassifierPath = settings->value("superPixelClassifierPath", superPixelClassifierPath).toString();
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
	if (!force && superPixelClassifierPath != initS.superPixelClassifierPath)
		settings->setValue("superPixelClassifierPath", superPixelClassifierPath);

	// add saving here...

	settings->endGroup();
}

void GlobalSettings::defaultSettings() {

	workingDir = "";
	xmlSubDir = "page";
	settingsFileName = "rdf-settings.nfo";
	superPixelClassifierPath = "super-pixel-classifier.json";
}

// Config --------------------------------------------------------------------
Config::Config() {

	mSettings = QSharedPointer<QSettings>(new QSettings(createSettingsFilePath(), QSettings::IniFormat));
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

	return settingsPath() == QCoreApplication::applicationDirPath();
}

void Config::setSettingsFile(const QString& filePath) {

	if (filePath.isEmpty())
		return;

	QFileInfo fileInfo(filePath);
	if (fileInfo.exists()) {
		mSettings = QSharedPointer<QSettings>(new QSettings(filePath, QSettings::IniFormat));
		mGlobal.settingsFileName = fileInfo.fileName();
	}
}

QString Config::settingsFilePath() const {
	return createSettingsFilePath();
}

QString Config::createSettingsFilePath(const QString& fileName) const {

	QString settingsName = (!fileName.isEmpty()) ? fileName : mGlobal.settingsFileName;

	return QFileInfo(settingsPath(), settingsName).absoluteFilePath();
}

QString Config::settingsPath() const {

	// check if we have a local settings file (portable)
	QFileInfo sf(QCoreApplication::applicationDirPath(), mGlobal.settingsFileName);
	if (sf.exists())
		return sf.absolutePath();

	return Utils::appDataPath();
}

void Config::load() {

	mGlobal.load(mSettings);
	mGlobalInit = mGlobal;
	
	qInfo() << "[READ] loading settings from" << mGlobal.settingsFileName;
}

void Config::save() const {

	bool force = false;	// not really needed?!
	mGlobal.save(mSettings, mGlobalInit, force);
}

}