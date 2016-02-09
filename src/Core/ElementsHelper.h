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

#pragma once

#include "Elements.h"

#pragma warning(push, 0)	// no warnings from includes
#include <QColor>
#include <QPen>
#pragma warning(pop)

#pragma warning(disable: 4251)	// dll interface warning

#ifndef DllCoreExport
#ifdef DLL_CORE_EXPORT
#define DllCoreExport Q_DECL_EXPORT
#else
#define DllCoreExport Q_DECL_IMPORT
#endif
#endif

// Qt defines
class QSettings;

namespace rdf {

// read defines

class DllCoreExport RegionTypeConfig {

public:
	RegionTypeConfig(const Region::Type& type = Region::type_unknown);

	Region::Type type() const;

	void setDraw(bool draw);
	bool draw() const;

	void setDrawPoly(bool draw);
	bool drawPoly() const;

	void setDrawBaseline(bool draw);
	bool drawBaseline() const;
	
	void setDrawText(bool draw);
	bool drawText() const;

	void setPen(const QPen& pen);
	QPen pen() const;

	void setBrush(const QColor& col);
	QColor brush() const;
	
	void load(QSettings& settings);
	void save(QSettings& settings) const;

protected:

	Region::Type mType = Region::type_unknown;
	QPen mPen;
	QColor mBrush;

	bool mDraw = true;
	bool mDrawPoly = true;
	bool mDrawBaseline = false;
	bool mDrawText = true;

};

class RegionXmlHelper {

public:
	static RegionXmlHelper& instance();

	enum XmlTags {
		tag_coords,
		tag_text_equiv,
		tag_unicode,
		tag_plain_text,
		tag_baseline,

		attr_points,
		attr_id,

		tag_end
	};

	QString tag(const XmlTags& tagId) const;

private:
	RegionXmlHelper();
	RegionXmlHelper(const RegionXmlHelper&);

	QStringList createTags() const;
	QStringList mTags;
};

class DllCoreExport RegionManager {

public:
	static RegionManager& instance();

	Region::Type type(const QString& typeName) const;
	QString typeName(const Region::Type& type) const;
	QStringList typeNames() const;
	bool isValidTypeName(const QString& typeName) const;

	QSharedPointer<Region> createRegion(const Region::Type& type) const;

	void drawRegion(QPainter& p, QSharedPointer<rdf::Region> region, const QVector<RegionTypeConfig>& config = QVector<RegionTypeConfig>(), bool recursive = true) const;

	QVector<RegionTypeConfig> regionTypeConfig() const;
	void save() const;

private:
	RegionManager();
	RegionManager(const RegionManager&);

	QStringList createTypeNames() const;
	QVector<RegionTypeConfig> createConfig() const;

	QStringList mTypeNames;
	QVector<RegionTypeConfig> mTypeConfig;
};


};