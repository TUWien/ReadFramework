/*******************************************************************************************************
 ReadFramework is the basis for modules developed at CVL/TU Wien for the EU project READ. 
  
 Copyright (C) 2016 Markus Diem <diem@cvl.tuwien.ac.at>
 Copyright (C) 2016 Stefan Fiel <fiel@cvl.tuwien.ac.at>
 Copyright (C) 2016 Florian Kleber <kleber@cvl.tuwien.ac.at>

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
 [1] http://www.cvl.tuwien.ac.at/cvl/
 [2] https://transkribus.eu/Transkribus/
 [3] https://github.com/TUWien/
 [4] http://nomacs.org
 *******************************************************************************************************/

#include "BaseImageElement.h"

#pragma warning(push, 0)	// no warnings from includes
#include <QUuid>
#include <QDebug>
#pragma warning(pop)

namespace rdf {


// BaseElement --------------------------------------------------------------------
/// <summary>
/// This class keeps an ID to keep track of transformed
/// elements during a processing chain.
/// You can e.g. generate Pixel elements from MserBlobs.
/// After filtering, processing etc. you can map back to the
/// (pixel accurate) MserBlob using this ID.
/// </summary>
/// <param name="id">The identifier, if empty a new ID is generated.</param>
BaseElement::BaseElement(const QString& id) {
	
	mId = id.isEmpty() ? QUuid::createUuid().toString() : id;
}

bool operator==(const BaseElement & l, const QString & id) {
	return l.id() == id;
}

/// <summary>
/// Returns true if l and r have the same id.
/// </summary>
/// <param name="l">An element to compare.</param>
/// <param name="r">An element to compare.</param>
/// <returns></returns>
bool operator==(const BaseElement & l, const BaseElement & r) {
	return l.id() == r.id();
}

/// <summary>
/// Returns true if l and r do not have the same id.
/// </summary>
/// <param name="l">An element to compare.</param>
/// <param name="r">An element to compare.</param>
/// <returns></returns>
bool operator!=(const BaseElement & l, const BaseElement & r) {
	return !(l == r);
}

QDataStream& operator<<(QDataStream& s, const BaseElement& e) {

	// this makes the operator<< virtual (stroustrup)
	s << e.toString();
	return s;
}

QDebug operator<<(QDebug d, const BaseElement& e) {

	d << qPrintable(e.toString());
	return d;
}

/// <summary>
/// Sets the (preferably unique) ID.
/// If no ID is set, a unqiue ID is generated and assigned.
/// </summary>
/// <param name="id">The identifier.</param>
void BaseElement::setId(const QString & id) {
	mId = id;
}

/// <summary>
/// Returns the elment's id.
/// </summary>
/// <returns></returns>
QString BaseElement::id() const {
	return mId;
}

QString BaseElement::toString() const {
	return id() + " toString() not implemented for this object";
}

void BaseElement::scale(double) {

	qWarning() << "scale() is used but not implemented!";
}

}