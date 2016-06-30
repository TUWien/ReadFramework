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
/// <summary>
/// Initializes a new instance of the <see cref="Region"/> class.
/// The Region is the base class for all Region elements that 
/// are contained in the extended PAGE XML.
/// </summary>
Region::Region() {
	mId = QUuid::createUuid().toString();
}

/// <summary>
/// Writes the Region r to the data stream s
/// </summary>
/// <param name="s">A data stream.</param>
/// <param name="r">A region r.</param>
/// <returns>The data stream with the current region.</returns>
QDataStream& operator<<(QDataStream& s, const Region& r) {

	// this makes the operator<< virtual (stroustrup)
	s << r.toString(false);	// for now show children too
	return s;
}

/// <summary>
/// Prints the Region r to the debug output.
/// </summary>
/// <param name="d">A Debug output.</param>
/// <param name="r">The Region to be printed.</param>
/// <returns>The debug output with the current Region.</returns>
QDebug operator<<(QDebug d, const Region& r) {

	d << qPrintable(r.toString(false));
	return d;
}

/// <summary>
/// Sets the Region type.
/// </summary>
/// <param name="type">The type.</param>
void Region::setType(const Region::Type & type) {
	mType = type;
}

/// <summary>
/// The Region's type (e.g. type_text_region).
/// </summary>
/// <returns></returns>
Region::Type Region::type() const {
	return mType;
}

/// <summary>
/// Set a unique identifier to the region.
/// </summary>
/// <param name="id">A unique identifier.</param>
void Region::setId(const QString & id) {
	mId = id;
}

/// <summary>
/// Returns the Region's unique identifier.
/// </summary>
/// <returns></returns>
QString Region::id() const {
	return mId;
}

/// <summary>
/// Set the polygon which enclosed the Region.
/// The polygon's coordinates are w.r.t the image coordinates.
/// </summary>
/// <param name="polygon">A polygon that represents the Region.</param>
void Region::setPolygon(const Polygon & polygon) {
	mPoly = polygon;
}

/// <summary>
/// The Region's polygon.
/// </summary>
/// <returns></returns>
Polygon Region::polygon() const {
	return mPoly;
}

/// <summary>
/// Draws the Region to the Painter.
/// </summary>
/// <param name="p">The painter.</param>
/// <param name="config">The configuration containing colors etc.</param>
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

/// <summary>
/// Adds a child to the current Region.
/// </summary>
/// <param name="child">The child region.</param>
void Region::addChild(QSharedPointer<Region> child) {
	mChildren.append(child);
}

/// <summary>
/// Removes the child specified.
/// </summary>
/// <param name="child">The child region.</param>
void Region::removeChild(QSharedPointer<Region> child) {
	
	int idx = mChildren.indexOf(child);
	
	if (idx != -1)
		mChildren.remove(idx);
	else
		qWarning() << "cannot remove" << child;
}

/// <summary>
/// Sets the child regions.
/// </summary>
/// <param name="children">The child regions.</param>
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
/// Returns a string discribing the Region.
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

/// <summary>
/// Returns a string with all attributes of the Region's children.
/// </summary>
/// <returns></returns>
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
	//else if (reader.tokenType() == QXmlStreamReader::StartElement && reader.qualifiedName().toString() == rm.tag(RegionXmlHelper::tag_coords)) {

	//}
	// report unknown tags
	else if (reader.tokenType() == QXmlStreamReader::StartElement)
		return false;

	return true;
}

/// <summary>
/// Writes the Region to the XML stream.
/// </summary>
/// <param name="writer">The XML stream at the position where the region should be written.</param>
/// <param name="withChildren">if set to <c>true</c> the Region's children are written too.</param>
/// <param name="close">if set to <c>true</c> the element's close tag is appended.</param>
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

/// <summary>
/// Writes the Region's children to the XML stream.
/// </summary>
/// <param name="writer">The XML stream.</param>
void Region::writeChildren(QXmlStreamWriter& writer) const {

	for (const QSharedPointer<Region> child : mChildren)
		child->write(writer, true);
}

// TextLine --------------------------------------------------------------------
/// <summary>
/// Initializes a new instance of the <see cref="TextLine"/> class.
/// This class represents Text lines (Region::type_text_line).
/// </summary>
TextLine::TextLine() : Region() {
	mType = Region::type_text_line;
}

/// <summary>
/// Set the BaseLine.
/// </summary>
/// <param name="baseLine">The Textline's base line.</param>
void TextLine::setBaseLine(const BaseLine & baseLine) {
	mBaseLine = baseLine;
}

/// <summary>
/// Returns the Textline's Baseline.
/// </summary>
/// <returns></returns>
BaseLine TextLine::baseLine() const {
	return mBaseLine;
}

/// <summary>
/// Set the GT/HTR text of the TextLine.
/// </summary>
/// <param name="text">The text corresponding to the textline.</param>
void TextLine::setText(const QString & text) {
	mText = text;
}

/// <summary>
/// GT/HTR Text of the Textline.
/// </summary>
/// <returns></returns>
QString TextLine::text() const {
	return mText;
}

/// <summary>
/// Reads a Textline from the XML stream.
/// The stream must be at the position of the 
/// Textline's tag and is forwarded until its end
/// </summary>
/// <param name="reader">The XML stream.</param>
/// <returns>true on success</returns>
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

/// <summary>
/// Writes the TextLine instance to the XML stream.
/// </summary>
/// <param name="writer">The XML stream.</param>
/// <param name="withChildren">if set to <c>true</c> the TextLine's children are written to the XML.</param>
/// <param name="close">if set to <c>true</c> the TextLine's end element is written.</param>
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


/// <summary>
/// Returns a string with all important properties of the TextLine.
/// </summary>
/// <param name="withChildren">if set to <c>true</c> children properties are written too.</param>
/// <returns></returns>
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

/// <summary>
/// Draws the TextLine to the Painter.
/// </summary>
/// <param name="p">The painter.</param>
/// <param name="config">The configuration (e.g. color of the Region).</param>
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
/// <summary>
/// Initializes a new instance of the <see cref="PageElement"/> class.
/// This class corresponds to a PAGE XML file. It hold's the XML file's path.
/// After parsing the PAGE XML, the rootRegion() should contain all relevant
/// regions.
/// </summary>
/// <param name="xmlPath">The XML path.</param>
PageElement::PageElement(const QString& xmlPath) {
	
	mXmlPath = xmlPath;
	mCreator = "CVL";
	mDateCreated = QDateTime::currentDateTime();
	mDateModified = QDateTime::currentDateTime();
}

/// <summary>
/// Prints the page element to the debug stream.
/// </summary>
/// <param name="d">The debug stream.</param>
/// <param name="p">The page element.</param>
/// <returns></returns>
QDebug operator<<(QDebug d, const PageElement& p) {

	d << qPrintable(p.toString());
	return d;
}

/// <summary>
/// Appends the page element to the data stream s.
/// </summary>
/// <param name="s">The data stream.</param>
/// <param name="p">The page element.</param>
/// <returns></returns>
QDataStream& operator<<(QDataStream& s, const PageElement& p) {

	s << p.toString();	// for now show children too
	return s;
}

/// <summary>
/// Creates a string with all relevant attributes of the current instance.
/// </summary>
/// <returns></returns>
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

/// <summary>
/// Set the XML path.
/// </summary>
/// <param name="xmlPath">The XML path.</param>
void PageElement::setXmlPath(const QString & xmlPath) {
	mXmlPath = xmlPath;
}

/// <summary>
/// The PAGE XML path.
/// </summary>
/// <returns></returns>
QString PageElement::xmlPath() const {
	return mXmlPath;
}

/// <summary>
/// Set the filename of the corresponding image.
/// </summary>
/// <param name="name">The image's filename.</param>
void PageElement::setImageFileName(const QString & name) {
	mImageFileName = name;
}

/// <summary>
/// Returns the filename of the corresponding image.
/// </summary>
/// <returns></returns>
QString PageElement::imageFileName() const {
	return mImageFileName;
}

/// <summary>
/// Set the size of the image.
/// </summary>
/// <param name="size">The image size.</param>
void PageElement::setImageSize(const QSize & size) {
	mImageSize = size;
}

/// <summary>
/// The image size.
/// </summary>
/// <returns></returns>
QSize PageElement::imageSize() const {
	return mImageSize;
}

/// <summary>
/// Set the root region. This is needed for writing to a PAGE XML file.
/// </summary>
/// <param name="region">The root region.</param>
void PageElement::setRootRegion(QSharedPointer<Region> region) {
	mRoot = region;
}

/// <summary>
/// Returns the root region. This is needed if the XML is read.
/// </summary>
/// <returns></returns>
QSharedPointer<Region> PageElement::rootRegion() const {
	return mRoot;
}

/// <summary>
/// Set the creator of the PAGE XML.
/// </summary>
/// <param name="creator">The PAGE creator (CVL if we write).</param>
void PageElement::setCreator(const QString & creator) {
	mCreator = creator;
}

/// <summary>
/// The PAGE XML creator.
/// </summary>
/// <returns></returns>
QString PageElement::creator() const {
	return mCreator;
}

/// <summary>
/// Set the date created of the PAGE XML.
/// </summary>
/// <param name="date">The date created.</param>
void PageElement::setDateCreated(const QDateTime & date) {
	mDateCreated = date;
}

/// <summary>
/// Returns the date created of the PAGE XML.
/// </summary>
/// <returns></returns>
QDateTime PageElement::dateCreated() const {
	return mDateCreated;
}

/// <summary>
/// Set the date modified of the PAGE XML.
/// </summary>
/// <param name="date">The last modified date of the PAGE XML.</param>
void PageElement::setDateModified(const QDateTime & date) {
	mDateModified = date;
}

/// <summary>
/// The modified date of the PAGE XML.
/// </summary>
/// <returns></returns>
QDateTime PageElement::dateModified() const {
	return mDateModified;
}



SeparatorRegion::SeparatorRegion() : Region() {
	mType = type_separator;
}

void SeparatorRegion::setLine(const Line & line) {

	QPolygon qlp;
	qlp << line.startPoint() << line.endPoint();
	Polygon p(qlp);

	setPolygon(p);
	mLine = line;

}

Line SeparatorRegion::line() const {
	
	if (mLine.isEmpty())
		return Line(mPoly);
	
	return mLine;
}

}