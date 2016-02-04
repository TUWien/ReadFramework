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

#include "Shapes.h"
#include "Utils.h"

#pragma warning(push, 0)	// no warnings from includes
// Qt Includes
#pragma warning(pop)

namespace rdf {

Polygon::Polygon(const QPolygon& polygon) {
	mPoly = polygon;
}
void Polygon::read(const QString & pointList) {
	mPoly = Converter::instance().stringToPoly(pointList);
}

QString Polygon::write() const {
	return Converter::instance().polyToString(mPoly);
}

int Polygon::size() const {
	return mPoly.size();
}

void Polygon::setPolygon(const QPolygon & polygon) {
	mPoly = polygon;
}

QPolygon Polygon::polygon() const {
	return mPoly;
}


// BaseLine --------------------------------------------------------------------
BaseLine::BaseLine(const QPolygon & baseLine) {
	mBaseLine = baseLine;
}

void BaseLine::setPolygon(QPolygon & baseLine) {
	mBaseLine = baseLine;
}

QPolygon BaseLine::polygon() const {
	return mBaseLine;
}

void BaseLine::read(const QString & pointList) {
	mBaseLine = Converter::instance().stringToPoly(pointList);
}

QString BaseLine::write() const {
	return Converter::instance().polyToString(mBaseLine);
}


}