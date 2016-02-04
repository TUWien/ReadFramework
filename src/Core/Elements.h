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

#include "Shapes.h"

#pragma warning(push, 0)	// no warnings from includes
#include <QVector>
#include <QSharedPointer>
#include <QPolygon>
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
class QXmlStreamReader;

namespace rdf {

// read defines

class DllCoreExport Region {

public:
	Region();

	enum Type {
		type_unknown = 0,
		type_text_region,
		type_text_line,
		type_word,
		type_separator,
		type_image,
		type_graphic,
		type_noise,

		type_end
	};

	friend DllCoreExport QDataStream& operator<<(QDataStream& s, const Region& r);
	friend DllCoreExport QDebug operator<< (QDebug d, const Region &r);

	void setType(const Region::Type& type);
	Region::Type type() const;

	void setId(const QString& id);
	QString id() const;

	void setPoly(const Polygon& polygon);
	Polygon polygon() const;

	void addChild(QSharedPointer<Region> child);
	void removeChild(QSharedPointer<Region> child);
	void setChildren(const QVector<QSharedPointer<Region> >& children);
	QVector<QSharedPointer<Region> > children() const;

	virtual QString toString(bool withChildren = false) const;
	virtual QString childrenToString() const;

	virtual bool read(QXmlStreamReader& reader);

protected:
	Type mType;
	QString mId;
	Polygon mPoly;
	QVector<QSharedPointer<Region> > mChildren;
};

class DllCoreExport TextLine : public Region {

public:
	TextLine();

	void setBaseLine(const BaseLine& baseLine);
	BaseLine baseLine() const;

	void setText(const QString& text);
	QString text() const;

	virtual bool read(QXmlStreamReader& reader) override;

	virtual QString toString(bool withChildren = false) const override;

protected:
	BaseLine mBaseLine;
	QString mText;

};


class DllCoreExport RegionManager {

public:
	static RegionManager& instance();

	Region::Type type(const QString& typeName) const;
	QString typeName(const Region::Type& type) const;
	QStringList typeNames() const;
	bool isValidTypeName(const QString& typeName) const;

	QSharedPointer<Region> createRegion(const Region::Type& type) const;

private:
	RegionManager();
	RegionManager(const RegionManager&);
	
	QStringList createTypeNames() const;

	QStringList mTypeNames;

};


class PageElement {

public:
	PageElement(const QString& xmlPath = QString());

	void setXmlPath(const QString& xmlPath);
	QString xmlPath() const;

	void setImageFileName(const QString& name);
	QString imageFileName() const;

	void setImageSize(const QSize& size);
	QSize imageSize() const;

	void setRootRegion(QSharedPointer<Region> region);
	QSharedPointer<Region> rootRegion() const;

protected:
	QString mXmlPath;
	QString mImageFileName;
	QSize mImageSize;

	QSharedPointer<Region> mRoot;
};

};