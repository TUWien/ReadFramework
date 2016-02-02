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

#include "BaseModule.h"
#include "Settings.h"

#pragma warning(push, 0)	// no warnings from includes
#include <QSettings>
#include <QDebug>
#pragma warning(pop)

namespace rdf {

// Module --------------------------------------------------------------------
Module::Module() {
	mModuleName = "Generic Module";
}

QString Module::name() const {
	return mModuleName;
}

QString Module::debugName() const {
	return "[" + mModuleName + "]";
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

void Module::loadSettings() {

	QSettings& settings = Config::instance().settings();
	settings.beginGroup(mModuleName);
	load(settings);
	settings.endGroup();
}

void Module::saveSettings() {

	QSettings& settings = Config::instance().settings();
	settings.beginGroup(mModuleName);
	save(settings);
	settings.endGroup();
}

void Module::load(const QSettings&) {
	// dummy
}

void Module::save(QSettings&) const {
	// dummy
}

}