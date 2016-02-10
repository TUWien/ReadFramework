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
#include "ElementsHelper.h"

#pragma warning(push, 0)	// no warnings from includes
#include <QDebug>
#include <QXmlStreamReader>
#include <QUuid>
#include <QPainter>
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

void Region::draw(QPainter& p, const RegionTypeConfig& config) const {

	p.setPen(config.pen());

	// draw polygon
	if (config.drawPoly() && !polygon().isEmpty()) {
		
		QPainterPath path;
		path.addPolygon(polygon().closedPolygon());

		p.drawPath(path);
		p.fillPath(path, config.brush());
	}

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
/// Children of this instance.
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

void Region::write(QXmlStreamWriter& writer, bool withChildren, bool close) const {

	RegionXmlHelper& rm = RegionXmlHelper::instance();

	writer.writeStartElement(RegionManager::instance().typeName(mType));
	writer.writeAttribute(rm.tag(RegionXmlHelper::attr_id), mId);
	
	// write polygon
	writer.writeStartElement(rm.tag(RegionXmlHelper::tag_coords));
	writer.writeAttribute(rm.tag(RegionXmlHelper::attr_points), mPoly.write());
	writer.writeEndElement();	// <Coords>

	if (withChildren)
		writeChildren(writer);

	if (close)
		writer.writeEndElement();	// <Type>
}

void Region::writeChildren(QXmlStreamWriter& writer) const {

	for (const QSharedPointer<Region> child : mChildren)
		child->write(writer, true);
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
				mText = reader.text().toUtf8().trimmed();	// add text
			}
			// read ASCII
			if (reader.tokenType() == QXmlStreamReader::StartElement && reader.qualifiedName() == rm.tag(RegionXmlHelper::tag_plain_text)) {
				reader.readNext();
				mText = reader.text().toString().trimmed();	// add text
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

void TextLine::write(QXmlStreamWriter & writer, bool withChildren, bool close) const {

	RegionXmlHelper& rm = RegionXmlHelper::instance();
	Region::write(writer, false, false);

	if (!mBaseLine.isEmpty()) {
		writer.writeStartElement(rm.tag(RegionXmlHelper::tag_baseline));
		writer.writeAttribute(rm.tag(RegionXmlHelper::attr_points), mBaseLine.write());
		writer.writeEndElement(); // </Baseline>
	}

	if (!mText.isEmpty()) {
		writer.writeStartElement(rm.tag(RegionXmlHelper::tag_text_equiv));
		writer.writeTextElement(rm.tag(RegionXmlHelper::tag_unicode), mText);
		writer.writeEndElement(); // </TextEquiv>
	}

	if (withChildren)
		writeChildren(writer);

	if (close)
		writer.writeEndElement(); // </Region>
}


QString TextLine::toString(bool withChildren) const {
	
	QString msg = Region::toString(false);

	if (!mText.isEmpty()) {
		msg += " | baseline: " + QString::number(mBaseLine.polygon().size());
		msg += " | text: ";
		msg += mText;
	}

	if (withChildren)
		msg += Region::childrenToString();

	return msg;
}

void TextLine::draw(QPainter & p, const RegionTypeConfig & config) const {

	Region::draw(p, config);

	if (config.drawBaseline() && !mBaseLine.isEmpty()) {
		
		QPolygon poly = mBaseLine.polygon();
		
		QPainterPath path;
		path.addPolygon(poly);
		p.drawPath(path);
	}

	if (config.drawText() && !mText.isEmpty()) {
		
		QPoint sp = mBaseLine.startPoint();

		if (sp.isNull() && !mPoly.isEmpty()) {
			sp = mPoly.polygon().first();
			p.drawText(sp, text());
		}
		else
			qDebug() << "could not draw text: region has no polygon";
	}

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