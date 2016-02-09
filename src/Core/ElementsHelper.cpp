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

#include "ElementsHelper.h"
#include "Settings.h"

#pragma warning(push, 0)	// no warnings from includes
#include <QDebug>
#include <QSettings>
#pragma warning(pop)

namespace rdf {

// RegionTypeConfig --------------------------------------------------------------------
RegionTypeConfig::RegionTypeConfig(const Region::Type& type) {
	mType = type;
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
	
	mDrawPoly = settings.value("drawPoly", mDrawPoly).toBool();
	mDrawBaseline = settings.value("drawBaseline", mDrawBaseline).toBool();
	mDrawBaseline = settings.value("drawText", mDrawText).toBool();
	settings.endGroup();
}

void RegionTypeConfig::save(QSettings & settings) const {

	settings.beginGroup(RegionManager::instance().typeName(mType));
	settings.setValue("pen", mPen);
	settings.setValue("brush", mBrush);
	//settings.setValue("penColor", mPen.color());
	//settings.setValue("penBrush", mPen.brush().color());
	//settings.setValue("penWidth", mPen.width());
	
	settings.setValue("drawPoly", mDrawPoly);
	settings.setValue("drawBaseline", mDrawBaseline);
	settings.setValue("drawText", mDrawText);
	settings.endGroup();
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
	case tag_text_equiv:	return "TextEquiv";
	case tag_unicode:		return "Unicode";
	case tag_plain_text:	return "PlainText";
	case tag_baseline:		return "Baseline";

	case attr_points:		return "points";
	case attr_id:			return "id";
	}

	qWarning() << "unknown tag: " << tagId;
	return "";
}


QVector<RegionTypeConfig> RegionManager::regionTypeConfig() const {
	return mTypeConfig;
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

QVector<RegionTypeConfig> RegionManager::createConfig() const {

	QVector<RegionTypeConfig> typeConfig;

	for (int type = 0; type < Region::type_end; type++) {

		RegionTypeConfig rc((Region::Type)type);
		//rc.load(Config::instance().settings());

		typeConfig.append(rc);
	}

	return typeConfig;
}

void RegionManager::save() const {
	
	for (const RegionTypeConfig& c : mTypeConfig)
		c.save(Config::instance().settings());
}

QString RegionManager::typeName(const Region::Type& type) const {

	switch (type) {
	case Region::type_unknown:		return "Unknown";
	case Region::type_root:			return "Root";
	case Region::type_text_region:	return "TextRegion";
	case Region::type_text_line:	return "TextLine";
	case Region::type_word:			return "Word";
	case Region::type_separator:	return "Separator";
	case Region::type_image:		return "ImageRegion";
	case Region::type_graphic:		return "GraphicRegion";
	case Region::type_noise:		return "NoiseRegion";
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
	case Region::type_text_region:
	case Region::type_text_line:
	case Region::type_word:
		return QSharedPointer<TextLine>(new TextLine());
		break;
		// Add new types here...
	}

	return QSharedPointer<Region>(new Region());
}

void RegionManager::drawRegion(QPainter & p, QSharedPointer<rdf::Region> region, const QVector<RegionTypeConfig>& config, bool recursive) const {

	if (!region) {
		qWarning() << "I cannot draw a NULL region";
		return;
	}

	QVector<RegionTypeConfig> c = config.empty() ? mTypeConfig : config;

	// do not draw this kind of region
	if (!c[region->type()].draw())
		return;

	region->draw(p, c[region->type()]);

	if (recursive) {

		for (auto r : region->children())
			drawRegion(p, r, c, recursive);
	}

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