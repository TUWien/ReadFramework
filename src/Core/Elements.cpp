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

#include "Elements.h"

#pragma warning(push, 0)	// no warnings from includes
#include <QDebug>
#include <QXmlStreamReader>
#pragma warning(pop)

namespace rdf {

// Region --------------------------------------------------------------------
Region::Region() {
	
}

QDataStream& operator<<(QDataStream& s, const Region& r) {

	// this makes the operator<< virtual (stroustrup)
	s << r.toString(true);	// for now show children too
	return s;
}

QDebug operator<<(QDebug d, const Region& r) {

	d << qPrintable(r.toString(true));
	return d;
}


void Region::setType(const Region::Type & type) {
	mType = type;
}

Region::Type Region::type() const {
	return mType;
}

void Region::setId(const QString & id) {
	mId = id;
}

QString Region::id() const {
	return mId;
}

void Region::setPoly(const Polygon & polygon) {
	mPoly = polygon;
}

Polygon Region::polygon() const {
	return mPoly;
}

void Region::addChild(QSharedPointer<Region> child) {
	mChildren.append(child);
}

void Region::removeChild(QSharedPointer<Region> child) {
	
	int idx = mChildren.indexOf(child);
	
	if (idx != -1)
		mChildren.remove(idx);
	else
		qWarning() << "cannot remove" << child;
}

void Region::setChildren(const QVector<QSharedPointer<Region>>& children) {
	mChildren = children;
}

/// <summary>
/// Childrens this instance.
/// </summary>
/// <returns></returns>
QVector<QSharedPointer<Region> > Region::children() const {
	return mChildren;
}

/// <summary>
/// Returns a string discribing the current Region.
/// </summary>
/// <returns></returns>
QString Region::toString(bool withChildren) const {

	QString msg;
	msg += "[" + RegionManager::instance().typeName(mType) + "] ";
	msg += "ID: " + mId;
	msg += "\tpoly: " + QString::number(mPoly.size());

	if (withChildren)
		msg += childrenToString();

	return msg;
}

QString Region::childrenToString() const {
	
	QString msg;

	for (const QSharedPointer<Region> region : mChildren) {
		msg += "\n  ";
		msg += region->toString(true);
	}

	return msg;
}

/// <summary>
/// Adds Attributes to the current region.
/// </summary>
/// <param name="reader">XML reader set to the current line (inside a region type).</param>
/// <returns>false if an opening tag is unknown</returns>
bool Region::read(QXmlStreamReader & reader) {

	QString tagCoords = "Coords";
	QString tagPoints = "points";

	// append children?!
	if (reader.tokenType() == QXmlStreamReader::StartElement && reader.qualifiedName().toString() == tagCoords) {
		mPoly.read(reader.attributes().value(tagPoints).toString());
	}
	else if (reader.tokenType() == QXmlStreamReader::StartElement && reader.qualifiedName().toString() == tagCoords) {

	}
	// report unknown tags
	else if (reader.tokenType() == QXmlStreamReader::StartElement)
		return false;

	return true;
}

// TextLine --------------------------------------------------------------------
TextLine::TextLine() : Region() {
	mType = Region::type_text_line;
}

void TextLine::setBaseLine(const BaseLine & baseLine) {
	mBaseLine = baseLine;
}

BaseLine TextLine::baseLine() const {
	return mBaseLine;
}

void TextLine::setText(const QString & text) {
	mText = text;
}

QString TextLine::text() const {
	return mText;
}

bool TextLine::read(QXmlStreamReader & reader) {

	QString tagText		= "TextEquiv";
	QString tagUnicode	= "Unicode";
	QString tagPlainText= "PlainText";

	if (reader.tokenType() == QXmlStreamReader::StartElement && reader.qualifiedName() == tagText) {

		while (!reader.atEnd()) {
			reader.readNext();

			// are we done with reading the text?
			if (reader.tokenType() == QXmlStreamReader::EndElement && reader.qualifiedName() == tagText)
				break;

			// read unicode
			if (reader.tokenType() == QXmlStreamReader::StartElement && reader.qualifiedName() == tagUnicode) {
				reader.readNext();
				mText = reader.text().toUtf8();	// add text
			}
			// read ASCII
			if (reader.tokenType() == QXmlStreamReader::StartElement && reader.qualifiedName() == tagPlainText) {
				reader.readNext();
				mText = reader.text().toString();	// add text
			}
		}

	}
	else
		return Region::read(reader);
}

QString TextLine::toString(bool withChildren) const {
	
	QString msg = Region::toString(false);

	if (!mText.isNull()) {
		msg += " | text: ";
		msg += mText;
	}

	if (withChildren)
		msg += Region::childrenToString();

	return msg;
}

// RegionManager --------------------------------------------------------------------
RegionManager::RegionManager() {

	mTypeNames = createTypeNames();
}

RegionManager& RegionManager::instance() {

	static QSharedPointer<RegionManager> inst;
	if (!inst)
		inst = QSharedPointer<RegionManager>(new RegionManager());
	return *inst;
}


QString RegionManager::typeName(const Region::Type& type) const {

	switch (type) {
	case Region::type_unknown:		return "Unknown";
	case Region::type_text_region:	return "TextRegion";
	case Region::type_text_line:	return "TextLine";
	case Region::type_word:			return "Word";
	case Region::type_separator:	return "Separator";
	case Region::type_image:		return "ImageRegion";
	case Region::type_graphic:		return "GraphicRegion";
	case Region::type_noise:		return "NoiseRegion";
	}

	return "Unknown";
}

QStringList RegionManager::createTypeNames() const {

	QStringList tn;
	for (int idx = 0; idx < Region::type_end; idx++)
		tn.append(typeName((Region::Type) idx));

	return tn;
}

QStringList RegionManager::typeNames() const {
	return mTypeNames;
}

bool RegionManager::isValidTypeName(const QString & typeName) const {

	return mTypeNames.contains(typeName);
}

QSharedPointer<Region> RegionManager::createRegion(const Region::Type & type) const {

	switch (type) {
	case Region::type_text_line:
	case Region::type_word:
		return QSharedPointer<TextLine>(new TextLine());
		break;
		// Add new types here...
	}

	return QSharedPointer<Region>(new Region());
}

Region::Type RegionManager::type(const QString& typeName) const {

	QStringList tns = typeNames();
	int typeIdx = tns.indexOf(typeName);

	if (typeIdx != -1)
		return (Region::Type) typeIdx;
	else
		qWarning() << "Unknown type: " << typeName;

	return Region::type_unknown;
}

// PageElement --------------------------------------------------------------------
PageElement::PageElement(const QString& xmlPath) {
	mXmlPath = xmlPath;
}

void PageElement::setXmlPath(const QString & xmlPath) {
	mXmlPath = xmlPath;
}

QString PageElement::xmlPath() const {
	return mXmlPath;
}

void PageElement::setImageFileName(const QString & name) {
	mImageFileName = name;
}

QString PageElement::imageFileName() const {
	return mImageFileName;
}

void PageElement::setImageSize(const QSize & size) {
	mImageSize = size;
}

QSize PageElement::imageSize() const {
	return mImageSize;
}

void PageElement::setRootRegion(QSharedPointer<Region> region) {
	mRoot = region;
}

QSharedPointer<Region> PageElement::rootRegion() const {
	return mRoot;
}



}