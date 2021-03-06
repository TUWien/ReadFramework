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
#include <QDir>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <opencv2/imgproc.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/ml.hpp>

#pragma warning(pop)

namespace rdf {

// SuperPixelLabelerConfig --------------------------------------------------------------------
/// <summary>
/// Initializes a new instance of the <see cref="SuperPixelLabelerConfig"/> class.
/// This class configures the feature collection process.
/// </summary>
SuperPixelLabelerConfig::SuperPixelLabelerConfig() : ModuleConfig("Super Pixel Labeler") {
}

/// <summary>
/// Full file path to the feature cache file.
/// This file is the collection's output and contains
/// all features collected.
/// </summary>
/// <returns></returns>
QString SuperPixelLabelerConfig::featureFilePath() const {
	return mFeatureFilePath;
}

/// <summary>
/// Full file path to the label config file.
/// The label config file configures the labeling process (e.g. if TextRegion is handwritten or printed).
/// </summary>
/// <returns></returns>
QString SuperPixelLabelerConfig::labelConfigFilePath() const {
	return mLabelConfigFilePath;
}

/// <summary>
/// Sepcify the maximum number of images collected per image.
/// </summary>
/// <returns></returns>
int SuperPixelLabelerConfig::maxNumFeaturesPerImage() const {
	return mMaxNumFeaturesPerImage;
}

/// <summary>
/// If less features are found for a class, it is not saved.
/// </summary>
/// <returns></returns>
int SuperPixelLabelerConfig::minNumFeaturesPerClass() const {
	return mMinNumFeaturesPerClass;
}

/// <summary>
/// The maximum number of features collected per class.
/// </summary>
/// <returns></returns>
int SuperPixelLabelerConfig::maxNumFeaturesPerClass() const {
	return mMaxNumFeaturesPerClass;
}

QString SuperPixelLabelerConfig::backgroundLabelName() const {
	return mBackgroundLabelName;
}

QString SuperPixelLabelerConfig::toString() const {
	return ModuleConfig::toString();
}

void SuperPixelLabelerConfig::load(const QSettings & settings) {
	
	mFeatureFilePath = settings.value("featureFilePath", mFeatureFilePath).toString();
	mLabelConfigFilePath = settings.value("labelConfigFilePath", mLabelConfigFilePath).toString();
	mBackgroundLabelName = settings.value("backgroundLabelName", mBackgroundLabelName).toString();
	mMaxNumFeaturesPerImage = settings.value("maxNumFeaturesPerImage", mMaxNumFeaturesPerImage).toInt();
	mMinNumFeaturesPerClass = settings.value("minNumFeaturesPerClass", mMinNumFeaturesPerClass).toInt();
	mMaxNumFeaturesPerClass = settings.value("maxNumFeaturesPerClass", mMaxNumFeaturesPerClass).toInt();
}

void SuperPixelLabelerConfig::save(QSettings & settings) const {

	settings.setValue("featureFilePath", mFeatureFilePath);
	settings.setValue("labelConfigFilePath", mLabelConfigFilePath);
	settings.setValue("backgroundLabelName", mBackgroundLabelName);
	settings.setValue("maxNumFeaturesPerImage", mMaxNumFeaturesPerImage);
	settings.setValue("minNumFeaturesPerClass", mMinNumFeaturesPerClass);
	settings.setValue("maxNumFeaturesPerClass", mMaxNumFeaturesPerClass);
}

// SuperPixelLabeler --------------------------------------------------------------------
SuperPixelLabeler::SuperPixelLabeler(const QVector<QSharedPointer<MserBlob> >& blobs, const Rect& imgRect) {

	mBlobs = blobs;
	mImgRect = imgRect;
	mConfig = QSharedPointer<SuperPixelLabelerConfig>::create();
	mConfig->loadSettings();

	mGlobalName = config()->backgroundLabelName();
}

SuperPixelLabeler::SuperPixelLabeler(const PixelSet& set, const Rect& imgRect) {

	mSet = set;
	mImgRect = imgRect;
	mConfig = QSharedPointer<SuperPixelLabelerConfig>::create();
	mConfig->loadSettings();

	mGlobalName = config()->backgroundLabelName();
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
	
	if (!mBlobs.empty())
		mSet = labelBlobs(labelImg, mBlobs);
	else if (!mSet.isEmpty())
		mSet = labelPixels(labelImg, mSet);

	return true;
}

QSharedPointer<SuperPixelLabelerConfig> SuperPixelLabeler::config() const {
	return qSharedPointerDynamicCast<SuperPixelLabelerConfig>(mConfig);
}

cv::Mat SuperPixelLabeler::draw(const cv::Mat& img, bool drawPixels) const {

	// draw mser blobs
	Timer dtf;
	QImage qImg = Image::mat2QImage(img, true);

	QImage labelImgQt = createLabelImage(mImgRect, true);

	QPainter p(&qImg);

	p.setOpacity(0.4);
	p.drawImage(QPoint(), labelImgQt);
	p.setOpacity(1.0);
	p.setPen(ColorManager::red());
	
	if (drawPixels)
		mSet.draw(p, PixelSet::draw_pixels);

	// draw legend
	mManager.draw(p);

	//for (auto b : mBlobs)
	//	p.drawRect(Converter::cvRectToQt(b->bbox().toCvRect()));

	return Image::qImage2Mat(qImg);//Image::qImage2Mat(createLabelImage(Rect(img)));
}

QString SuperPixelLabeler::toString() const {
	return Module::toString();
}

/// <summary>
/// Parses the filePath for GT labels.
/// If a label is found, the background of
/// label image is set to the GT label.
/// </summary>
/// <param name="filePath">The file path.</param>
void SuperPixelLabeler::setFilePath(const QString & filePath) {
	QString name = parseLabel(filePath);
	setBackgroundLabelName(name);
}

/// <summary>
/// Sets the name of the background label.
/// This is useful if the GT is specified on
/// image level.
/// </summary>
/// <param name="name">The label name.</param>
void SuperPixelLabeler::setBackgroundLabelName(const QString & name) {
	mGlobalName = name;
}

void SuperPixelLabeler::setRootRegion(const QSharedPointer<RootRegion>& region) {
	mGtRegion = region;
}

void SuperPixelLabeler::setLabelManager(const LabelManager & manager) {
	mManager = manager;
}

void SuperPixelLabeler::setPage(const QSharedPointer<PageElement>& page) {
	mPage = page;
}

QImage SuperPixelLabeler::createLabelImage(const Rect & imgRect, bool visualize) const {

	if (mManager.isEmpty())
		mWarning << "label manager is empty...";

	LabelInfo bgLabel = mManager.find(mGlobalName);

	if (bgLabel.isNull())
		bgLabel = mManager.backgroundLabel();

	QImage img(imgRect.size().toQSize(), QImage::Format_RGB888);
	img.fill(bgLabel.color());

	QVector<QSharedPointer<Region>> allRegions;
	
	// if there are no layers specified, the label regions are drawn in the order in which they occur in the xml source
	if (!mPage || mPage->layers().isEmpty()) {
		allRegions = Region::allRegions(mGtRegion.data());
	}
	// otherwise, regions are drawn in order of their layer zIndex
	else {
		// layers should be sorted by zIndex
		mPage->sortLayers(true);
		allRegions.append(mPage->defaultLayer()->regions());
		for (const auto& layer : mPage->layers()) {
			allRegions.append(layer->regions());
		}
	}

	QMap<int, QVector<QSharedPointer<Region> > > mapRegions;

	for (auto region : allRegions) {

		if (!region)
			continue;

		LabelInfo ll = mManager.find(*region);

		if (ll == LabelInfo())
			continue;

		if (ll.isNull()) {
			qDebug() << "could not find region: " << RegionManager::instance().typeName(region->type());
			continue;
		}
		
		mapRegions[ll.zIndex()] << region;
	}


	QPainter p(&img);
	// this allows for overlaying multiple classes
	//p.setCompositionMode(QPainter::CompositionMode_Plus);

	for (QVector<QSharedPointer<Region> > cRegions : mapRegions) {

		for (auto region : cRegions) {
			
			LabelInfo ll = mManager.find(*region);

			// draw the current region
			QColor labelC = (!visualize) ? ll.color() : ll.visColor();
			p.setPen(labelC);
			p.setBrush(labelC);
			region->polygon().draw(p);
		}
	}

	if (visualize)
		mManager.draw(p);

	return img;
}

PixelSet SuperPixelLabeler::set() const {
	return mSet;
}

/// <summary>
/// Parses the filePath for potential GT labels.
/// Labels have to be between braces []:
/// C:\images\[printed]\img001.jpg
/// C:\images\img001[handwritten].jpg
/// If a label was extracted, but could not be found in 
/// the label config, the LabelInfo::unknownLabel() name
/// is returned.
/// </summary>
/// <param name="filePath">The file path.</param>
/// <returns></returns>
QString SuperPixelLabeler::parseLabel(const QString & filePath) const {

	LabelInfo labelInfo;

	QRegExp re("\\[(.*)\\]");
	int pos = re.indexIn(filePath);	// match it

	if (pos == -1) {
		qInfo() << "no label found in" << filePath;
		return labelInfo.name();
	}

	QStringList labels = re.capturedTexts();

	//if (labels.size() > 1)
	//	qWarning() << "I have found more than one potential label:" << labels.join(",");

	for (const QString& labelStr : labels) {
		labelInfo = mManager.find(labelStr);

		if (!labelInfo.isNull()) {
			qInfo() << "GT label found in filepath:" << labelStr;
			break;
		}
	}

	if (labelInfo.isNull())
		qWarning() << "I have found a potential label, but could not match it:" << labels.join(" ");

	return labelInfo.name();
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
		QSharedPointer<Pixel> px = cb->toPixel();
		QSharedPointer<PixelLabel> l = px->label();
		l->setTrueLabel(mManager.find(id));
		set.add(px);
	}
	
	return set;
}

PixelSet SuperPixelLabeler::labelPixels(const cv::Mat & labelImg, const PixelSet& set) const {

	PixelSet setL;
	int rCnt = 0;

	for (const auto& px : set.pixels()) {

		Rect r = px->bbox().clipped(labelImg.size());
		cv::Mat labelBBox = labelImg(r.toCvRect());
		cv::Mat mask = px->toBinaryMask(r);

		// clip mask
		if (r != px->bbox())
			mask = mask(Rect(Vector2D(), r.size()).toCvRect());

		// find the blob's label
		QColor col1 = IP::statMomentColor(labelBBox, mask, 0.2);
		QColor col2 = IP::statMomentColor(labelBBox, mask, 0.8);

		// if less than 60% of the blob's area is consistent, we reject the blob
		if (col1 != col2) {
			rCnt++;
			continue;
		}

		// col1 == col2
		int id = LabelInfo::color2Id(col1);

		// assign ground truth
		QSharedPointer<PixelLabel> l = px->label();
		l->setTrueLabel(mManager.find(id));
		setL << px;
	}

	qDebug() << rCnt << "rejected because they have an ambigous GT class";

	return set;
}

// FeatureCollection --------------------------------------------------------------------
FeatureCollection::FeatureCollection(const cv::Mat & descriptors, const LabelInfo & label) {
	mDesc = descriptors;
	mLabel = label;
}

bool operator==(const FeatureCollection& fcl, const FeatureCollection& fcr) {

	return fcl.label() == fcr.label();
}

QJsonObject FeatureCollection::toJson(const QString& filePath) const {

	QJsonObject jo;
	mLabel.toJson(jo);

	// embed features
	if (filePath == "")
		jo.insert("descriptors", Image::matToJson(mDesc));
	else {

		// write data to external file
		QString fp = Utils::createFilePath(filePath, "-" + Utils::timeStampFileName(mLabel.name(), ""), "rdf");
		Image::writeMat(mDesc, fp);

		QFileInfo fi(fp);
		jo.insert("descriptors", Image::matToJsonExtern(mDesc, fi.fileName()));
	}

	return jo;
}

FeatureCollection FeatureCollection::read(QJsonObject & jo, const QString& filePath) {

	QString path = QFileInfo(filePath).absolutePath();

	FeatureCollection fc;
	fc.mLabel = LabelInfo::fromJson(jo.value("Class").toObject());
	fc.mDesc = Image::jsonToMat(jo.value("descriptors").toObject(), path);

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
		const LabelInfo& cLabel = px->label()->trueLabel();
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

	// NOTE: JSON objects have a size limit ~40MB
	for (const FeatureCollection& fc : mCollection) {
		ja << fc.toJson(filePath);
	}
	
	QJsonObject jo;
	jo.insert(FeatureCollection::jsonKey(), ja);

	Utils::writeJson(filePath, jo);
}

FeatureCollectionManager FeatureCollectionManager::read(const QString & filePath) {

	FeatureCollectionManager manager;

	// parse the feature collections
	QJsonArray labels = Utils::readJson(filePath).value(FeatureCollection::jsonKey()).toArray();
	if (labels.isEmpty()) {
		qCritical() << "cannot locate" << FeatureCollection::jsonKey();
		return manager;
	}

	// parse labels
	for (const QJsonValue& cLabel : labels) {
		QJsonObject co = cLabel.toObject();
		manager.add(FeatureCollection::read(co, filePath));
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
		if (nd < minFeaturesPerClass || mCollection[idx].label() == LabelInfo::unknownLabel()) {
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

		if (mCollection[ri].label() == LabelInfo::unknownLabel()) {
			qInfo() << mCollection[ri].label().name() << "removed since it is 'unknown'";
		}
		else {
			qInfo() << mCollection[ri].label().name() <<
				"removed since it has too few features: " <<
				mCollection[ri].numDescriptors() <<
				"minimum:" << minFeaturesPerClass;
		}
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

	if (maxSamples != -1 && features.rows > maxSamples) {
		int nFeatures = features.rows;
		cv::resize(features, features, cv::Size(features.cols, maxSamples), 0.0, 0.0, CV_INTER_NN);
		cv::resize(labels, labels, cv::Size(labels.cols, maxSamples), 0.0, 0.0, CV_INTER_NN);
		qInfo() << nFeatures << "features reduced to" << features.rows;
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
		
		// ignore unknown
		if (fc.label().id() == LabelInfo::label_unknown)
			continue;

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

		// ignore unknown
		if (fc.label().id() == LabelInfo::label_unknown)
			continue;

		cv::Mat cLabels(fc.descriptors().rows, 1, CV_32SC1);
		cLabels.setTo(fc.label().id());

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

QStringList SuperPixelTrainerConfig::featureCachePaths() const {
	return mFeatureCachePaths;
}

QString SuperPixelTrainerConfig::modelPath() const {
	return mModelPath;
}

QString SuperPixelTrainerConfig::toString() const {

	QString msg = ModuleConfig::toString();
	msg += "feature files:\n" + featureCachePaths().join("\n");

	return msg;
}

void SuperPixelTrainerConfig::setNumTrees(int numTrees) {
	mNumTrees = numTrees;
}

int SuperPixelTrainerConfig::numTrees() const {
	return mNumTrees;
}

void SuperPixelTrainerConfig::load(const QSettings & settings) {

	QString paths = settings.value("featureCachePaths", mFeatureCachePaths.join(",")).toString();
	mFeatureCachePaths = paths.split(",");
	mModelPath = settings.value("modelPath", mModelPath).toString();
	mNumTrees = settings.value("numTrees", mNumTrees).toInt();
}

void SuperPixelTrainerConfig::save(QSettings & settings) const {
	
	settings.setValue("featureCachePaths", mFeatureCachePaths.join(","));
	settings.setValue("modelPath", mModelPath);
	settings.setValue("numTrees", mNumTrees);
}

// SuperPixelTrainer --------------------------------------------------------------------
SuperPixelTrainer::SuperPixelTrainer(const FeatureCollectionManager & fcm) {
	mFeatureManager = fcm;
	mConfig = QSharedPointer<SuperPixelTrainerConfig>::create();
	mConfig->loadSettings();
}

bool SuperPixelTrainer::isEmpty() const {
	return mFeatureManager.isEmpty();
}

bool SuperPixelTrainer::compute() {

	if (!checkInput())
		return false;

	Timer dt;
	
	mModel = cv::ml::RTrees::create();
	
	// TODO: validate!
	cv::TermCriteria tc(cv::TermCriteria::COUNT, config()->numTrees(), 1e-6);
	mModel->setTermCriteria(tc);
	qInfo() << "training RF with" << config()->numTrees() << "trees";

	if (mFeatureManager.numFeatures() == 0) {
		qCritical() << "Cannot train random trees if no feature vectors are provided";
		return false;
	}

	mInfo << "training model with" << mFeatureManager.numFeatures() << "features, this might take a while...";

	mModel->train(mFeatureManager.toCvTrainData());

	// Print variable importance
	cv::Mat vi = mModel->getVarImportance();
	if(!vi.empty()) {
		
		double viSum = cv::sum(vi)[0];
		qInfo() << "var#\timportance (in %%):";
		int i, n = (int)vi.total();
		for (i = 0; i < n; i++ )
			qInfo() << i << "\t" << 100.f * vi.at<float>(i)/viSum;

	}
	qInfo() << "num trees (nodes):" << mModel->getNodes().size();

	mInfo << "trained in" << dt;

	return true;
}

QSharedPointer<SuperPixelTrainerConfig> SuperPixelTrainer::config() const {
	return qSharedPointerDynamicCast<SuperPixelTrainerConfig>(Module::config());
}

cv::Mat SuperPixelTrainer::draw(const cv::Mat & img) const {

	// draw mser blobs
	Timer dtf;
	QImage qImg = Image::mat2QImage(img, true);

	QPainter p(&qImg);
	// TODO: draw something

	return Image::qImage2Mat(qImg);
}

QString SuperPixelTrainer::toString() const {
	return config()->toString();
}

bool SuperPixelTrainer::write(const QString & filePath) const {

	if (mModel && !mModel->isTrained())
		qWarning() << "writing trainer that is NOT trained!";

	return model()->write(filePath);
}

QSharedPointer<SuperPixelModel> SuperPixelTrainer::model() const {
	
	QSharedPointer<SuperPixelModel> sm(new SuperPixelModel(mFeatureManager.toLabelManager(), mModel));
	return sm;
}

bool SuperPixelTrainer::checkInput() const {
	return !isEmpty();
}

}