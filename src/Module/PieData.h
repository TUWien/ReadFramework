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

#include "Elements.h"
#include "PageParser.h"
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
		QJsonObject getImgObject(const QString xmlDoc = QString());
		QMap<QString, int> createDictionary(const QString &txt, const int ignoreSize = 3) const;
		void saveJsonDatabase();


	protected:
		bool calculateFeatures(QJsonObject &document, QString xmlDoc);
		QString normalize(const QString& str) const;

	private:
		QString mXmlDir = "C:\\tmp\\read-database";
		QString mJsonFile = "C:\\tmp\\read-database\\database.json";
		QMap<QString,int> mDictionary;

	};


	//don't care
	//QtJson::JsonObject document;
	//document["name"] = "page1.png"
	//
	//QtJson::JsonArray imgRegions;
	//QtJson::JsonObject image1, image2;
	//image1["size"] = 20;
	//image2["size"] = 40;
	//imgRegions.append(image1);
	//imgRegions.append(image2);

	//QtJson::JsonArray txtRegions;
	//QtJson::JsonObject text1, text2;
	//text1["size"] = 20;
	//text2["size"] = 40;
	//imgRegions.append(text1);
	//imgRegions.append(text2);
	//	
	//document["images"] = imgRegions;
	//document["text"] = txtRegions;

}