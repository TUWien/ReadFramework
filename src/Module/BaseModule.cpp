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
 [1] https://cvl.tuwien.ac.at/
 [2] https://transkribus.eu/Transkribus/
 [3] https://github.com/TUWien/
 [4] https://nomacs.org
 *******************************************************************************************************/

#include "BaseModule.h"
#include "Settings.h"
#include "ScaleFactory.h"

#pragma warning(push, 0)	// no warnings from includes
#include <QSettings>
#include <QDebug>
#pragma warning(pop)

namespace rdf {

// ModuleConfig --------------------------------------------------------------------
ModuleConfig::ModuleConfig(const QString& moduleName) {
	mModuleName = moduleName;
}

void ModuleConfig::loadSettings() {

	DefaultSettings s;
	loadSettings(s);
}

void ModuleConfig::loadSettings(QSettings & settings) {
	settings.beginGroup(mModuleName);
	load(settings);
	settings.endGroup();
}

void ModuleConfig::saveSettings() const {

	DefaultSettings s;
	saveSettings(s);
}

void ModuleConfig::saveSettings(QSettings & settings) const {

	settings.beginGroup(mModuleName);
	save(settings);
	settings.endGroup();
}

void ModuleConfig::saveDefaultSettings() const {
	
	QSettings s(Config::instance().settingsFilePath(), QSettings::IniFormat);
	saveDefaultSettings(s);
}

void ModuleConfig::saveDefaultSettings(QSettings& settings) const {

	if (name() != "Generic Module") {
		// write default settings
		if (!settings.childGroups().contains(name())) {
			saveSettings(settings);
		}
	}
}

QString ModuleConfig::name() const {
	return mModuleName;
}

QString ModuleConfig::toString() const {
	return name();
}

void ModuleConfig::load(const QSettings&) {
	// dummy
	qWarning() << "ModuleConfig::load() called - make sure to call the derived method...";
}

void ModuleConfig::save(QSettings&) const {
	// dummy
	qWarning() << "ModuleConfig::save() called - make sure to call the derived method...";
}

// Module --------------------------------------------------------------------
Module::Module() {
	mConfig = QSharedPointer<ModuleConfig>::create();
}

QString Module::name() const {
	return mConfig->name();
}

void Module::setConfig(QSharedPointer<ModuleConfig> config) {
	mConfig = config;
}

QSharedPointer<ModuleConfig> Module::config() const {
	return mConfig;
}

QString Module::debugName() const {
	return "[" + mConfig->name() + "]";
}

QString Module::toString() const {
	return debugName();
}

QDataStream& operator<<(QDataStream& s, const Module& m) {

	// this makes the operator<< virtual (stroustrup)
	s << m.toString();
	return s;
}

QDebug operator<<(QDebug d, const Module& m) {

	d << qPrintable(m.toString());
	return d;
}

// -------------------------------------------------------------------- ScaleModuleConfig 
ScaleModuleConfig::ScaleModuleConfig(const QString & moduleName, const QSharedPointer<ScaleFactory>& sf) : ModuleConfig(moduleName) {
	
	mScaleFactory = sf ? sf : QSharedPointer<ScaleFactory>(new ScaleFactory());
}

void ScaleModuleConfig::setScaleFactory(const QSharedPointer<ScaleFactory>& sf) {
	mScaleFactory = sf;
}

QSharedPointer<ScaleFactory> ScaleModuleConfig::scaleFactory() {
	return mScaleFactory;
}

}