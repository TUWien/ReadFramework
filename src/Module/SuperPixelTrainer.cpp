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

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/ml/ml.hpp>

#pragma warning(pop)

namespace rdf {

// SuperPixelLabelerConfig --------------------------------------------------------------------
SuperPixelLabelerConfig::SuperPixelLabelerConfig() : ModuleConfig("Super Pixel Trainer") {
}

QString SuperPixelLabelerConfig::featureFilePath() const {
	return mFeatureFilePath;
}

QString SuperPixelLabelerConfig::labelConfigFilePath() const {
	return mLabelConfigFilePath;
}

int SuperPixelLabelerConfig::maxNumFeaturesPerImage() const {
	return mMaxNumFeaturesPerImage;
}

int SuperPixelLabelerConfig::minNumFeaturesPerClass() const {
	return mMinNumFeaturesPerClass;
}

int SuperPixelLabelerConfig::maxNumFeaturesPerClass() const {
	return mMaxNumFeaturesPerClass;
}

QString SuperPixelLabelerConfig::toString() const {
	return ModuleConfig::toString();
}

void SuperPixelLabelerConfig::load(const QSettings & settings) {
	
	mFeatureFilePath = settings.value("featureFilePath", mFeatureFilePath).toString();
	mLabelConfigFilePath = settings.value("labelConfigFilePath", mLabelConfigFilePath).toString();
	mMaxNumFeaturesPerImage = settings.value("maxNumFeaturesPerImage", mMaxNumFeaturesPerImage).toInt();
	mMinNumFeaturesPerClass = settings.value("minNumFeaturesPerClass", mMinNumFeaturesPerClass).toInt();
	mMaxNumFeaturesPerClass = settings.value("maxNumFeaturesPerClass", mMaxNumFeaturesPerClass).toInt();
}

void SuperPixelLabelerConfig::save(QSettings & settings) const {

	settings.setValue("featureFilePath", mFeatureFilePath);
	settings.setValue("labelConfigFilePath", mLabelConfigFilePath);
	settings.setValue("maxNumFeaturesPerImage", mMaxNumFeaturesPerImage);
	settings.setValue("minNumFeaturesPerClass", mMinNumFeaturesPerClass);
	settings.setValue("maxNumFeaturesPerClass", mMaxNumFeaturesPerClass);
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
	QJsonArray labels = Utils::readJson(filePath, LabelManager::jsonKey()).toArray();
	if (labels.isEmpty()) {
		qCritical() << "cannot locate" << LabelManager::jsonKey();
		return manager;
	}

	// parse labels
	for (const QJsonValue& cLabel : labels)
		manager.add(LabelInfo::fromJson(cLabel.toObject().value("Class").toObject()));
	
	return manager;
}

void LabelManager::toJson(QJsonObject& jo) const {

	QJsonArray ja;

	for (const LabelInfo& fc : mLookups) {
		QJsonObject cJo;
		fc.toJson(cJo);
		ja << cJo;
	}

	jo.insert(FeatureCollection::jsonKey(), ja);
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

QString LabelManager::jsonKey() {
	return "Labels";
}


// FeatureCollection --------------------------------------------------------------------
FeatureCollection::FeatureCollection(const cv::Mat & descriptors, const LabelInfo & label) {
	mDesc = descriptors;
	mLabel = label;
}

bool operator==(const FeatureCollection& fcl, const FeatureCollection& fcr) {

	return fcl.label() == fcr.label();
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

void FeatureCollection::setDescriptors(const cv::Mat & desc) {
	mDesc = desc;
}

cv::Mat FeatureCollection::descriptors() const {
	return mDesc;
}

int FeatureCollection::numDescriptors() const {
	return mDesc.rows;
}

// FeatureCollectionManager --------------------------------------------------------------------
FeatureCollectionManager::FeatureCollectionManager(const cv::Mat & descriptors, const PixelSet & set) {
	
	mCollection = FeatureCollection::split(descriptors, set);
}

bool FeatureCollectionManager::isEmpty() const {
	return mCollection.isEmpty();
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

int FeatureCollectionManager::numFeatures() const {

	int nf = 0;
	for (const FeatureCollection& fc : mCollection)
		nf += fc.numDescriptors();

	return nf;
}

/// <summary>
/// Merges two collection managers (e.g. from two images).
/// If the same labels exist, features will be appended.
/// </summary>
/// <param name="other">The other manager.</param>
void FeatureCollectionManager::merge(const FeatureCollectionManager & other) {

	for (const FeatureCollection& col : other.collection()) {

		int idx = mCollection.indexOf(col);

		// append features, if the collection exists already
		if (idx != -1)
			mCollection[idx].append(col.descriptors());
		else
			add(col);

	}
}

void FeatureCollectionManager::normalize(int minFeaturesPerClass, int maxFeaturesPerClass) {

	QVector<int> removeIdx;
	for (int idx = 0; idx < mCollection.size(); idx++) {

		int nd = mCollection[idx].numDescriptors();
		if (nd < minFeaturesPerClass) {
			removeIdx << idx;
		}
		else if (nd > maxFeaturesPerClass) {
			cv::Mat desc = mCollection[idx].descriptors();
			cv::resize(desc, desc, cv::Size(desc.cols, maxFeaturesPerClass), 0.0, 0.0, CV_INTER_NN);
			mCollection[idx].setDescriptors(desc);
			qInfo() << mCollection[idx].label().name() << nd << "->" << desc.rows << "features";
		}
	}

	qSort(removeIdx.begin(), removeIdx.end(), qGreater<int>());
	for (int ri : removeIdx) {
		qInfo() << mCollection[ri].label().name() << "removed since it has too few features: " << mCollection[ri].numDescriptors();
		mCollection.remove(ri);
	}
}

QString FeatureCollectionManager::toString() const {

	QString str = "Feature Collection Manager ---------------------\n";
	for (auto fc : mCollection)
		str += fc.label().name() + " " + QString::number(fc.descriptors().rows) + " features | ";

	return str;
}

cv::Ptr<cv::ml::TrainData> FeatureCollectionManager::toCvTrainData(int maxSamples) const {

	assert(numFeatures() > 0);
	cv::Mat features = allFeatures();
	cv::Mat labels = allLabels();

	if (features.rows > maxSamples) {
		cv::resize(features, features, cv::Size(features.cols, maxSamples), 0.0, 0.0, CV_INTER_NN);
		cv::resize(labels, labels, cv::Size(labels.cols, maxSamples), 0.0, 0.0, CV_INTER_NN);
	}

	assert(features.rows == labels.rows);

	cv::Ptr<cv::ml::TrainData> td = cv::ml::TrainData::create(features, cv::ml::ROW_SAMPLE, labels);
	return td;
}

LabelManager FeatureCollectionManager::toLabelManager() const {

	LabelManager lm;

	for (const FeatureCollection& fc : mCollection)
		lm.add(fc.label());

	return lm;
}

/// <summary>
/// Merges all features for training.
/// </summary>
/// <returns></returns>
cv::Mat FeatureCollectionManager::allFeatures() const {

	cv::Mat features;

	for (const FeatureCollection& fc : mCollection) {
		
		if (features.empty())
			features = fc.descriptors();
		else {
			assert(fc.descriptors().cols == features.cols);
			features.push_back(fc.descriptors());
		}
	}

	double maxV, minV;
	cv::minMaxLoc(features, &minV, &maxV);

	// normalize
	if (minV != maxV) {
		double dr = 1.0 / (maxV - minV);
		features.convertTo(features, CV_32FC1, dr, -dr * minV);
	}
	else {
		qWarning() << "I seem to get weird values here: ";
		features.convertTo(features, CV_32FC1);
		Image::imageInfo(features, "features");
	}

	return features;
}

/// <summary>
/// Returns the labels in a OpenCV compilant format.
/// </summary>
/// <returns></returns>
cv::Mat FeatureCollectionManager::allLabels() const {

	cv::Mat labels;

	for (const FeatureCollection& fc : mCollection) {

		cv::Mat cLabels(fc.descriptors().rows, 1, CV_32SC1, cv::Scalar(fc.label().id()));

		if (labels.empty())
			labels = cLabels;
		else
			labels.push_back(cLabels);
	}

	return labels;
}

// SuperPixelTrainerConfig --------------------------------------------------------------------
SuperPixelTrainerConfig::SuperPixelTrainerConfig() : ModuleConfig("SuperPixelTrainer") {
}

QString SuperPixelTrainerConfig::toString() const {
	return ModuleConfig::toString();
}

void SuperPixelTrainerConfig::load(const QSettings &) {
	// TODO: add load/save here
}

void SuperPixelTrainerConfig::save(QSettings &) const {
	// TODO: add load/save here
}

// SuperPixelTrainer --------------------------------------------------------------------
SuperPixelTrainer::SuperPixelTrainer(const FeatureCollectionManager & fcm) {
	mFeatureManager = fcm;
	mConfig = QSharedPointer<SuperPixelTrainerConfig>::create();

}

bool SuperPixelTrainer::isEmpty() const {
	return mFeatureManager.isEmpty();
}

bool SuperPixelTrainer::compute() {

	if (!checkInput())
		return false;

	Timer dt;
	
	mTrainer = cv::ml::RTrees::create();

	if (mFeatureManager.numFeatures() == 0) {
		qCritical() << "Cannot train random trees if no feature vectors are provided";
		return false;
	}

	mTrainer->train(mFeatureManager.toCvTrainData(100));
	mTrainer->save("C:/temp/rt.yml");

	mInfo << "trained in" << dt;

	return true;
}

QSharedPointer<SuperPixelTrainerConfig> SuperPixelTrainer::config() const {
	return qSharedPointerDynamicCast<SuperPixelTrainerConfig>(Module::config());
}

cv::Mat SuperPixelTrainer::draw(const cv::Mat & img) const {

	// draw mser blobs
	Timer dtf;
	QPixmap pm = Image::mat2QPixmap(img);

	QPainter p(&pm);
	// TODO: draw something

	return Image::qPixmap2Mat(pm);
}

QString SuperPixelTrainer::toString() const {
	return config()->toString();
}

bool SuperPixelTrainer::write(const QString & filePath) const {

	if (!mTrainer->isTrained())
		qWarning() << "writing trainer that is NOT trained!";

	// write all label data
	QJsonObject jo;
	LabelManager lm = mFeatureManager.toLabelManager();
	lm.toJson(jo);
	
	// write RTrees classifier
	toJson(jo);

	int64 bw = Utils::writeJson(filePath, jo);

	return bw > 0;	// if we wrote more than 0 bytes, it's ok
}

void SuperPixelTrainer::toJson(QJsonObject& jo) const {

	cv::FileStorage fs(".yml", cv::FileStorage::WRITE | cv::FileStorage::MEMORY | cv::FileStorage::FORMAT_YAML);
	mTrainer->write(fs);
	std::string data = fs.releaseAndGetString();

	QByteArray ba(data.c_str(), (int)data.length());
	QString ba64Str = ba.toBase64();
	
	jo.insert(name(), ba64Str);
}

bool SuperPixelTrainer::checkInput() const {
	return !isEmpty();
}

}