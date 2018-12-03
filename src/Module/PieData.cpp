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

#include "PieData.h"

#pragma warning(push, 0)	// no warnings from includes
#include <QTextDocument>
#pragma warning(pop)

namespace rdf {
	
	PieData::PieData(const QString& xmlDir, const QString& jsonFile) {
		if (!xmlDir.isEmpty()) mXmlDir = xmlDir;
		if (!jsonFile.isEmpty()) mJsonFile = jsonFile;
	}

	QJsonObject PieData::getImgObject(const QString& xmlDoc) {

		if (xmlDoc.isEmpty())
			return QJsonObject();

		QJsonObject newPage;
		collect(newPage, xmlDoc);

		return newPage;
	}

	QMap<QString, int> PieData::createDictionary(const QString & txt, const int ignoreSize) const 	{

		QStringList words = txt.split(QRegExp("\\s+"), QString::SkipEmptyParts);
		QStringList filteredWords;

		for (const auto &word : words) {
			if (word.length() > ignoreSize) filteredWords << word;
		}

		QMap<QString, int> dictionary;
		for (const auto &word : filteredWords) {
			dictionary[word]++;
		}

		//qDebug() << dictionary;

		return dictionary;

	}

	void PieData::saveJsonDatabase() {
		//QStringList filters;
		//filters << "*.xml";
		//fileInfoList = dir.entryInfoList(filters, QDir::Files | QDir::NoDotAndDotDot);

		QJsonObject xmlDatabaseObj;
		xmlDatabaseObj["database"] = mXmlDir;
		mDictionary.clear();

		//QDir xmlRootDir(mXmlDir);
		QJsonArray databaseImgs;
		QMap<QString, QJsonArray> documents;

		QDirIterator it(mXmlDir, QStringList() << "*.xml", QDir::Files, QDirIterator::Subdirectories);
		while (it.hasNext()) {
			QFileInfo f(it.next());
			
			QJsonObject page;
			if (collect(page, f.absoluteFilePath())) {

				QString identifier = page["collection"].toString() + "|" + page["document"].toString();	// | is not allows in paths
				QJsonArray da = documents.value(identifier);
				da << page;
				documents.insert(identifier, da);
			}
		}
		
		// -------------------------------------------------------------------- recreate hierarchy 
		// here we get the documents
		QMap<QString, QJsonArray> collections;
		QMap<QString, QJsonArray>::const_iterator di = documents.constBegin();
		while (di != documents.constEnd()) {
			
			QStringList id = di.key().split("|");

			if (id.size() != 2) {
				di++;
				continue;
			}

			QJsonArray pages = di.value();
			QJsonObject dob;
			dob["name"] = id[1];
			dob["pages"] = pages;
			
			QJsonArray da = collections.value(id[0]);
			da << dob;
			collections.insert(id[0], da);

			di++;	// == dr ; )
		}

		// and now we put them together int o collections
		QJsonArray colArray;
		QMap<QString, QJsonArray>::const_iterator ci = collections.constBegin();
		while (ci != collections.constEnd()) {

			QJsonObject dob;
			dob["name"] = ci.key();
			dob["documents"] = ci.value();

			colArray << dob;

			ci++;	// == dr ; )
		}

		xmlDatabaseObj["collections"] = colArray;

		if (!mDictionary.isEmpty()) {
			QVariantMap vDict;
			for (const auto w : mDictionary.keys()) {
				if (mDictionary.value(w) > mFilterDict)
					vDict.insert(w, mDictionary.value(w));
			}
			QJsonObject dictionary = QJsonObject::fromVariantMap(vDict);
			xmlDatabaseObj["dictionary"] = dictionary;
		}

		xmlDatabaseObj["imgs"] = databaseImgs;

		QFile saveFile(mJsonFile);
		if (!saveFile.open(QIODevice::WriteOnly)) {
			qWarning("Couldn't open save file.");
			return;
		}

		QJsonDocument saveDatabase(xmlDatabaseObj);
		saveFile.write(saveDatabase.toJson());

	}

	bool PieData::collect(QJsonObject &document, const QString& xmlPath) {

		if (xmlPath.isEmpty()) {
			return false;
		}

		rdf::PageXmlParser parser;
		if (!parser.read(xmlPath)) {
			qWarning() << "could not xml for creating database" << xmlPath;
			return false;
		}

		auto pe = parser.page();
		
		// add your constraints here...
		if (pe->rootRegion()->children().size() == 0) {
			qDebug() << xmlPath << "not added to the database...";
			return false;
		}

		document["xmlPath"] = xmlPath;

		bool ok = calculateFeatures(pe, document);
		if (ok)
			ok = calculateLabels(pe, document);
		if (ok)
			ok = calculateDictionary(document);

		return ok;
	}

	bool PieData::calculateFeatures(QSharedPointer<PageElement> page, QJsonObject & document) {
		
		QVector<QSharedPointer<rdf::Region>> regions = rdf::Region::allRegions(page->rootRegion().data());

		document["imgName"] = page->imageFileName();
		document["width"]	= page->imageSize().width();
		document["height"]	= page->imageSize().height();

		QJsonArray jsonRegions;
		QString txtFromRegions;
		QString txtFromLines;

		for (auto r : regions) {

			// calculate general region properties
			QJsonObject rProp;
			rdf::Polygon pol = r->polygon();
			QPolygonF qPol = pol.closedPolygon();
			QRectF qRect = qPol.boundingRect();
			rProp["width"] = qRect.width();
			rProp["height"] = qRect.height();
			rProp["type"] = r->type();


			// append specific region properties
			switch (r->type()) {

			case Region::type_table_region:
			case Region::type_chart:
			case Region::type_image:
			case Region::type_graphic:
				jsonRegions << rProp;
				break;

			case Region::type_text_region: {
				jsonRegions << rProp;
				auto tr = r.dynamicCast<rdf::TextRegion>();
				QString ct = normalize(tr->text());
				if (!ct.isEmpty())
					txtFromRegions += " " + ct;
				break;
			}
			case Region::type_text_line: {
				auto tl = r.dynamicCast<rdf::TextLine>();
				QString ct = normalize(tl->text());
				if (!ct.isEmpty())
					txtFromLines += " " + ct;
				break;
			}
			}
		}

		// prefer text from regions
		QString content;
		if (!txtFromRegions.isEmpty())
			content = txtFromRegions;
		else if (!txtFromLines.isEmpty())
			content = txtFromLines;

		if (!regions.isEmpty())		document["regions"] = jsonRegions;
		if (!content.isEmpty())
			document["content"] = content;
		else
			qDebug() << "no content written...";
		//document["seps"] = separatorRegions;

		return true;
	}

	bool PieData::calculateLabels(QSharedPointer<PageElement> page, QJsonObject & document) {
		
		QString path = document["xmlPath"].toString();
		
		if (path.isEmpty())
			return false;

		QStringList entries = path.split("/"); 

		if (entries.size() >= 3) {
			document["document"] = entries[entries.size() - 3];
		}
		if (entries.size() >= 4) {
			document["collection"] = entries[entries.size() - 4];
		}
		
		return true;
	}

	bool PieData::calculateDictionary(const QJsonObject & document) {
		
		QString content = document["content"].toString();

		if (!content.isEmpty()) {
			QMap<QString, int> dict = createDictionary(content);
			//QVariantMap vDict;
			if (!dict.isEmpty()) {

				for (const auto w : dict.keys()) {
					//vDict.insert(w, dict.value(w));
					//mWords << w;
					mDictionary[w] += dict.value(w);
				}
				//QJsonObject dictionary = QJsonObject::fromVariantMap(vDict);
				//document["dictionary"] = dictionary;
			}
		}
		
		return true;
	}

	QString PieData::normalize(const QString & str) const {
		
		QTextDocument doc;
		doc.setHtml(str);


		QString ns = doc.toPlainText();

		ns = ns.replace(QRegExp("[^a-zA-Z\\ ]+"), "");
		ns = ns.trimmed();
		//qDebug() << "normalized...";

		return ns;
	}

}