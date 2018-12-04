/*******************************************************************************************************
 ReadFramework is the basis for modules developed at CVL/TU Wien for the EU project READ. 
  
 Copyright (C) 2016 Markus Diem <diem@caa.tuwien.ac.at>
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

#include "Elements.h"
#include "PageParser.h"
#include "word2vec/Word2Vec.h"

#pragma warning(push, 0)	// no warnings from includes
// Qt Includes
#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QDirIterator>
#include <QJsonDocument>
#include <QDebug>
#pragma warning(pop)

#ifndef DllCoreExport
#ifdef DLL_CORE_EXPORT
#define DllCoreExport Q_DECL_EXPORT
#else
#define DllCoreExport Q_DECL_IMPORT
#endif
#endif

// Qt defines

namespace rdf {

// read defines
	class DllCoreExport PieData {

	public:
		PieData(const QString& xmlDir = QString(), const QString& jsonFile = QString());
		QJsonObject getImgObject(const QString& xmlDoc = QString());
		QMap<QString, int> createDictionary(const QString &txt, const int ignoreSize = 3) const;
		void saveJsonDatabase();
		void setWord2Vec(bool w=false);


	protected:
		bool collect(QJsonObject &document, const QString& xmlPath);
		bool calculateFeatures(QSharedPointer<PageElement> page, QJsonObject& document);
		bool calculateLabels(QSharedPointer<PageElement> page, QJsonObject& document);
		bool calculateDictionary(const QJsonObject& document);
		QString normalize(const QString& str) const;

	private:
		QString mXmlDir = "C:\\tmp\\read-database";
		QString mJsonFile = "C:\\tmp\\read-database\\database.json";
		QMap<QString,int> mDictionary;
		int mFilterDict = 2;
		bool mWord2Vec = true;
		bool mSaveWordVecToFile = false;

	};

}