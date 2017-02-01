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
#include "Utils.h"

#pragma warning (disable: 4714)	// force inline

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
Region::Region(const Type& type) {
	mId = "CVL-" + QUuid::createUuid().toString();
	mType = type;
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

void Region::setSelected(bool select) {
	mSelected = select;
}

bool Region::selected() const {
	return mSelected;
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

void Region::setCustom(const QString & c) {
	mCustom = c;
}

QString Region::custom() const {
	return mCustom;
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

	QPen cp = config.pen();
	
	if (selected()) {
		cp.setStyle(Qt::DashLine);
		qDebug() << "should be dashed...";
	}

	p.setPen(cp);

	// draw polygon
	if (config.drawPoly() && !polygon().isEmpty()) {
		
		QPainterPath path;
		path.addPolygon(polygon().closedPolygon());

		p.fillPath(path, config.brush());
		p.drawPath(path);
	}

}

/// <summary>
/// Adds a child to the current Region.
/// </summary>
/// <param name="child">The child region.</param>
void Region::addChild(QSharedPointer<Region> child) {
	mChildren.append(child);
}

void Region::addUniqueChild(QSharedPointer<Region> child) {

	if (!child) {
		qWarning() << "addUniqueChild: child is NULL where it should not be";
		return;
	}

	bool containsChild = false;
	for (const QSharedPointer<Region> i  : mChildren) {
		if (i && *i == *child) {
			containsChild = true;
			break;
		}
	}

	if (!containsChild)
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

QVector<QSharedPointer<Region> > Region::filter(QSharedPointer<Region> root, const Region::Type& type) {

	QVector<QSharedPointer<Region> > regions;

	if (root)
		root->collectRegions(regions, type);

	return regions;

}

QVector<QSharedPointer<Region>> Region::allRegions(QSharedPointer<Region> root) {

	QVector<QSharedPointer<Region> > regions;

	if (root)
		root->collectRegions(regions);

	return regions;
}

void Region::collectRegions(QVector<QSharedPointer<Region> >& regions, const Region::Type& type) const {

	for (auto c : children())
		if (type == type_unknown || c->type() == type)
			regions << c;

	for (auto c : children())
		c->collectRegions(regions, type);
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
		
		QString pts = reader.attributes().value(rm.tag(RegionXmlHelper::attr_points)).toString();
		if (!pts.isEmpty()) {
			mPoly.read(pts);
		}
		// fallback to old point coordinates
		else {
			readPoints(reader);
		}
	}
	//else if (reader.tokenType() == QXmlStreamReader::StartElement && reader.qualifiedName().toString() == rm.tag(RegionXmlHelper::tag_coords)) {

	//}
	else if (reader.tokenType() == QXmlStreamReader::StartElement)
		return false;

	return true;
}

void Region::readAttributes(QXmlStreamReader & reader) {
	
	setId(reader.attributes().value(RegionXmlHelper::instance().tag(RegionXmlHelper::attr_id)).toString());
	setCustom(reader.attributes().value(RegionXmlHelper::instance().tag(RegionXmlHelper::attr_custom)).toString());
}

bool Region::readPoints(QXmlStreamReader & reader) {

	RegionXmlHelper& rm = RegionXmlHelper::instance();
	QPolygonF poly;

	while (!reader.atEnd()) {
		reader.readNext();

		if (reader.tokenType() == QXmlStreamReader::StartElement && reader.qualifiedName().toString() == rm.tag(RegionXmlHelper::tag_point)) {
			
			// parse x
			bool ok = false;
			QString str = reader.attributes().value("x").toString();
			int x = str.toInt(&ok);

			if (!ok) {
				qWarning() << "could not parse coordinate: " << str;
				continue;
			}

			// parse y
			str = reader.attributes().value("y").toString();
			int y = str.toInt(&ok);

			if (!ok) {
				qWarning() << "could not parse coordinate: " << str;
				continue;
			}

			poly << QPointF(x, y);
		}

		// are we done?
		if (reader.tokenType() == QXmlStreamReader::EndElement && reader.qualifiedName().toString() == rm.tag(RegionXmlHelper::tag_coords))
			break;

	}

	mPoly.setPolygon(poly);

	return !poly.isEmpty();
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
	if (!mCustom.isEmpty())
		writer.writeAttribute(rm.tag(RegionXmlHelper::attr_custom), mCustom);

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

bool Region::operator==(const Region & r1) {

	QPolygonF p1 = r1.polygon().polygon();
	QPolygonF p2 = mPoly.polygon();

	if (p1.isEmpty() || p2.isEmpty())
		return false;

	if (p1 == p2)
		return true;
	else
		return false;
}

// TextLine --------------------------------------------------------------------
/// <summary>
/// Initializes a new instance of the <see cref="TextLine"/> class.
/// This class represents Text lines (Region::type_text_line).
/// </summary>
TextLine::TextLine(const Type& type) : Region(type) {
	
	// default to text line
	if (mType == type_unknown)
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
				mTextPresent = true;
			}
			// read ASCII
			if (reader.tokenType() == QXmlStreamReader::StartElement && reader.qualifiedName() == rm.tag(RegionXmlHelper::tag_plain_text)) {
				reader.readNext();
				mText = reader.text().toString().trimmed();	// add text
				mTextPresent = true;
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

	if (!mText.isEmpty() || mTextPresent) {
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
		
		QPolygonF poly = mBaseLine.polygon();
		
		QPainterPath path;
		path.addPolygon(poly);
		p.drawPath(path);

		if (config.drawBaselineLimits()) {

			QBrush b = p.brush();				// backup
			QPen pen = p.pen();					// backup
			p.setBrush(pen.color().darker());	// we want the ends to be filled
			p.setPen(Qt::NoPen);

			// draw start & end points
			int r = config.pen().width();
			p.drawEllipse(mBaseLine.startPoint(), r, r);
			p.drawEllipse(mBaseLine.endPoint(), r, r);

			p.setBrush(b);						// reset
			p.setPen(pen);						// reset
		}

	}

	if (config.drawText() && !mText.isEmpty()) {
		
		QPointF sp = mBaseLine.startPoint();

		if (sp.isNull() && !mPoly.isEmpty()) {
			sp = mPoly.polygon().first();
			p.drawText(sp, text());
		}
		//else
		//	qDebug() << "could not draw text: region has no polygon";
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

	mRoot = mRoot.create();
}

bool PageElement::isEmpty() {
	return !mRoot || mXmlPath.isEmpty();
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



SeparatorRegion::SeparatorRegion(const Type& type) : Region(type) {
	
	if (mType == type_unknown)
		mType = type_separator;
}

void SeparatorRegion::setLine(const Line & line) {

	QLineF l = line.line();
	QPolygonF qlp;
	qlp << l.p1() << l.p2();
	Polygon p(qlp.toPolygon());

	setPolygon(p);
	mLine = line;

}

Line SeparatorRegion::line() const {
	
	if (mLine.isEmpty())
		return Line(mPoly);
	
	return mLine;
}

bool SeparatorRegion::operator==(const Region & sr1) {

	if (mPoly.size() != sr1.polygon().size())
		return false;

	Polygon p1 = sr1.polygon();
	Line l1(p1);

	//Line l1 = sr1.line();
	mLine = line();

	if (l1.isEmpty() || mLine.isEmpty())
		return false;

	if (l1.p1() == mLine.p1() && l1.p2() == mLine.p2())
		return true;
	else
		return false;

}

TableRegion::TableRegion(const Type & type) : Region(type) {

	// default to text line
	if (mType == type_unknown)
		mType = Region::type_table_region;
}

//bool TableRegion::read(QXmlStreamReader & reader) {
//	return false;
//}

void TableRegion::readAttributes(QXmlStreamReader & reader) {

	Region::readAttributes(reader);

	mRows = reader.attributes().value(RegionXmlHelper::instance().tag(RegionXmlHelper::attr_rows)).toInt();
	mCols = reader.attributes().value(RegionXmlHelper::instance().tag(RegionXmlHelper::attr_cols)).toInt();

	//if (rType == Region::type_table_region) {
	//	QSharedPointer<TableRegion> pT = region.dynamicCast<TableRegion>();
	//	pT->setRows(reader.attributes().value(RegionXmlHelper::instance().tag(RegionXmlHelper::attr_rows)).toInt());
	//	pT->setCols(reader.attributes().value(RegionXmlHelper::instance().tag(RegionXmlHelper::attr_cols)).toInt());
	//}
}

rdf::Line TableRegion::topBorder() const {
	if (mPoly.size() != 4)
		return rdf::Line();
	else {
		QPointF p1 = mPoly.polygon()[0];
		QPointF p2 = mPoly.polygon()[1];
		QPolygonF tmp;
		tmp << p1 << p2;
		return rdf::Line(rdf::Polygon(tmp));
	}
}

rdf::Line TableRegion::bottomBorder() const {
	if (mPoly.size() != 4)
		return rdf::Line();
	else {
		QPointF p1 = mPoly.polygon()[3];
		QPointF p2 = mPoly.polygon()[2];
		QPolygonF tmp;
		tmp << p1 << p2;
		return rdf::Line(rdf::Polygon(tmp));
	}
}

rdf::Line TableRegion::leftBorder() const {
	if (mPoly.size() != 4)
		return rdf::Line();
	else {
		QPointF p1 = mPoly.polygon()[0];
		QPointF p2 = mPoly.polygon()[3];
		QPolygonF tmp;
		tmp << p1 << p2;
		return rdf::Line(rdf::Polygon(tmp));
	}
}

rdf::Line TableRegion::rightBorder() const {
	if (mPoly.size() != 4)
		return rdf::Line();
	else {
		QPointF p1 = mPoly.polygon()[1];
		QPointF p2 = mPoly.polygon()[2];
		QPolygonF tmp;
		tmp << p1 << p2;
		return rdf::Line(rdf::Polygon(tmp));
	}
}

QPointF TableRegion::leftUpper() const {
	if (mPoly.size() != 4)
		return QPointF();
	else {
		return mPoly.polygon()[0];
	}
}

QPointF TableRegion::rightDown() const {
	if (mPoly.size() != 4)
		return QPointF();
	else {
		return mPoly.polygon()[2];
	}
}

QPointF TableRegion::leftUpperCorner() const {

	if (mPoly.size() != 4)
		return QPointF();
	else {
		QPointF lu = mPoly.polygon()[0];
		QPointF ru = mPoly.polygon()[1];
		QPointF ld = mPoly.polygon()[3];
		//check which coordinate is the leftmost (upperleft or downleft)
		return QPointF(lu.x() < ld.x() ? lu.x() : ld.x(), 
			lu.y() < ru.y() ? lu.y() : ru.y());
	}
}

QPointF TableRegion::rightDownCorner() const {

	if (mPoly.size() != 4)
		return QPointF();
	else {
		QPointF ru = mPoly.polygon()[1];
		QPointF rd = mPoly.polygon()[2];
		QPointF ld = mPoly.polygon()[3];
		//check which coordinate is the leftmost (upperleft or downleft)
		return QPointF(ru.x() > rd.x() ? ru.x() : rd.x(), 
			rd.y() > ld.y() ? rd.y() : ld.y());
	}
}

void TableRegion::setRows(int r) {
	mRows = r;
}

int TableRegion::rows() const {
	return mRows;
}

void TableRegion::setCols(int c) {
	mCols = c;
}

int TableRegion::cols() const {
	return mCols;
}

TableCell::TableCell(const Type & type) : Region(type) {
	// default to text line
	if (mType == type_unknown)
		mType = Region::type_table_cell;
}

rdf::Line TableCell::topBorder() const
{
	//check if cornerpoints are defined
	if (mPoly.size() >= 4 && mCornerPts.size() == 4) {
		
		QPointF p1 = mPoly.polygon()[mCornerPts[0]];
		QPointF p2 = mPoly.polygon()[mCornerPts[3]];
		QPolygonF tmp;
		tmp << p1 << p2;
		return rdf::Line(rdf::Polygon(tmp));
		
	//no cornerpoints but polygon is present
	} else if (mPoly.size() == 4 && mCornerPts.isEmpty()) {

		QPointF p1 = mPoly.polygon()[0];
		QPointF p2 = mPoly.polygon()[3];
		QPolygonF tmp;
		tmp << p1 << p2;
		return rdf::Line(rdf::Polygon(tmp));
	}

	return rdf::Line();
}

rdf::Line TableCell::bottomBorder() const
{
	if (mPoly.size() >= 4 && mCornerPts.size() == 4) {

		QPointF p1 = mPoly.polygon()[mCornerPts[1]];
		QPointF p2 = mPoly.polygon()[mCornerPts[2]];
		QPolygonF tmp;
		tmp << p1 << p2;
		return rdf::Line(rdf::Polygon(tmp));

	}
	else if (mPoly.size() == 4 && mCornerPts.isEmpty()) {
		QPointF p1 = mPoly.polygon()[1];
		QPointF p2 = mPoly.polygon()[2];
		QPolygonF tmp;
		tmp << p1 << p2;
		return rdf::Line(rdf::Polygon(tmp));
	}

	return rdf::Line();
}

rdf::Line TableCell::leftBorder() const
{
	if (mPoly.size() >= 4 && mCornerPts.size() == 4) {

		QPointF p1 = mPoly.polygon()[mCornerPts[0]];
		QPointF p2 = mPoly.polygon()[mCornerPts[1]];
		QPolygonF tmp;
		tmp << p1 << p2;
		return rdf::Line(rdf::Polygon(tmp));

	}
	else if (mPoly.size() == 4 && mCornerPts.isEmpty()) {
		QPointF p1 = mPoly.polygon()[0];
		QPointF p2 = mPoly.polygon()[1];
		QPolygonF tmp;
		tmp << p1 << p2;
		return rdf::Line(rdf::Polygon(tmp));
	}

	return rdf::Line();
}

rdf::Line TableCell::rightBorder() const
{
	if (mPoly.size() >= 4 && mCornerPts.size() == 4) {

		QPointF p1 = mPoly.polygon()[mCornerPts[3]];
		QPointF p2 = mPoly.polygon()[mCornerPts[2]];
		QPolygonF tmp;
		tmp << p1 << p2;
		return rdf::Line(rdf::Polygon(tmp));

	}
	else if (mPoly.size() == 4 && mCornerPts.isEmpty()) {
		QPointF p1 = mPoly.polygon()[3];
		QPointF p2 = mPoly.polygon()[2];
		QPolygonF tmp;
		tmp << p1 << p2;
		return rdf::Line(rdf::Polygon(tmp));
	}

	return rdf::Line();
}

void TableCell::readAttributes(QXmlStreamReader & reader) {

	Region::readAttributes(reader);

	mRow = reader.attributes().value(RegionXmlHelper::instance().tag(RegionXmlHelper::attr_row)).toInt();
	mCol = reader.attributes().value(RegionXmlHelper::instance().tag(RegionXmlHelper::attr_col)).toInt();

	mRowSpan = reader.attributes().value(RegionXmlHelper::instance().tag(RegionXmlHelper::attr_rowspan)).toInt();
	mColSpan = reader.attributes().value(RegionXmlHelper::instance().tag(RegionXmlHelper::attr_colspan)).toInt();

	mLeftBorderVisible = reader.attributes().value(RegionXmlHelper::instance().tag(RegionXmlHelper::attr_leftVisible)).toString().compare("true") == 0 ? true : false;
	mRightBorderVisible = reader.attributes().value(RegionXmlHelper::instance().tag(RegionXmlHelper::attr_rightVisible)).toString().compare("true") == 0 ? true : false;;
	mTopBorderVisible = reader.attributes().value(RegionXmlHelper::instance().tag(RegionXmlHelper::attr_topVisible)).toString().compare("true") == 0 ? true : false;
	mBottomBorderVisible = reader.attributes().value(RegionXmlHelper::instance().tag(RegionXmlHelper::attr_bottomVisible)).toString().compare("true") == 0 ? true : false;

	//if (rType == Region::type_table_cell) {
	//	QSharedPointer<TableCell> pT = region.dynamicCast<TableCell>();
	//	pT->setRow(reader.attributes().value(RegionXmlHelper::instance().tag(RegionXmlHelper::attr_row)).toInt());
	//	pT->setCol(reader.attributes().value(RegionXmlHelper::instance().tag(RegionXmlHelper::attr_col)).toInt());
	//	
	//	pT->setRowSpan(reader.attributes().value(RegionXmlHelper::instance().tag(RegionXmlHelper::attr_rowspan)).toInt());
	//	pT->setColSpan(reader.attributes().value(RegionXmlHelper::instance().tag(RegionXmlHelper::attr_colspan)).toInt());

	//	bool tb;
	//	tb = reader.attributes().value(RegionXmlHelper::instance().tag(RegionXmlHelper::attr_leftVisible)).toString().compare("true") == 0 ? true : false;
	//	pT->setLeftBorderVisible(tb);
	//	tb = reader.attributes().value(RegionXmlHelper::instance().tag(RegionXmlHelper::attr_rightVisible)).toString().compare("true") == 0 ? true : false;
	//	pT->setRightBorderVisible(tb);
	//	tb = reader.attributes().value(RegionXmlHelper::instance().tag(RegionXmlHelper::attr_topVisible)).toString().compare("true") == 0 ? true : false;
	//	pT->setTopBorderVisible(tb);
	//	tb = reader.attributes().value(RegionXmlHelper::instance().tag(RegionXmlHelper::attr_bottomVisible)).toString().compare("true") == 0 ? true : false;
	//	pT->setBottomBorderVisible(tb);
	//}


}

bool TableCell::read(QXmlStreamReader & reader) {

	RegionXmlHelper& rm = RegionXmlHelper::instance();

	if (reader.tokenType() == QXmlStreamReader::StartElement && reader.qualifiedName().toString() == rm.tag(RegionXmlHelper::tag_cornerpts)) {

		reader.readNext();
		QString pts = reader.text().toUtf8().trimmed();	// add text

		if (!pts.isEmpty()) {

			QStringList points = pts.split(" ");

			if (points.size() != 4) {
				qWarning() << "illegal point string: " << points;
			}
			else {

				bool xok = false;
				for (auto i : points) {
					int x = i.toInt(&xok);
					
					if (xok)
						mCornerPts.append(x);
					else
						qWarning() << "illegal point string: " << x;
				}
			}
		}
		// fallback to old point coordinates
		//else {
		//	readPoints(reader);
		//}
	}
	else
		return Region::read(reader);

	return true;
	
}

void TableCell::setRow(int r) {
	mRow = r;
}

int TableCell::row() const {
	return mRow;
}

void TableCell::setCol(int c) {
	mCol = c;
}

int TableCell::col() const {
	return mCol;
}

void TableCell::setRowSpan(int r) {
	mRowSpan = r;
}

int TableCell::rowSpan() const {
	return mRowSpan;
}

void TableCell::setColSpan(int c) {
	mColSpan = c;
}

int TableCell::colSpan() const {
	return mColSpan;
}

void TableCell::setLeftBorderVisible(bool b) {
	mLeftBorderVisible = b;
}

bool TableCell::leftBorderVisible() const {
	return mLeftBorderVisible;
}

void TableCell::setRightBorderVisible(bool b) {
	mRightBorderVisible = b;
}

bool TableCell::rightBorderVisible() const {
	return mRightBorderVisible;
}

void TableCell::setTopBorderVisible(bool b) {
	mTopBorderVisible = b;
}

bool TableCell::topBorderVisible() const {
	return mTopBorderVisible;
}

void TableCell::setBottomBorderVisible(bool b) {
	mBottomBorderVisible = b;
}

bool TableCell::bottomBorderVisible() const {
	return mBottomBorderVisible;
}

void TableCell::setHeader(bool b) {
	mHeader = b;
}

bool TableCell::header() const {
	return mHeader;
}

bool TableCell::operator<(const TableCell & cell) const {

	if (row() == cell.row()) {
		return col() < cell.col();
	} else {
		return row() < cell.row();
	}
}

}