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

#include "PixelLabel.h"

#pragma warning(push, 0)	// no warnings from includes
#include <QJsonObject>		// needed for LabelLookup
#include <QJsonDocument>	// needed for LabelLookup
#include <QJsonArray>		// needed for LabelLookup

#include <QDebug>
#pragma warning(pop)

namespace rdf {

// LabelLookup --------------------------------------------------------------------
LabelLookup::LabelLookup(int id, const QString& name) {

	mId = id;

	if (!name.isEmpty())
		mName = name;
}

bool LabelLookup::operator==(const LabelLookup & l1) const {
	return id() == l1.id() && name() == l1.name();
}

bool LabelLookup::isNull() const {
	return id() == -1;
}

bool LabelLookup::contains(const QString& key) const {

	if (mName == key)
		return true;

	return mAlias.contains(key);
}

QColor LabelLookup::color() const {
	QColor c(id() << 8);	// << 8 away from alpha (RGBA)
	return c;
}

QColor LabelLookup::visColor() const {
	return mVisColor;
}

int LabelLookup::color2Id(const QColor & col) {
	int ci = col.rgba();
	return ci >> 8 & 0xFFFF;
}

LabelLookup LabelLookup::ignoreLabel() {
	LabelLookup ll(label_ignore, QObject::tr("Ignore"));
	ll.mVisColor = ColorManager::darkGray(0.4);
	return ll;
}

LabelLookup LabelLookup::unknownLabel() {
	LabelLookup ll(label_unknown, QObject::tr("Unknown"));
	ll.mVisColor = ColorManager::red();

	return ll;
}

LabelLookup LabelLookup::backgroundLabel() {
	LabelLookup ll(label_background, QObject::tr("Background"));
	ll.mVisColor = QColor(0, 0, 0);

	return ll;
}

int LabelLookup::id() const {
	return mId;
}

QString LabelLookup::name() const {
	return mName;
}

QString LabelLookup::toString() const {

	QString str;
	str += QString::number(id()) + ", " + name() + ", ";

	for (const QString& a : mAlias)
		str += a + ", ";

	return str;
}

QDataStream& operator<<(QDataStream& s, const LabelLookup& ll) {

	s << ll.toString();
	return s;
}

QDebug operator<<(QDebug d, const LabelLookup& ll) {

	d << qPrintable(ll.toString());
	return d;
}

LabelLookup LabelLookup::fromString(const QString & str) {

	// expecting a string like:
	// #ID, #Name, #Alias1, #Alias2, ..., #AliasN
	QStringList list = str.split(",");

	if (list.size() < 2) {
		qWarning() << "illegal label string: " << str;
		qInfo() << "I expected: ID, Name, Alias1, ..., AliasN";
		return LabelLookup();
	}

	LabelLookup ll;

	// parse ID
	bool ok = false;
	ll.mId = list[0].toInt(&ok);

	if (!ok) {
		qWarning() << "first entry must be an int, but it is: " << list[0];
		return LabelLookup();
	}

	// parse name
	ll.mName = list[1];

	for (int idx = 2; idx < list.size(); idx++) {
		ll.mAlias << list[idx];
	}

	return ll;
}

LabelLookup LabelLookup::fromJson(const QJsonObject & jo) {

	//"Label": {
	//	"id": 5,
	//	"name": "image",
	//	"alias": ["ImageRegion", "ChartRegion", "GraphicRegion"],
	//	"color": "#990066", 
	//},
	LabelLookup ll;
	ll.mId = jo.value("id").toInt(label_unknown);
	ll.mName = jo.value("name").toString();
	ll.mVisColor.setNamedColor(jo.value("color").toString());

	for (const QJsonValue& jv : jo.value("alias").toArray()) {
		const QString alias = jv.toString();
		if (!alias.isEmpty())
			ll.mAlias << alias;
	}

	// print warning
	if (ll.id() == label_unknown) {
		QJsonDocument jd(jo);
		qCritical().noquote() << "could not parse" << jd.toJson();
		return LabelLookup::unknownLabel();
	}

	return ll;
}

QString LabelLookup::jsonKey() {
	return QString("TUWienLabelLookup");
}

// PixelLabel --------------------------------------------------------------------
PixelLabel::PixelLabel(const QString & id) : BaseElement(id) {
}

void PixelLabel::setLabel(const LabelLookup & label) {
	mLabel = label;
}

LabelLookup PixelLabel::label() const {
	return mLabel;
}

void PixelLabel::setTrueLabel(const LabelLookup & label) {
	mTrueLabel = label;
}

LabelLookup PixelLabel::trueLabel() const {
	return mTrueLabel;
}

}