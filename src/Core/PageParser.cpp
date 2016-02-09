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
#include "ElementsHelper.h"
#include "Utils.h"
#include "Settings.h"

#pragma warning(push, 0)	// no warnings from includes
#include <QFile>
#include <QFileInfo>
#include <QXmlStreamReader>
#include <QDebug>
#include <QBuffer>
#include <QDir>
#pragma warning(pop)

namespace rdf {

PageXmlParser::PageXmlParser() {

}

void PageXmlParser::read(const QString & xmlPath) {

	mPage = parse(xmlPath);

}

void PageXmlParser::write(const QString & xmlPath, const QSharedPointer<PageElement> pageElement) {

	mPage = pageElement;

	if (!mPage) {
		qWarning() << "[PageXmlWriter] cannot write a NULL page...";
		return;
	}

	Timer dt;

	QFileInfo fileInfo(xmlPath);

	// update date created
	if (!fileInfo.exists())
		mPage->setDateCreated(QDateTime::currentDateTimeUtc());

	// update date modified
	mPage->setDateModified(QDateTime::currentDateTimeUtc());	// using UTC directly here - somehow the +01:00 to CET is not working here
	
	const QByteArray& ba = writePageElement();

	QFile file(xmlPath);
	file.open(QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::Text);

	qint64 success = file.write(ba);
	file.close();

	if (success > 0)
		qDebug() << "XML written to" << xmlPath << "in" << dt;
	else
		qDebug() << "could not write to" << xmlPath;
}

QSharedPointer<PageElement> PageXmlParser::page() const {
	return mPage;
}

QString PageXmlParser::tagName(const RootTags & tag) const {
	
	switch (tag) {

	case tag_root:				return "PcGts";
	case attr_xmlns:			return "xmlns";
	case attr_xsi:				return "xmlns:xsi";
	case attr_schemaLocation:	return "xsi:schemaLocation";

	case tag_page:				return "Page";
	case attr_imageFilename:	return "imageFilename";
	case attr_imageWidth:		return "imageWidth";
	case attr_imageHeight:		return "imageHeight";

	case tag_meta:				return "Metadata";
	case tag_meta_creator:		return "Creator";
	case tag_meta_created:		return "Created";
	case tag_meta_changed:		return "LastChange";

	case attr_text_type:		return "type";
	}
	
	return "";
}

QSharedPointer<PageElement> PageXmlParser::parse(const QString& xmlPath) const {

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

	RegionManager& rm = RegionManager::instance();
	
	// ok - we can initialize our page element
	pageElement = QSharedPointer<PageElement>(new PageElement());
	QSharedPointer<Region> root = QSharedPointer<Region>(new Region());
	root->setType(Region::type_root);

	Timer dt;

	while (!reader.atEnd()) {

		QString tag = reader.qualifiedName().toString();

		if (reader.tokenType() == QXmlStreamReader::StartElement && tag == tagName(tag_meta)) {
			parseMetadata(reader, pageElement);
		}
		// e.g. <Page imageFilename="00001234.tif" imageWidth="1000" imageHeight="2000">
		else if (reader.tokenType() == QXmlStreamReader::StartElement && tag == tagName(tag_page)) {


			pageElement->setImageFileName(reader.attributes().value(tagName(attr_imageFilename)).toString());

			bool wok = false, hok = false;
			int width = reader.attributes().value(tagName(attr_imageWidth)).toInt(&wok);
			int height = reader.attributes().value(tagName(attr_imageHeight)).toInt(&hok);

			if (wok & hok)
				pageElement->setImageSize(QSize(width, height));
			else
				qWarning() << "could not read image dimensions";
		}
		// e.g. <TextLine id="r1" type="heading">
		else if (reader.tokenType() == QXmlStreamReader::StartElement && rm.isValidTypeName(tag)) {
			
			parseRegion(reader, root);
		}

		reader.readNext();
	}

	pageElement->setRootRegion(root);

	//qDebug() << "---------------------------------------------------------\n" << *pageElement;
	qInfo() << xmlInfo.fileName() << "with" << root->children().size() << "elements parsed in" << dt;

	return pageElement;
}

/// <summary>
/// Parses all regions from a PAGE XML hierarchically.
/// </summary>
/// <param name="reader">The XML Reader.</param>
/// <param name="parent">The parent of the region which is parsed next.</param>
void PageXmlParser::parseRegion(QXmlStreamReader & reader, QSharedPointer<Region> parent) const {

	RegionManager& rm = RegionManager::instance();

	Region::Type rType = rm.type(reader.qualifiedName().toString());
	QSharedPointer<Region> region = rm.createRegion(rType);
	
	// add region attributes
	region->setType(rType);
	region->setId(reader.attributes().value(RegionXmlHelper::instance().tag(RegionXmlHelper::attr_id)).toString());
	parent->addChild(region);

	// TODO add type to text regions
	//region->setTextType(reader.attributes().value(tagName(attr_text_type)).toString());
	bool readNextLine = true;

	while (!reader.atEnd()) {
		
		if (readNextLine)
			reader.readNext();
		else
			reader.readNextStartElement();

		QString tag = reader.qualifiedName().toString();

		// are we done here?
		if (reader.tokenType() == QXmlStreamReader::EndElement && rm.isValidTypeName(tag))
			break;

		// append children?!
		if (reader.tokenType() == QXmlStreamReader::StartElement && rm.isValidTypeName(tag)) 
			parseRegion(reader, region);
		else
			readNextLine = region->read(reader);	// present current line to the region
	}

}

/// <summary>
/// Parses the metadata of a PAGE XML.
/// </summary>
/// <param name="reader">The XML reader.</param>
/// <param name="page">The page element.</param>
void PageXmlParser::parseMetadata(QXmlStreamReader & reader, QSharedPointer<PageElement> page) const {

	// That's what we expect here:
	//	<Metadata>
	//	<Creator>TRP</Creator>
	//	<Created>2015-03-26T12:13:19.933+01:00</Created>
	//	<LastChange>2016-01-13T08:59:18.921+01:00</LastChange>
	//	</Metadata>
	while (!reader.atEnd()) {

		reader.readNext();
		QString tag = reader.qualifiedName().toString();

		// are we done?
		if (reader.tokenType() == QXmlStreamReader::EndElement && tag == tagName(tag_meta))
			break;

		if (reader.tokenType() == QXmlStreamReader::StartElement && tag == tagName(tag_meta_created)) {
			reader.readNext();
			page->setDateCreated(QDateTime::fromString(reader.text().toString(), Qt::ISODate));
		}
		else if (reader.tokenType() == QXmlStreamReader::StartElement && tag == tagName(tag_meta_changed)) {
			reader.readNext();
			page->setDateModified(QDateTime::fromString(reader.text().toString(), Qt::ISODate));
		}
		else if (reader.tokenType() == QXmlStreamReader::StartElement && tag == tagName(tag_meta_creator)) {
			reader.readNext();
			page->setCreator(reader.text().toString());
		}

	}
}

QByteArray PageXmlParser::writePageElement() const {

	if (!mPage) {
		qWarning() << "Cannot write XML if page is NULL";
		return QByteArray();
	}

	QByteArray ba;
	QBuffer buffer(&ba);
	buffer.open(QIODevice::WriteOnly);


	QXmlStreamWriter writer(&buffer);
	writer.setAutoFormatting(true);
	writer.writeStartDocument();

	// <PcGts>
	writer.writeStartElement(tagName(tag_root));
	writer.writeAttribute(tagName(attr_xmlns), "http://schema.primaresearch.org/PAGE/gts/pagecontent/2013-07-15");
	writer.writeAttribute(tagName(attr_xsi), "http://www.w3.org/2001/XMLSchema-instance");
	writer.writeAttribute(tagName(attr_schemaLocation), "http://schema.primaresearch.org/PAGE/gts/pagecontent/2013-07-15 http://schema.primaresearch.org/PAGE/gts/pagecontent/2013-07-15/pagecontent.xsd");

	// <Metadata>
	writeMetaData(writer);

	// <Page>
	writer.writeStartElement(tagName(tag_page));
	writer.writeAttribute(tagName(attr_imageFilename), mPage->imageFileName());
	writer.writeAttribute(tagName(attr_imageWidth), QString::number(mPage->imageSize().width()));
	writer.writeAttribute(tagName(attr_imageHeight), QString::number(mPage->imageSize().height()));

	QSharedPointer<Region> root = mPage->rootRegion();

	for (const QSharedPointer<Region> r : root->children()) {
		r->write(writer);
	}

	// close
	writer.writeEndElement();	// </Page>
	writer.writeEndElement();	// </PcGts>
	writer.writeEndDocument();

	return ba;
}

void PageXmlParser::writeMetaData(QXmlStreamWriter& writer) const {

	writer.writeStartElement(tagName(tag_meta));
	
	writer.writeTextElement(tagName(tag_meta_creator), mPage->creator());
	writer.writeTextElement(tagName(tag_meta_created), mPage->dateCreated().toString(Qt::ISODate));
	writer.writeTextElement(tagName(tag_meta_changed), mPage->dateModified().toString(Qt::ISODate));

	writer.writeEndElement();	// <Metadata>

}


QString PageXmlParser::imagePathToXmlPath(const QString& path) {

	// TODO: add optional xml search/save path
	QFileInfo info(path);
	QString xmlDir = info.absolutePath() + QDir::separator() + Config::instance().global().xmlSubDir;
	QString xmlFileName = info.fileName().replace(info.suffix(), "xml");

	QString xmlPath = QFileInfo(xmlDir, xmlFileName).absoluteFilePath();

	return xmlPath;
}

}