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

#pragma warning(push, 0)	// no warnings from includes
#include <QString>
#include <QSharedPointer>
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

namespace rdf {

class PageElement;
class Region;

// read defines
class DllCoreExport PageXmlParser {

public:
	PageXmlParser();

	enum RootTags {
		tag_root,
		tag_page,
		tag_meta,
		tag_layers,
		tag_meta_creator,
		tag_meta_created,
		tag_meta_changed,

		attr_imageFilename,
		attr_imageWidth,
		attr_imageHeight,
		attr_text_type,
		attr_xmlns,
		attr_xsi,
		attr_schemaLocation,

		tag_end
	};

	bool read(const QString& xmlPath, bool ignoreLayers = false);
	void write(const QString& xmlPath, const QSharedPointer<PageElement> pageElement);

	QString tagName(const RootTags& tag) const;

	void setPage(QSharedPointer<PageElement> page);
	QSharedPointer<PageElement> page() const;

	static QString imagePathToXmlPath(const QString& path);

protected:

	QSharedPointer<PageElement> mPage;

	virtual QSharedPointer<PageElement> parse(const QString& xmlPath, bool ignoreLayers = false) const;
	virtual void parseRegion(QXmlStreamReader& reader, QSharedPointer<Region> parent) const;
	virtual void parseMetadata(QXmlStreamReader& reader, QSharedPointer<PageElement> page) const;
	virtual void parseLayers(QXmlStreamReader& reader, QSharedPointer<PageElement> page, bool ignoreLayers = false) const;

	QByteArray writePageElement() const;
	void writeMetaData(QXmlStreamWriter& writer) const;
};

};
