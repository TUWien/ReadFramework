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

#include "SuperPixelTrainer.h"

#include "Utils.h"
#include "Image.h"
#include "Drawer.h"
#include "ImageProcessor.h"

#include "SuperPixel.h"
#include "Elements.h"
#include "ElementsHelper.h"
#include "Algorithms.h"

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

	QImage labelImgQt = createLabelImage(mImgRect);
	cv::Mat labelImg = Image::qImage2Mat(labelImgQt);
	mSet = labelBlobs(labelImg, mBlobs);

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
	mSet.draw(p);

	//for (auto b : mBlobs)
	//	p.drawRect(Converter::cvRectToQt(b->bbox().toCvRect()));

	return Image::qPixmap2Mat(pm);//Image::qImage2Mat(createLabelImage(Rect(img)));
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
	img.fill(LabelInfo::backgroundLabel().color());

	auto allRegions = Region::allRegions(mGtRegion);

	QPainter p(&img);
	for (auto region : allRegions) {

		if (!region)
			continue;

		LabelInfo ll = mManager.find(*region);

		if (ll.isNull()) { 
			qDebug() << "could not find region: " << region->type();
			ll = LabelInfo::ignoreLabel();
		}
		
		QColor labelC = ll.color();
		p.setPen(labelC);
		p.setBrush(labelC);
		region->polygon().draw(p);
	}

	return img;
}

PixelSet SuperPixelLabeler::set() const {
	return mSet;
}

bool SuperPixelLabeler::checkInput() const {

	return !mBlobs.empty();
}

PixelSet SuperPixelLabeler::labelBlobs(const cv::Mat & labelImg, const QVector<QSharedPointer<MserBlob> >& blobs) const {
	
	PixelSet set;

	for (const QSharedPointer<MserBlob>& cb : blobs) {

		assert(cb);
		Rect r = cb->bbox().clipped(labelImg.size());
		cv::Mat labelBBox = labelImg(r.toCvRect());
		cv::Mat mask = cb->toBinaryMask();

		// find the blob's label
		QColor col = IP::statMomentColor(labelBBox, mask);
		int id = LabelInfo::color2Id(col);

		// assign ground truth & convert to pixel
		PixelLabel label;
		label.setTrueLabel(mManager.find(id));
		QSharedPointer<Pixel> px = cb->toPixel();
		px->setLabel(label);
		set.add(px);
	}
	
	return set;
}

// LabelManager --------------------------------------------------------------------
LabelManager::LabelManager() {
	add(LabelInfo::backgroundLabel());
	add(LabelInfo::ignoreLabel());
	add(LabelInfo::unknownLabel());
}

bool LabelManager::isEmpty() const {
	return mLookups.empty();
}

int LabelManager::size() const {
	return mLookups.size();
}

LabelManager LabelManager::read(const QString & filePath) {
	
	LabelManager manager;

	// parse the lookups
	QJsonArray labels = Utils::readJson(filePath,LabelInfo::jsonKey()).toArray();
	if (labels.isEmpty()) {
		qCritical() << "cannot locate" << LabelInfo::jsonKey();
		return manager;
	}

	// parse labels
	for (const QJsonValue& cLabel : labels)
		manager.add(LabelInfo::fromJson(cLabel.toObject().value("Class").toObject()));
	
	return manager;
}

void LabelManager::add(const LabelInfo & label) {

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

bool LabelManager::contains(const LabelInfo & label) const {

	for (const LabelInfo ll : mLookups) {
		if (label == ll)
			return true;
	}

	return false;
}

bool LabelManager::containsId(const LabelInfo & label) const {

	for (const LabelInfo ll : mLookups) {
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

LabelInfo LabelManager::find(const QString & str) const {
	
	// try to directly find the entry
	for (const LabelInfo ll : mLookups) {
		
		if (ll.name() == str)
			return ll;
	}

	for (const LabelInfo ll : mLookups) {

		if (ll.contains(str))
			return ll;
	}

	return LabelInfo();
}

LabelInfo LabelManager::find(const Region & r) const {

	QString name = RegionManager::instance().typeName(r.type());
	return find(name);
}

LabelInfo LabelManager::find(int id) const {
	
	// try to directly find the entry
	for (const LabelInfo ll : mLookups) {

		if (ll.id() == id)
			return ll;
	}

	return LabelInfo();
}


// FeatureCollection --------------------------------------------------------------------
FeatureCollection::FeatureCollection(const cv::Mat & descriptors, const LabelInfo & label) {
	mDesc = descriptors;
	mLabel = label;
}

QJsonObject FeatureCollection::toJson() const {

	QJsonObject jo;
	mLabel.toJson(jo);
	jo.insert("descriptors", Image::matToJson(mDesc));

	return jo;
}

FeatureCollection FeatureCollection::read(QJsonObject & jo) {

	FeatureCollection fc;
	fc.mLabel = LabelInfo::fromJson(jo.value("Class").toObject());
	fc.mDesc = Image::jsonToMat(jo.value("descriptors").toObject());

	return fc;
}

/// <summary>
/// Splits the descriptors according to their trueLabels.
/// </summary>
/// <param name="descriptors">The descriptors.</param>
/// <param name="set">The pixel set. NOTE: set & descriptors must be synced descriptors.rows == set.size()</param>
/// <returns></returns>
QVector<FeatureCollection> FeatureCollection::split(const cv::Mat & descriptors, const PixelSet & set) {
	
	assert(descriptors.rows == set.size());
	QVector<FeatureCollection> collections;

	for (int idx = 0; idx < set.size(); idx++) {

		const QSharedPointer<Pixel> px = set.pixels()[idx];
		const LabelInfo& cLabel = px->label().trueLabel();
		bool isNew = true;
		
		for (FeatureCollection& fc : collections) {
			if (fc.label() == cLabel) {
				fc.append(descriptors.row(idx));
				isNew = false;
			}
		}

		if (isNew) {
			collections.append(FeatureCollection(descriptors.row(idx), cLabel));
		}
	}

	return collections;
}

QString FeatureCollection::jsonKey() {
	return "FeatureCollection";
}

void FeatureCollection::append(const cv::Mat & descriptor) {
	mDesc.push_back(descriptor);
}

LabelInfo FeatureCollection::label() const {
	return mLabel;
}

// FeatureCollectionManager --------------------------------------------------------------------
FeatureCollectionManager::FeatureCollectionManager(const cv::Mat & descriptors, const PixelSet & set) {
	
	mCollection = FeatureCollection::split(descriptors, set);
}

void FeatureCollectionManager::write(const QString & filePath) const {

	QJsonArray ja;

	for (const FeatureCollection& fc : mCollection)
		ja << fc.toJson();
	
	QJsonObject jo;
	jo.insert(FeatureCollection::jsonKey(), ja);

	Utils::writeJson(filePath, jo);
}

FeatureCollectionManager FeatureCollectionManager::read(const QString & filePath) {

	FeatureCollectionManager manager;

	// parse the feature collections
	QJsonArray labels = Utils::readJson(filePath, FeatureCollection::jsonKey()).toArray();
	if (labels.isEmpty()) {
		qCritical() << "cannot locate" << FeatureCollection::jsonKey();
		return manager;
	}

	// parse labels
	for (const QJsonValue& cLabel : labels) {
		QJsonObject co = cLabel.toObject();
		manager.add(FeatureCollection::read(co));
	}

	return manager;
}

void FeatureCollectionManager::add(const FeatureCollection & collection) {
	mCollection << collection;
}

QVector<FeatureCollection> FeatureCollectionManager::collection() const {
	return mCollection;
}

}