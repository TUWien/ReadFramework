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

#include "PageParser.h"
#include "Elements.h"
#include "Utils.h"

#pragma warning(push, 0)	// no warnings from includes
#include <QFile>
#include <QFileInfo>
#include <QXmlStreamReader>
#include <QDebug>
#pragma warning(pop)

namespace rdf {

PageXmlParser::PageXmlParser() {

}

void PageXmlParser::read(const QString & xmlPath) {

	mPage = parse(xmlPath);

}

QString PageXmlParser::tagName(const RootTags & tag) const {
	
	switch (tag) {
	case tag_page:				return "Page";
	case attr_imageFilename:		return "imageFilename";
	case attr_imageWidth:		return "imageWidth";
	case attr_imageHeight:		return "imageHeight";
	case tag_meta:				return "MetaData";
	case attr_meta_creator:		return "Creator";
	case attr_meta_created:		return "Created";
	case attr_meta_changed:		return "LastChange";
	case attr_id:				return "id";
	case attr_text_type:		return "type";
	}
	
	return "";
}

QSharedPointer<PageElement> PageXmlParser::parse(const QString& xmlPath) const {

	Timer dt;
	QFile f(xmlPath);
	QSharedPointer<PageElement> pageElement;

	if (!f.open(QIODevice::ReadOnly)) {
		qWarning() << "Sorry, I could not open " << xmlPath << " for reading...";
		return pageElement;
	}

	// load the element
	QFileInfo xmlInfo = xmlPath;
	QXmlStreamReader reader(f.readAll());
	f.close();

	QString pageTag = tagName(tag_page);	// cache - since it might be called a lot of time
	QString metaTag = tagName(tag_meta);

	Region dummyRegion;
	QStringList regionNames = dummyRegion.typeNames();
	
	// ok - we can initialize our page element
	pageElement = QSharedPointer<PageElement>(new PageElement());
	QSharedPointer<Region> root = QSharedPointer<Region>(new Region());

	while (!reader.atEnd()) {

		if (reader.tokenType() == QXmlStreamReader::StartElement && reader.qualifiedName() == tagName(tag_meta)) {
			// TODO
		
		}
		// e.g. <Page imageFilename="00001234.tif" imageWidth="1000" imageHeight="2000">
		else if (reader.tokenType() == QXmlStreamReader::StartElement && reader.qualifiedName() == tagName(tag_page)) {


			pageElement->setImageFileName(reader.attributes().value(tagName(attr_imageFilename)).toString());

			bool wok = false, hok = false;
			int width = reader.attributes().value(tagName(attr_imageWidth)).toInt(&wok);
			int height = reader.attributes().value(tagName(attr_imageHeight)).toInt(&hok);

			if (wok & hok)
				pageElement->setImageSize(QSize(width, height));
			else
				qWarning() << "could not read image dimensions";
		}
		// e.g. <TextRegion id="r1" type="heading">
		else if (reader.tokenType() == QXmlStreamReader::StartElement && regionNames.contains(reader.qualifiedName().toString())) {
			
			parseRegion(reader, root, regionNames);
		}

		reader.readNext();
	}

	pageElement->setRootRegion(root);

	qDebug() << xmlInfo.fileName() << "parsed in" << dt << " I found" << root->children().size() << "elements...";

	return pageElement;
}

void PageXmlParser::parseRegion(QXmlStreamReader & reader, QSharedPointer<Region> parent, const QStringList& regionNames) const {

	// TODO: e.g. word needs to be a different object
	QSharedPointer<Region> region = QSharedPointer<Region>(new Region());
	region->setType(reader.qualifiedName().toString());
	region->setId(reader.attributes().value(tagName(attr_id)).toString());
	
	parent->addChild(region);

	// TODO add type to text regions
	//region->setTextType(reader.attributes().value(tagName(attr_text_type)).toString());

	while (!reader.atEnd()) {
		reader.readNext();

		QString tag = reader.qualifiedName().toString();

		// are we done here?
		if (reader.tokenType() == QXmlStreamReader::EndElement && regionNames.contains(tag))
			break;

		// append children?!
		if (reader.tokenType() == QXmlStreamReader::StartElement && regionNames.contains(tag)) {
			parseRegion(reader, parent, regionNames);
		}
		else
			region->read(reader);	// present current line to the region
	}

	//qDebug() << "adding " << *region;

}



}