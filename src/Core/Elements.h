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
class PixelSet;

class DllCoreExport Region {

public:
	enum Type {
		type_unknown = 0,
		type_root,
		type_table_region,
		type_table_cell,
		type_text_region,
		type_text_line,
		type_word,
		type_separator,
		type_image,
		type_graphic,
		type_chart,
		type_noise,

		type_end
	};

	Region(const Type& type = Type::type_unknown, const QString& id = "");

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
	bool reassignChild(QSharedPointer<Region> child);
	void addUniqueChild(QSharedPointer<Region> child, bool update = false);
	void removeChild(QSharedPointer<Region> child);
	void removeAllChildren();
	void setChildren(const QVector<QSharedPointer<Region> >& children);
	QVector<QSharedPointer<Region> > children() const;

	static QVector<QSharedPointer<Region> > selectedRegions(const Region* root);
	static QVector<QSharedPointer<Region> > allRegions(const Region* root);
	static QVector<QSharedPointer<Region> > filter(const Region* root, const Region::Type& type);

	virtual void draw(QPainter& p, const RegionTypeConfig& config) const;

	virtual QString toString(bool withChildren = false) const;
	virtual QString childrenToString() const;

	virtual bool read(QXmlStreamReader& reader);
	virtual void readAttributes(QXmlStreamReader& reader);
	virtual void write(QXmlStreamWriter& writer) const;
	virtual void writeChildren(QXmlStreamWriter& writer) const;
	void createElement(QXmlStreamWriter& writer) const;
	void writePolygon(QXmlStreamWriter& writer) const;

	virtual bool operator==(const Region& r1);

protected:
	Type mType = Type::type_unknown;
	bool mSelected = false;
	QString mId;
	QString mCustom;
	Polygon mPoly;
	QVector<QSharedPointer<Region> > mChildren;

	void collectRegions(QVector<QSharedPointer<Region> >& allRegions, const Region::Type& type = type_unknown) const;
	virtual bool readPoints(QXmlStreamReader& reader);
};

class DllCoreExport RootRegion : public Region {

public:
	RootRegion(const Type& type = Type::type_unknown);

	QVector<QSharedPointer<Region> > selectedRegions() const;
	QVector<QSharedPointer<Region> > allRegions() const;
	QVector<QSharedPointer<Region> > filter(const Region::Type& type) const;

};

class DllCoreExport TableRegion : public Region {

public:
	TableRegion(const Type& type = Type::type_unknown);

	//virtual bool read(QXmlStreamReader& reader);
	virtual void readAttributes(QXmlStreamReader& reader) override;
	virtual bool operator==(const Region& sr1);

	rdf::Line topBorder() const;
	rdf::Line bottomBorder() const;
	rdf::Line leftBorder() const;
	rdf::Line rightBorder() const;

	QPointF leftUpper() const;
	QPointF rightDown() const;

	QPointF leftUpperCorner() const;
	QPointF rightDownCorner() const;

	void setRows(int r);
	int rows() const;

	void setCols(int c);
	int cols() const;

protected:
	int mRows = -1;
	int mCols = -1;

	//QColor mLineColor;
	//QColor mBgColor;
};

class DllCoreExport TableCell : public Region {

public:
	TableCell(const Type& type = Type::type_unknown);

	rdf::Line topBorder() const;
	rdf::Line bottomBorder() const;
	rdf::Line leftBorder() const;
	rdf::Line rightBorder() const;
	
	rdf::Vector2D upperLeft() const;
	rdf::Vector2D upperRight() const;
	rdf::Vector2D downLeft() const;
	rdf::Vector2D downRight() const;

	virtual void readAttributes(QXmlStreamReader& reader) override;
	virtual bool read(QXmlStreamReader& reader) override;
	virtual void write(QXmlStreamWriter& writer) const override;

	void setRow(int r);
	int row() const;
	void setCol(int c);
	int col() const;

	void setRowSpan(int r);
	int rowSpan() const;
	void setColSpan(int c);
	int colSpan() const;

	void setLeftBorderVisible(bool b);
	bool leftBorderVisible() const;

	void setRightBorderVisible(bool b);
	bool rightBorderVisible() const;

	void setTopBorderVisible(bool b);
	bool topBorderVisible() const;

	void setBottomBorderVisible(bool b);
	bool bottomBorderVisible() const;

	void setHeader(bool b);
	bool header() const;

	//sorts Cells according row and cell
	bool operator< (const TableCell& cell) const;
	static bool compareCells(const QSharedPointer<rdf::TableCell> l1, const QSharedPointer<rdf::TableCell> l2);

protected:
	int mRow = -1;
	int mCol = -1;

	int mRowSpan = -1;
	int mColSpan = -1;

	bool mLeftBorderVisible = false;
	bool mRightBorderVisible = false;
	bool mTopBorderVisible = false;
	bool mBottomBorderVisible = false;

	bool mHeader = false;

	//QString mComments;
	//Polygon mPoly;
	//Polygon mCornerPts;
	QVector<int> mCornerPts;
};

class DllCoreExport TextEquiv {

public:
	TextEquiv();
	TextEquiv(const QString& text);

	static TextEquiv read(QXmlStreamReader& reader);

	void write(QXmlStreamWriter& writer) const;

	QString text() const;
	bool isNull() const;

protected:
	QString mText; // unicode
	bool mIsNull = false;
};

class DllCoreExport TextLine : public Region {

public:
	TextLine(const Type& type = Type::type_unknown);

	void setBaseLine(const BaseLine& baseLine);
	BaseLine baseLine() const;

	void setText(const QString& text);
	QString text() const;

	virtual bool read(QXmlStreamReader& reader) override;
	virtual void write(QXmlStreamWriter& writer) const override;

	virtual QString toString(bool withChildren = false) const override;

	virtual void draw(QPainter& p, const RegionTypeConfig& config) const override;

protected:
	BaseLine mBaseLine;
	TextEquiv mTextEquiv;
};

class DllCoreExport TextRegion : public Region {
	
public:
	TextRegion(const Type& type = Type::type_unknown);

	void setText(const QString& text);
	QString text() const;

	virtual bool read(QXmlStreamReader& reader) override;
	virtual void write(QXmlStreamWriter& writer) const override;

	virtual QString toString(bool withChildren = false) const override;
	
	virtual void draw(QPainter& p, const RegionTypeConfig& config) const override;

protected:
	TextEquiv mTextEquiv;
};

class DllCoreExport SeparatorRegion : public Region {

public:
	SeparatorRegion(const Type& type = Type::type_unknown);

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

class DllCoreExport LayerElement {

public:

	LayerElement();

	bool readAttributes(QXmlStreamReader& reader);
	void readChildren(QXmlStreamReader& reader);
	void write(QXmlStreamWriter& writer) const;

	void setId(const QString& id);
	QString id() const;

	void setZIndex(int zIndex);
	int zIndex() const;

	void setCaption(const QString& caption);
	QString caption() const;

	void setRegionRefIds(const QVector<QString>& regionRefIds);
	QVector<QString> regionRefIds() const;

	void setRegions(const QVector<QSharedPointer<Region>>& regions);
	QVector<QSharedPointer<Region>> regions() const;

protected:
	bool mChanged = false;
	QString mId = "";
	int mZIndex = 0;
	QString mCaption = "";
	QVector<QString> mRegionRefIds;
	QVector<QSharedPointer<Region>> mRegions;

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

	void setRootRegion(QSharedPointer<RootRegion> region);
	QSharedPointer<RootRegion> rootRegion() const;

	void setCreator(const QString& creator);
	QString creator() const;

	void setDateCreated(const QDateTime& date);
	QDateTime dateCreated() const;

	void setDateModified(const QDateTime& date);
	QDateTime dateModified() const;

	void setLayers(const QVector<QSharedPointer<LayerElement>>& layers);
	QVector<QSharedPointer<LayerElement>> layers() const;

	void setDefaultLayer(const QSharedPointer<LayerElement>& defaultLayer);
	QSharedPointer<LayerElement> defaultLayer() const;

	void redefineLayersByType(const QVector<Region::Type>& layerTypeAssignment);

	void sortLayers(bool checkIfSorted = false);

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

	QSharedPointer<RootRegion> mRoot;
	QVector<QSharedPointer<LayerElement>> mLayers;
	QSharedPointer<LayerElement> mDefaultLayer;

	static bool layerZIndexGt(const QSharedPointer<LayerElement>& l1, const QSharedPointer<LayerElement>& l2);

};

};
