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

#include "SuperPixelClassification.h"

#include "Utils.h"
#include "Image.h"
#include "Drawer.h"
#include "ImageProcessor.h"

#include "SuperPixel.h"
#include "Elements.h"
#include "ElementsHelper.h"

#pragma warning(push, 0)	// no warnings from includes
#include <QDebug>
#include <QPainter>
#include <QFile>
#include <QFileInfo>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <opencv2\imgproc\imgproc.hpp>
#include <opencv2/features2d/features2d.hpp>

#pragma warning(pop)

namespace rdf {

// SuperPixelLabelerConfig --------------------------------------------------------------------
SuperPixelLabelerConfig::SuperPixelLabelerConfig() : ModuleConfig("Super Pixel Trainer") {
}

QString SuperPixelLabelerConfig::toString() const {
	return ModuleConfig::toString();
}

// SuperPixelLabeler --------------------------------------------------------------------
SuperPixelLabeler::SuperPixelLabeler(const QVector<QSharedPointer<MserBlob> >& blobs, const Rect& imgRect) {

	mBlobs = blobs;
	mImgRect = imgRect;
	mConfig = QSharedPointer<SuperPixelLabelerConfig>::create();
}

bool SuperPixelLabeler::isEmpty() const {
	return mBlobs.empty();
}

bool SuperPixelLabeler::compute() {

	if (!mGtRegion) {
		qWarning() << "cannot label an empty region...";
		return false;
	}

	QImage img = createLabelImage(mImgRect);

	return true;
}

QSharedPointer<SuperPixelLabelerConfig> SuperPixelLabeler::config() const {
	return qSharedPointerDynamicCast<SuperPixelLabelerConfig>(mConfig);
}

cv::Mat SuperPixelLabeler::draw(const cv::Mat& img) const {

	// draw mser blobs
	Timer dtf;
	QPixmap pm = Image::mat2QPixmap(img);

	QPainter p(&pm);

	return Image::qImage2Mat(createLabelImage(Rect(img)));
}

QString SuperPixelLabeler::toString() const {
	return Module::toString();
}

void SuperPixelLabeler::setRootRegion(const QSharedPointer<Region>& region) {
	mGtRegion = region;
}

void SuperPixelLabeler::setLabelManager(const LabelManager & manager) {
	mManager = manager;
}

QImage SuperPixelLabeler::createLabelImage(const Rect & imgRect) const {

	if (mManager.isEmpty())
		mWarning << "label manager is empty...";

	QImage img(imgRect.size().toQSize(), QImage::Format_RGB888);
	img.fill(QColor(0, 0, 0));

	auto allRegions = Region::allRegions(mGtRegion);

	QPainter p(&img);
	for (auto region : allRegions) {

		if (!region)
			continue;

		const LabelLookup& ll = mManager.find(*region);

		if (ll.isNull()) { 
			qDebug() << "could not find region: " << region->type();
			continue;
		}
		qDebug() << "drawing color: " << ll.color();

		QColor labelC = ll.color();
		p.setPen(labelC);
		p.setBrush(labelC);
		region->polygon().draw(p);

		qDebug() << ll.id() << "==" << LabelLookup::color2Id(ll.color());
	}

	return img;
}

bool SuperPixelLabeler::checkInput() const {

	return !mBlobs.empty();
}


// SuperPixelClassificationConfig --------------------------------------------------------------------
SuperPixelClassificationConfig::SuperPixelClassificationConfig() : ModuleConfig("Super Pixel Classification") {
}

QString SuperPixelClassificationConfig::toString() const {
	return ModuleConfig::toString();
}

int SuperPixelClassificationConfig::maxSide() const {
	return mMaxSide;
}

// SuperPixelClassification --------------------------------------------------------------------
SuperPixelClassification::SuperPixelClassification(const cv::Mat& img, const PixelSet& set) {

	mImg = img;
	mSet = set;
	mConfig = QSharedPointer<SuperPixelClassificationConfig>::create();
}

bool SuperPixelClassification::isEmpty() const {
	return mImg.empty();
}

bool SuperPixelClassification::compute() {

	mWarning << "not implemented yet";
	return true;
}

QSharedPointer<SuperPixelClassificationConfig> SuperPixelClassification::config() const {
	return qSharedPointerDynamicCast<SuperPixelClassificationConfig>(mConfig);
}

cv::Mat SuperPixelClassification::draw(const cv::Mat& img) const {

	// draw mser blobs
	Timer dtf;
	QPixmap pm = Image::mat2QPixmap(img);

	QPainter p(&pm);
	
	return Image::qPixmap2Mat(pm);
}

QString SuperPixelClassification::toString() const {
	return Module::toString();
}

bool SuperPixelClassification::checkInput() const {

	return !mImg.empty();
}

// LabelLookup --------------------------------------------------------------------
LabelLookup::LabelLookup() {
}

bool LabelLookup::operator==(const LabelLookup & l1) const {
	return id() == l1.id() && name() == l1.name();
}

bool LabelLookup::isNull() const {
	return id() == -1;
}

bool LabelLookup::contains(const QString& key) const {

	if (mName == key)
		return true;

	return mAlias.contains(key);
}

QColor LabelLookup::color() const {
	QColor c(id() << 8);	// << 8 away from alpha (RGBA)
	return c;
}

int LabelLookup::color2Id(const QColor & col) {
	int ci = col.rgba();
	return ci >> 8 & 0xFFFF;
}

int LabelLookup::id() const {
	return mId;
}

QString LabelLookup::name() const {
	return mName;
}

QString LabelLookup::toString() const {

	QString str;
	str += QString::number(id()) + ", " + name() + ", ";

	for (const QString& a : mAlias)
		str += a + ", ";

	return str;
}

QDataStream& operator<<(QDataStream& s, const LabelLookup& ll) {

	s << ll.toString();
	return s;
}

QDebug operator<<(QDebug d, const LabelLookup& ll) {

	d << qPrintable(ll.toString());
	return d;
}


LabelLookup LabelLookup::fromString(const QString & str) {

	// expecting a string like:
	// #ID, #Name, #Alias1, #Alias2, ..., #AliasN
	QStringList list = str.split(",");
	
	if (list.size() < 2) {
		qWarning() << "illegal label string: " << str;
		qInfo() << "I expected: ID, Name, Alias1, ..., AliasN";
		return LabelLookup();
	}
	
	LabelLookup ll;

	// parse ID
	bool ok = false;
	ll.mId = list[0].toInt(&ok);

	if (!ok) {
		qWarning() << "first entry must be an int, but it is: " << list[0];
		return LabelLookup();
	}

	// parse name
	ll.mName = list[1];

	for (int idx = 2; idx < list.size(); idx++) {
		ll.mAlias << list[idx];
	}

	return ll;
}

QString LabelLookup::jsonKey() {
	return QString("TUWienLabelLookup");
}


// LabelManager --------------------------------------------------------------------
LabelManager::LabelManager() {
}

bool LabelManager::isEmpty() const {
	return mLookups.empty();
}

int LabelManager::size() const {
	return mLookups.size();
}

LabelManager LabelManager::read(const QString & filePath) {
	
	LabelManager manager;

	QFile file(filePath);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		QFileInfo fi(filePath);

		if (!fi.exists())
			qCritical() << filePath << "does not exist...";
		else
			qCritical() << "cannot open" << filePath;
		
		return manager;
	}

	// read the file
	QByteArray ba = file.readAll();
	QJsonDocument doc = QJsonDocument::fromJson(ba);
	if (doc.isNull() || doc.isEmpty()) {
		qCritical() << "cannot parse NULL document: " << filePath;
		return manager;
	}

	// parse the lookups
	QJsonArray labels = doc.object().value(LabelLookup::jsonKey()).toArray();
	if (labels.isEmpty()) {
		qCritical() << "cannot locate" << LabelLookup::jsonKey();
		return manager;
	}

	// parse labels
	for (const QJsonValue& cLabel : labels)
		manager.add(LabelLookup::fromString(cLabel.toString()));
	
	return manager;
}

void LabelManager::add(const LabelLookup & label) {

	if (contains(label)) {
		qInfo() << label << "already exists - ignoring...";
		return;
	}
	else if (containsId(label)) {
		qCritical() << label.id() << "already exists - rejecting" << label;
		return;
	}

	mLookups << label;
}

bool LabelManager::contains(const LabelLookup & label) const {

	for (const LabelLookup ll : mLookups) {
		if (label == ll)
			return true;
	}

	return false;
}

bool LabelManager::containsId(const LabelLookup & label) const {

	for (const LabelLookup ll : mLookups) {
		if (label.id() == ll.id())
			return true;
	}

	return false;
}

QString LabelManager::toString() const {
	
	QString str = "Label Manager ---------------------------\n";
	for (auto s : mLookups)
		str += s.toString() + "\n";
	str += "\n";
	
	return str;
}

LabelLookup LabelManager::find(const QString & str) const {
	
	// try to directly find the entry
	for (const LabelLookup ll : mLookups) {
		
		if (ll.name() == str)
			return ll;
	}

	for (const LabelLookup ll : mLookups) {

		if (ll.contains(str))
			return ll;
	}

	return LabelLookup();
}

LabelLookup LabelManager::find(const Region & r) const {

	QString name = RegionManager::instance().typeName(r.type());
	return find(name);
}

}