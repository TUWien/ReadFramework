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

QString Region::typeName(const Region::Type& type) const {
	
	switch (type) {
	case type_unknown:		return "Unknown";
	case type_text_region:	return "TextRegion";
	case type_text_line:	return "TextLine";
	case type_word:			return "Word";
	case type_separator:	return "Separator";
	case type_image:		return "ImageRegion";
	case type_graphic:		return "GraphicRegion";
	case type_noise:		return "NoiseRegion";
	}

	return "Unknown";
}

QStringList Region::typeNames() const {

	QStringList tn;
	for (int idx = 0; idx < type_end; idx++)
		tn.append(typeName((Region::Type) idx));

	return tn;
}

void Region::setType(const QString& typeName) {

	QStringList tns = typeNames();
	int typeIdx = tns.indexOf(typeName);

	if (typeIdx != -1)
		mType = (Region::Type) typeIdx;
	else
		qWarning() << "Unknown type: " << typeName;

}

QDataStream& operator<<(QDataStream& s, const Region& r) {

	// this makes the operator<< virtual (stroustrup)
	s << r.toString();
	return s;
}

QDebug operator<<(QDebug d, const Region& r) {

	d << qPrintable(r.toString());
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

QVector<QSharedPointer<Region> > Region::children() const {
	return mChildren;
}

QString Region::toString() const {

	QString msg;
	msg += "[" + typeName(mType) + "] ";
	msg += "ID: " + mId;
	msg += " poly: " + QString::number(mPoly.size());

	return msg;
}

void Region::read(QXmlStreamReader & reader) {

	QString tagCoords = "Coords";
	QString tagPoints = "points";

	// append children?!
	if (reader.tokenType() == QXmlStreamReader::StartElement && reader.qualifiedName().toString() == tagCoords) {
		mPoly.read(reader.attributes().value(tagPoints).toString());
	}
	else if (reader.tokenType() == QXmlStreamReader::StartElement && reader.qualifiedName().toString() == tagCoords) {

	}
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