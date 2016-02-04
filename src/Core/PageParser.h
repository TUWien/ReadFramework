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

namespace rdf {

class PageElement;
class Region;

// read defines
class DllCoreExport PageXmlParser {

public:
	PageXmlParser();

	enum RootTags {
		tag_page,
		tag_meta,

		attr_imageFilename,
		attr_imageWidth,
		attr_imageHeight,
		attr_id,
		attr_text_type,

		attr_meta_creator,
		attr_meta_created,
		attr_meta_changed,

		tag_end
	};

	void read(const QString& xmlPath);

	QString tagName(const RootTags& tag) const;

protected:

	QSharedPointer<PageElement> mPage;

	QSharedPointer<PageElement> parse(const QString& xmlPath) const;
	void parseRegion(QXmlStreamReader& reader, QSharedPointer<Region> parent) const;
	void parseMetadata(QXmlStreamReader& reader, QSharedPointer<PageElement> page) const;
};

};