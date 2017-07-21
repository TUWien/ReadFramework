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

#pragma once

#pragma warning(push, 0)	// no warnings from includes
#include <QObject>
#include <QSharedPointer>
#include <QDebug>
#pragma warning(pop)

#include <opencv2/core.hpp>

#pragma warning (disable: 4251)	// inlined Qt functions in dll interface

#ifndef DllCoreExport
#ifdef DLL_CORE_EXPORT
#define DllCoreExport Q_DECL_EXPORT
#else
#define DllCoreExport Q_DECL_IMPORT
#endif
#endif

// Qt defines
class QSettings;

namespace rdf {

#define mDebug		qDebug().noquote()		<< debugName()
#define mInfo		qInfo().noquote()		<< debugName()
#define mWarning	qWarning().noquote()	<< debugName()
#define mCritical	qCritical().noquote()	<< debugName()

class DllCoreExport ModuleConfig {

public:
	ModuleConfig(const QString& moduleName = "Generic Module");

	friend DllCoreExport QDataStream& operator<<(QDataStream& s, const ModuleConfig& m);
	friend DllCoreExport QDebug operator<< (QDebug d, const ModuleConfig &m);

	void loadSettings();
	void loadSettings(QSettings& settings);
	void saveSettings() const;
	void saveSettings(QSettings& settings) const;
	void saveDefaultSettings() const;
	virtual void saveDefaultSettings(QSettings& settings) const;

	QString name() const;
	virtual QString toString() const;

protected:
	virtual void load(const QSettings& settings);
	virtual void save(QSettings& settings) const;

	QString mModuleName;						/**< the module's name.**/

	template <class num>
	num checkParam(num param, num min, num max, const QString & name) const {

		if (param < min) {
			qWarning().noquote() << name << "must be >" << min << "but it is: " << param;
			return min;
		}

		if (param > max) {
			qWarning().noquote() << name << "must be <" << max << "but it is: " << param;
			return max;
		}

		return param;
	}

};


/// <summary>
/// This is the base class for all modules.
/// It provides all functions which are implemented by the modules.
/// </summary>
class DllCoreExport Module {
	
public:

	/// <summary>
	/// Default constructor
	/// Initializes a new instance of the <see cref="Module"/> class.
	/// </summary>
	Module();

	friend DllCoreExport QDataStream& operator<<(QDataStream& s, const Module& m);
	friend DllCoreExport QDebug operator<< (QDebug d, const Module &m);


	 /// <summary>
	 /// Returns true if the module was initialized with the default constructor.
	 /// Note, if empty is true, nothing can be computed.
	 /// </summary>
	 /// <returns>Returns true if the module was initialized.</returns>
	virtual bool isEmpty() const = 0;

	 /// <summary>
	 /// Returns the module's name.
	 /// </summary>
	 /// <returns>The module's name.</returns>
	virtual QString name() const;

	 /// <summary>
	 /// Converts the module's parameters and results to a string.
	 /// </summary>
	 /// <returns>The string containing all parameters and results of the module.</returns>
	virtual QString toString() const;

	 /// <summary>
	 /// Runs the algorithm implemented by the module.
	 /// </summary>
	 /// <returns>True on success.</returns>
	virtual bool compute() = 0;

	virtual void setConfig(QSharedPointer<ModuleConfig> config);
	QSharedPointer<ModuleConfig> config() const;

protected:
	QSharedPointer<ModuleConfig> mConfig;		/**< the module config **/

	virtual bool checkInput() const = 0;		/**< checks if all input images are in the specified format.**/
	QString debugName() const;

};

}
