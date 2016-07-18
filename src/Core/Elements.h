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
#include <QDateTime>
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
class QXmlStreamWriter;
class QPainter;

namespace rdf {

// read defines
class RegionTypeConfig;

class DllCoreExport Region {

public:
	Region();

	enum Type {
		type_unknown = 0,
		type_root,
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

	void setSelected(bool select);
	bool selected() const;

	void setType(const Region::Type& type);
	Region::Type type() const;

	void setId(const QString& id);
	QString id() const;

	void setCustom(const QString& c);
	QString custom() const;

	void setPolygon(const Polygon& polygon);
	Polygon polygon() const;

	void addChild(QSharedPointer<Region> child);
	void addUniqueChild(QSharedPointer<Region> child);
	void removeChild(QSharedPointer<Region> child);
	void setChildren(const QVector<QSharedPointer<Region> >& children);
	QVector<QSharedPointer<Region> > children() const;
	static QVector<QSharedPointer<Region> > allRegions(QSharedPointer<Region> root);

	virtual void draw(QPainter& p, const RegionTypeConfig& config) const;

	virtual QString toString(bool withChildren = false) const;
	virtual QString childrenToString() const;

	virtual bool read(QXmlStreamReader& reader);
	virtual void write(QXmlStreamWriter& writer, bool withChildren = true, bool close = true) const;
	virtual void writeChildren(QXmlStreamWriter& writer) const;

	virtual bool operator==(const Region& r1);

protected:
	Type mType = Type::type_unknown;
	bool mSelected = false;
	QString mId;
	QString mCustom;
	Polygon mPoly;
	QVector<QSharedPointer<Region> > mChildren;

	void collectRegions(QVector<QSharedPointer<Region> >& allRegions) const;
};

class DllCoreExport TextLine : public Region {

public:
	TextLine();

	void setBaseLine(const BaseLine& baseLine);
	BaseLine baseLine() const;

	void setText(const QString& text);
	QString text() const;

	virtual bool read(QXmlStreamReader& reader) override;
	virtual void write(QXmlStreamWriter& writer, bool withChildren = true, bool close = true) const override;

	virtual QString toString(bool withChildren = false) const override;

	virtual void draw(QPainter& p, const RegionTypeConfig& config) const override;

protected:
	BaseLine mBaseLine;
	QString mText;
	bool mTextPresent = false;
};


class DllCoreExport SeparatorRegion : public Region {

public:
	SeparatorRegion();

	void setLine(const Line& line);
	Line line() const;

	//virtual bool read(QXmlStreamReader& reader) override;
	//virtual void write(QXmlStreamWriter& writer, bool withChildren = true, bool close = true) const override;

	//virtual QString toString(bool withChildren = false) const override;

	//virtual void draw(QPainter& p, const RegionTypeConfig& config) const override;
	//virtual bool operator==(const SeparatorRegion& sr1);
	virtual bool operator==(const Region& sr1);

protected:
	Line mLine;
};

class DllCoreExport PageElement {

public:
	PageElement(const QString& xmlPath = QString());

	bool isEmpty();

	void setXmlPath(const QString& xmlPath);
	QString xmlPath() const;

	void setImageFileName(const QString& name);
	QString imageFileName() const;

	void setImageSize(const QSize& size);
	QSize imageSize() const;

	void setRootRegion(QSharedPointer<Region> region);
	QSharedPointer<Region> rootRegion() const;

	void setCreator(const QString& creator);
	QString creator() const;

	void setDateCreated(const QDateTime& date);
	QDateTime dateCreated() const;

	void setDateModified(const QDateTime& date);
	QDateTime dateModified() const;

	friend DllCoreExport QDebug operator<< (QDebug d, const PageElement &p);
	friend DllCoreExport QDataStream& operator<<(QDataStream& s, const PageElement& p);
	virtual QString toString() const;

protected:
	QString mXmlPath;
	QString mImageFileName;
	QSize mImageSize;

	// meta attributes
	QString mCreator;
	QDateTime mDateCreated;
	QDateTime mDateModified;

	QSharedPointer<Region> mRoot;
};

};