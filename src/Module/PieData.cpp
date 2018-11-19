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
// Qt Includes
#pragma warning(pop)

namespace rdf {
	
	PieData::PieData(const QString& xmlDir, const QString& jsonFile) {
		if (!xmlDir.isEmpty()) mXmlDir = xmlDir;
		if (!jsonFile.isEmpty()) mJsonFile = jsonFile;
	}

	QJsonObject PieData::getImgObject(const QString xmlDoc) {

		if (xmlDoc.isEmpty())
			return QJsonObject();

		QJsonObject newPage;
		calculateFeatures(newPage, xmlDoc);
		return newPage;
	}

	void PieData::saveJsonDatabase() {
		//QStringList filters;
		//filters << "*.xml";
		//fileInfoList = dir.entryInfoList(filters, QDir::Files | QDir::NoDotAndDotDot);

		QJsonObject xmlDatabaseObj;
		xmlDatabaseObj["database"] = mXmlDir;

		//QDir xmlRootDir(mXmlDir);
		QJsonArray databaseImgs;
		QDirIterator it(mXmlDir, QStringList() << "*.xml", QDir::Files, QDirIterator::Subdirectories);
		while (it.hasNext()) {
			QFileInfo f(it.next());
			QString tmp = f.absoluteFilePath();
			//qDebug() << it.next();
			QJsonObject currentXmlDoc;
			calculateFeatures(currentXmlDoc, f.absoluteFilePath());
			databaseImgs.append(currentXmlDoc);
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

	bool PieData::calculateFeatures(QJsonObject &document, QString xmlDoc) {

		if (xmlDoc.isEmpty()) {
			return false;
		}

		//Achtung!!! geändert
		//QString loadXmlPath = rdf::PageXmlParser::imagePathToXmlPath(mTemplateName);
		QString loadXmlPath = xmlDoc;

		rdf::PageXmlParser parser;
		if (!parser.read(loadXmlPath)) {
			qWarning() << "could not xml for creating database" << loadXmlPath;
			return false;
		}

		auto pe = parser.page();
		QSize templSize = pe->imageSize();

		QVector<QSharedPointer<rdf::Region>> test = rdf::Region::allRegions(pe->rootRegion().data());

		document["imgName"] = pe->imageFileName();
		document["xmlName"] = xmlDoc;
		document["width"] = templSize.width();
		document["height"] = templSize.height();

		QSharedPointer<rdf::Region> region;
		QSharedPointer<rdf::TextRegion> text;
		QSharedPointer<rdf::TableRegion> tableRegion;

		//bool detHeader = false;
		QJsonArray tableRegions;
		QJsonArray textRegions;
		QJsonArray imgRegions;
		QJsonArray graphicRegions;
		QJsonArray chartRegions;
		QJsonArray separatorRegions;


		//type_table_region,
		//type_text_region,
		//type_image,
		//type_graphic,
		//type_chart,
		//type_separator,
		for (auto i : test) {

			if (i->type() == i->type_table_region) {
				tableRegion = i.dynamicCast<rdf::TableRegion>();
				QJsonObject table;
				rdf::Polygon pol = tableRegion->polygon();
				QPolygonF qPol = pol.closedPolygon();
				QRectF qRect = qPol.boundingRect();
				table["size"] = qRect.width() * qRect.height();
				tableRegions.append(table);

			}
			else if (i->type() == i->type_text_region) {
				text = i.dynamicCast<rdf::TextRegion>();

				QJsonObject txt;
				rdf::Polygon pol = text->polygon();
				QPolygonF qPol = pol.closedPolygon();
				QRectF qRect = qPol.boundingRect();
				txt["size"] = qRect.width() * qRect.height();
				textRegions.append(txt);

			}
			else if (i->type() == i->type_image) {
				//image
				QJsonObject img;
				rdf::Polygon pol = i->polygon();
				QPolygonF qPol = pol.closedPolygon();
				QRectF qRect = qPol.boundingRect();
				img["size"] = qRect.width() * qRect.height();
				imgRegions.append(img);
			}
			else if (i->type() == i->type_graphic) {
				//graphic
				QJsonObject gra;
				rdf::Polygon pol = i->polygon();
				QPolygonF qPol = pol.closedPolygon();
				QRectF qRect = qPol.boundingRect();
				gra["size"] = qRect.width() * qRect.height();
				graphicRegions.append(gra);
			}
			else if (i->type() == i->type_chart) {
				//chart
				QJsonObject chart;
				rdf::Polygon pol = i->polygon();
				QPolygonF qPol = pol.closedPolygon();
				QRectF qRect = qPol.boundingRect();
				chart["size"] = qRect.width() * qRect.height();
				chartRegions.append(chart);
			}
			//} else if (i->type() == i->type_separator) {
			//	//separator
			//	QJsonObject sep;
			//	sep["length"] = 200;
			//	separatorRegions.append(sep);
			//}
		}

		document["tables"] = tableRegions;
		document["texts"] = textRegions;
		document["images"] = imgRegions;
		document["graphics"] = graphicRegions;
		document["charts"] = chartRegions;
		//document["seps"] = separatorRegions;


		return true;
	}

}