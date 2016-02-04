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
#include <QUUid>
#pragma warning(pop)

namespace rdf {

// Region --------------------------------------------------------------------
Region::Region() {
	mId = QUuid::createUuid().toString();
}

QDataStream& operator<<(QDataStream& s, const Region& r) {

	// this makes the operator<< virtual (stroustrup)
	s << r.toString(false);	// for now show children too
	return s;
}

QDebug operator<<(QDebug d, const Region& r) {

	d << qPrintable(r.toString(false));
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
	//msg += "\tpoly: " + QString::number(mPoly.size());
	msg += "\tkids: " + QString::number(mChildren.size());

	if (withChildren)
		msg += childrenToString();

	return msg;
}

QString Region::childrenToString() const {
	
	QString msg;

	for (const QSharedPointer<Region> region : mChildren) {
		msg += "\n";
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

	RegionXmlHelper& rm = RegionXmlHelper::instance();

	// append children?!
	if (reader.tokenType() == QXmlStreamReader::StartElement && reader.qualifiedName().toString() == rm.tag(RegionXmlHelper::tag_coords)) {
		mPoly.read(reader.attributes().value(rm.tag(RegionXmlHelper::attr_points)).toString());
	}
	else if (reader.tokenType() == QXmlStreamReader::StartElement && reader.qualifiedName().toString() == rm.tag(RegionXmlHelper::tag_coords)) {

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

	RegionXmlHelper& rm = RegionXmlHelper::instance();

	// read <TextEquiv>
	if (reader.tokenType() == QXmlStreamReader::StartElement && reader.qualifiedName() == rm.tag(RegionXmlHelper::tag_text_equiv)) {

		while (!reader.atEnd()) {
			reader.readNext();

			// are we done with reading the text?
			if (reader.tokenType() == QXmlStreamReader::EndElement && reader.qualifiedName() == rm.tag(RegionXmlHelper::tag_text_equiv))
				break;

			// read unicode
			if (reader.tokenType() == QXmlStreamReader::StartElement && reader.qualifiedName() == rm.tag(RegionXmlHelper::tag_unicode)) {
				reader.readNext();
				mText = reader.text().toUtf8();	// add text
			}
			// read ASCII
			if (reader.tokenType() == QXmlStreamReader::StartElement && reader.qualifiedName() == rm.tag(RegionXmlHelper::tag_plain_text)) {
				reader.readNext();
				mText = reader.text().toString();	// add text
			}
		}
	}
	// read baseline
	else if (reader.tokenType() == QXmlStreamReader::StartElement && reader.qualifiedName().toString() == rm.tag(RegionXmlHelper::tag_baseline)) {
		mBaseLine.read(reader.attributes().value(rm.tag(RegionXmlHelper::attr_points)).toString());
	}
	else
		return Region::read(reader);

	return true;
}

QString TextLine::toString(bool withChildren) const {
	
	QString msg = Region::toString(false);

	if (!mText.isEmpty()) {
		msg += " | text: ";
		msg += mText;
	}

	if (withChildren)
		msg += Region::childrenToString();

	return msg;
}

// RegionXmlHelper --------------------------------------------------------------------
RegionXmlHelper::RegionXmlHelper() {

	mTags = createTags();
}

RegionXmlHelper& RegionXmlHelper::instance() {

	static QSharedPointer<RegionXmlHelper> inst;
	if (!inst)
		inst = QSharedPointer<RegionXmlHelper>(new RegionXmlHelper());
	return *inst;
}

QStringList RegionXmlHelper::createTags() const {

	QStringList tn;
	for (int idx = 0; idx < XmlTags::tag_end; idx++)
		tn.append(tag((XmlTags) idx));

	return tn;
}

QString RegionXmlHelper::tag(const XmlTags& tagId) const {

	switch (tagId) {
	case tag_coords:		return "Coords";
	case tag_text_equiv:	return "TextEquiv";
	case tag_unicode:		return "Unicode";
	case tag_plain_text:	return "PlainText";
	case tag_baseline:		return "BaseLine";

	case attr_points:		return "points";
	}

	qWarning() << "unknown tag: " << tagId;
	return "";
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
	case Region::type_root:			return "Root";
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
	mCreator = "CVL";
	mDateCreated = QDateTime::currentDateTime();
	mDateModified = QDateTime::currentDateTime();
}

QDebug operator<<(QDebug d, const PageElement& p) {

	d << qPrintable(p.toString());
	return d;
}

QDataStream& operator<<(QDataStream& s, const PageElement& p) {

	s << p.toString();	// for now show children too
	return s;
}

QString PageElement::toString() const {

	QString msg = xmlPath();
	msg += "\nCreator:\t" + mCreator;
	msg += "\nCreated on:\t" + mDateCreated.toLocalTime().toString(Qt::SystemLocaleShortDate);
	if (mDateCreated != mDateModified)
		msg += "\nModified on:\t" + mDateCreated.toLocalTime().toString(Qt::SystemLocaleShortDate);

	msg += "\n";

	if (mRoot)
		msg += mRoot->toString(true);

	return msg;
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

void PageElement::setCreator(const QString & creator) {
	mCreator = creator;
}

QString PageElement::creator() const {
	return mCreator;
}

void PageElement::setDateCreated(const QDateTime & date) {
	mDateCreated = date;
}

QDateTime PageElement::dateCreated() const {
	return mDateCreated;
}

void PageElement::setDateModified(const QDateTime & date) {
	mDateModified = date;
}

QDateTime PageElement::dateModified() const {
	return mDateModified;
}



}