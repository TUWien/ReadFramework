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
 [1] https://cvl.tuwien.ac.at/
 [2] https://transkribus.eu/Transkribus/
 [3] https://github.com/TUWien/
 [4] https://nomacs.org
 *******************************************************************************************************/

#include "ElementsHelper.h"
#include "Settings.h"

#pragma warning(push, 0)	// no warnings from includes
#include <QDebug>
#include <QSettings>
#include <QPainter>
#pragma warning(pop)

namespace rdf {

// RegionTypeConfig --------------------------------------------------------------------
RegionTypeConfig::RegionTypeConfig(const Region::Type& type) {
	
	mType = type;
	assignDefaultColor(type);
}

Region::Type RegionTypeConfig::type() const {
	return mType;
}

void RegionTypeConfig::setDraw(bool draw) {
	mDraw = draw;
}

bool RegionTypeConfig::draw() const {
	return mDraw;
}

void RegionTypeConfig::setDrawPoly(bool draw) {
	mDrawPoly = draw;
}

bool RegionTypeConfig::drawPoly() const {
	return mDrawPoly;
}

void RegionTypeConfig::setDrawBaseline(bool draw) {
	mDrawBaseline = draw;
}

bool RegionTypeConfig::drawBaseline() const {
	return mDrawBaseline;
}

void RegionTypeConfig::setDrawBaselineLimits(bool draw) {
	mDrawBaselineLimits = draw;
}

bool RegionTypeConfig::drawBaselineLimits() const {
	return mDrawBaselineLimits;
}

void RegionTypeConfig::setDrawText(bool draw) {
	mDrawText = draw;
}

bool RegionTypeConfig::drawText() const {
	return mDrawText;
}

void RegionTypeConfig::setPen(const QPen & pen) {
	mPen = pen;
}

QPen RegionTypeConfig::pen() const {
	return mPen;
}

void RegionTypeConfig::setBrush(const QColor & col) {
	mBrush = col;
}

QColor RegionTypeConfig::brush() const {
	return mBrush;
}

void RegionTypeConfig::load(QSettings & settings) {

	settings.beginGroup(RegionManager::instance().typeName(mType));
	mPen = settings.value("pen", mPen).value<QPen>();
	mBrush = settings.value("brush", mBrush).value<QColor>();
	
	mDraw = settings.value("draw", mDraw).toBool();
	mDrawPoly = settings.value("drawPoly", mDrawPoly).toBool();
	mDrawBaseline = settings.value("drawBaseline", mDrawBaseline).toBool();
	mDrawText = settings.value("drawText", mDrawText).toBool();
	settings.endGroup();
}

void RegionTypeConfig::save(QSettings & settings) const {

	settings.beginGroup(RegionManager::instance().typeName(mType));
	settings.setValue("pen", mPen);
	settings.setValue("brush", mBrush);
	
	settings.setValue("draw", mDraw);
	settings.setValue("drawPoly", mDrawPoly);
	settings.setValue("drawBaseline", mDrawBaseline);
	settings.setValue("drawText", mDrawText);
	settings.endGroup();
}

void RegionTypeConfig::assignDefaultColor(const Region::Type & type) {

	QColor col(100,100,100);

	switch (type) {
	case Region::type_text_region:	col = QColor(0, 102, 153);	break;
	case Region::type_text_line:	col = QColor(0, 153, 4);	break;
	case Region::type_word:			col = QColor(96, 202, 255); break;
	case Region::type_separator:	col = QColor(190, 22, 22);	break;
	case Region::type_image:		col = QColor(255, 174, 0);	break;
	case Region::type_graphic:		col = QColor(100, 74, 0);	break;
	case Region::type_chart:		col = QColor(0, 108, 255);	break;
	case Region::type_noise:		col = QColor(204, 204, 204);break;
	case Region::type_border:		col = QColor(204, 185, 146);break;
	default:;	// default is gray
	}

	// assign default color
	mPen.setColor(col);
	col.setAlpha(25);
	mBrush = col;
}

// RegionXmlHelper --------------------------------------------------------------------
RegionXmlHelper::RegionXmlHelper() {

	mTags = createTags();
}

RegionXmlHelper& RegionXmlHelper::instance() {

	static QSharedPointer<RegionXmlHelper> inst;
	if (!inst)
		inst = QSharedPointer<RegionXmlHelper>(new RegionXmlHelper());
	return *inst;
}

QStringList RegionXmlHelper::createTags() const {

	QStringList tn;
	for (int idx = 0; idx < XmlTags::tag_end; idx++)
		tn.append(tag((XmlTags) idx));

	return tn;
}

QString RegionXmlHelper::tag(const XmlTags& tagId) const {

	switch (tagId) {
	case tag_coords:		return "Coords";
	case tag_point:			return "Point";
	case tag_text_equiv:	return "TextEquiv";
	case tag_unicode:		return "Unicode";
	case tag_plain_text:	return "PlainText";
	case tag_baseline:		return "Baseline";
	case tag_cornerpts:		return "CornerPts";

	case attr_points:		return "points";
	case attr_id:			return "id";
	case attr_custom:		return "custom";

	case attr_rows:			return "rows";
	case attr_cols:			return "cols";
	case attr_row:			return "row";
	case attr_col:			return "col";
	case attr_rowspan:		return "rowSpan";
	case attr_colspan:		return "colSpan";
	case attr_leftVisible:	return "leftBorderVisible";
	case attr_rightVisible:	return "rightBorderVisible";
	case attr_topVisible:	return "topBorderVisible";
	case attr_bottomVisible:return "bottomBorderVisible";
	default:;
	}

	qWarning() << "unknown tag: " << tagId;
	return "";
}

// RegionManager --------------------------------------------------------------------
RegionManager::RegionManager() {

	mTypeNames = createTypeNames();
	mTypeConfig = createConfig();
}

RegionManager& RegionManager::instance() {

	static QSharedPointer<RegionManager> inst;
	if (!inst)
		inst = QSharedPointer<RegionManager>(new RegionManager());
	return *inst;
}

QVector<QSharedPointer<RegionTypeConfig> > RegionManager::regionTypeConfig() const {
	return mTypeConfig;
}

/// <summary>
/// Selects the regions.
/// </summary>
/// <param name="selRegions">a vector with all regions to select.</param>
/// <param name="rootRegion">The root region. If it is not empty, all regions are unselected first.</param>
void RegionManager::selectRegions(const QVector<QSharedPointer<Region>>& selRegions, QSharedPointer<Region> rootRegion) const {

	if (rootRegion) {
		QVector<QSharedPointer<Region> > regions = Region::allRegions(rootRegion.data());

		// deselect all
		for (auto r : regions)
			r->setSelected(false);
	}

	for (auto r : selRegions)
		r->setSelected(true);

}

QVector<QSharedPointer<RegionTypeConfig> > RegionManager::createConfig() const {

	QVector<QSharedPointer<RegionTypeConfig> > typeConfig;

	for (int type = 0; type < Region::type_end; type++) {

		QSharedPointer<RegionTypeConfig> rc(new RegionTypeConfig((Region::Type)type));
		typeConfig.append(rc);
	}

	return typeConfig;
}

QString RegionManager::typeName(const Region::Type& type) const {

	switch (type) {
	case Region::type_unknown:		return "Unknown";
	case Region::type_root:			return "Root";
	case Region::type_text_region:	return "TextRegion";
	case Region::type_table_region:	return "TableRegion";
	case Region::type_table_cell:	return "TableCell";
	case Region::type_text_line:	return "TextLine";
	case Region::type_word:			return "Word";
	case Region::type_separator:	return "SeparatorRegion";
	case Region::type_image:		return "ImageRegion";
	case Region::type_graphic:		return "GraphicRegion";
	case Region::type_chart:		return "ChartRegion";
	case Region::type_noise:		return "NoiseRegion";
	case Region::type_border:		return "Border";
	default:;
	}

	return "Unknown";
}

QStringList RegionManager::createTypeNames() const {

	QStringList tn;
	for (int idx = 0; idx < Region::type_end; idx++)
		tn.append(typeName((Region::Type) idx));

	return tn;
}

QStringList RegionManager::typeNames() const {
	return mTypeNames;
}

bool RegionManager::isValidTypeName(const QString & typeName) const {

	return mTypeNames.contains(typeName);
}

QSharedPointer<Region> RegionManager::createRegion(const Region::Type & type) const {

	switch (type) {
	case Region::type_separator:
		return QSharedPointer<SeparatorRegion>::create(type);
	case Region::type_text_region:
		return QSharedPointer<TextRegion>::create(type);
	case Region::type_text_line:
	case Region::type_word:
		return QSharedPointer<TextLine>::create(type);
	case Region::type_image:
	case Region::type_graphic:
	case Region::type_chart:
	case Region::type_noise:
	case Region::type_border:
		return QSharedPointer<Region>::create(type);
	case Region::type_table_region:
		return QSharedPointer<TableRegion>::create(type);
	case Region::type_table_cell:
		return QSharedPointer<TableCell>::create(type);
		// Add new types here...
	default:
		qWarning() << "unknown region type" << type;
	}

	return QSharedPointer<Region>::create();
}

QSharedPointer<RegionTypeConfig> RegionManager::getConfig(
	const QSharedPointer<Region>& r, 
	const QVector<QSharedPointer<RegionTypeConfig> >& config) const {


	const QVector<QSharedPointer<RegionTypeConfig> >& c = (config.isEmpty()) ? regionTypeConfig() : config;
	
	Region::Type t = r ? r->type() : Region::Type::type_unknown;

	if (t >= 0 && t < c.size())
		return c[t];

	return QSharedPointer<RegionTypeConfig>();
}

void RegionManager::drawRegion(
	QPainter & p, 
	QSharedPointer<rdf::Region> region, 
	const QVector<QSharedPointer<RegionTypeConfig> >& config, 
	bool recursive,
	bool activeSelection) const {

	if (!region) {
		qWarning() << "I cannot draw a NULL region";
		return;
	}

	QVector<QSharedPointer<RegionTypeConfig> > c = config.empty() ? mTypeConfig : config;

	// do not draw this kind of region
	if (region->type() < 0 || region->type() > c.size()) {
		qDebug() << "illegal region type" << region->type();
		return;
	}
		
	if (!c[region->type()]->draw())
		return;

	if (activeSelection && !region->selected())
		p.setOpacity(0.2);
	else
		p.setOpacity(1.0);

	region->draw(p, *c[region->type()]);

	if (recursive) {

		for (auto r : region->children())
			drawRegion(p, r, c, recursive, activeSelection);
	}

}

QVector<QSharedPointer<rdf::Region> > RegionManager::regionsAt(
	QSharedPointer<rdf::Region> root, 
	const QPoint & p,
	const QVector<QSharedPointer<RegionTypeConfig> >& config) const {
	
	QVector<QSharedPointer<rdf::Region> > sRegions;

	if (!root)
		return sRegions;

	QVector<QSharedPointer<Region> > regions = rdf::Region::allRegions(root.data());

	for (auto r : regions) {

		if (!r) {
			qWarning() << "NULL region detected";
			continue;
		}

		QSharedPointer<RegionTypeConfig> cr = getConfig(r, config);
			
		// is the region visible to the user?
		if (!cr->drawPoly() || !cr->draw())
			continue;

		// append region if it contains the point
		if (r->polygon().closedPolygon().containsPoint(p, Qt::WindingFill))
			sRegions << r;
	}
	
	return sRegions;
}

Region::Type RegionManager::type(const QString& typeName) const {

	QStringList tns = typeNames();
	int typeIdx = tns.indexOf(typeName);

	if (typeIdx != -1)
		return (Region::Type) typeIdx;
	else
		qWarning() << "Unknown type: " << typeName;

	return Region::type_unknown;
}

}
