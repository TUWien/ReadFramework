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
	img.fill(LabelLookup::backgroundLabel().color());

	auto allRegions = Region::allRegions(mGtRegion);

	QPainter p(&img);
	for (auto region : allRegions) {

		if (!region)
			continue;

		LabelLookup ll = mManager.find(*region);

		if (ll.isNull()) { 
			qDebug() << "could not find region: " << region->type();
			ll = LabelLookup::ignoreLabel();
		}
		
		QColor labelC = ll.color();
		p.setPen(labelC);
		p.setBrush(labelC);
		region->polygon().draw(p);
	}

	return img;
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

		Image::save(mask, "C:/temp/mask.png");

		// find the blob's label
		QColor col = IP::statMomentColor(labelBBox, mask);
		int id = LabelLookup::color2Id(col);

		// assign ground truth & convert to pixel
		PixelLabel label;
		label.setTrueLabel(mManager.find(id));
		QSharedPointer<Pixel> px = cb->toPixel();
		px->setLabel(label);
		set.add(px);

		//if (label.trueLabel().id() != LabelLookup::label_unknown)
		//	qDebug() << "I found" << label.trueLabel();

	}
	
	return set;
}

// LabelManager --------------------------------------------------------------------
LabelManager::LabelManager() {
	add(LabelLookup::backgroundLabel());
	add(LabelLookup::ignoreLabel());
	add(LabelLookup::unknownLabel());
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
		manager.add(LabelLookup::fromJson(cLabel.toObject()));
	
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

LabelLookup LabelManager::find(int id) const {
	
	// try to directly find the entry
	for (const LabelLookup ll : mLookups) {

		if (ll.id() == id)
			return ll;
	}

	return LabelLookup();
}


}