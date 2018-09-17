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

#include "PixelLabel.h"

#include "ImageProcessor.h"
#include "Elements.h"
#include "ElementsHelper.h"
#include "Image.h"
#include "Utils.h"

#pragma warning(push, 0)	// no warnings from includes
#include <QJsonObject>		// needed for LabelInfo
#include <QJsonDocument>	// needed for LabelInfo
#include <QJsonArray>		// needed for LabelInfo
#include <QPainter>

#include <QDebug>

#include <opencv2/ml.hpp>
#pragma warning(pop)

namespace rdf {

// LabelInfo --------------------------------------------------------------------
LabelInfo::LabelInfo(int id, const QString& name) {

	mId = id;

	if (!name.isEmpty())
		mName = name;
}

bool LabelInfo::operator==(const LabelInfo & l1) const {
	return id() == l1.id() && name() == l1.name();
}

bool LabelInfo::operator!=(const LabelInfo & l1) const {
	return !(*this == l1);
}

bool LabelInfo::isNull() const {
	return id() == -1 || id() == label_unknown;
}

bool LabelInfo::contains(const QString& key) const {

	if (mName == key)
		return true;

	return mAlias.contains(key);
}

QColor LabelInfo::color() const {
	QColor c(id() << 8);	// << 8 away from alpha (RGBA)
	return c;
}

QColor LabelInfo::visColor() const {
	return mVisColor;
}

bool LabelInfo::isBackground() const {
	return mIsBackground;
}

int LabelInfo::color2Id(const QColor & col) {
	int ci = col.rgba();
	return ci >> 8 & 0xFFFF;
}

LabelInfo LabelInfo::unknownLabel() {
	LabelInfo ll(label_unknown, QObject::tr("Unknown"));
	ll.mId = 0;
	ll.mVisColor = ColorManager::red();

	return ll;
}

int LabelInfo::id() const {
	return mId;
}

QString LabelInfo::name() const {
	return mName;
}

QString LabelInfo::toString() const {

	QString str;
	str += QString::number(id()) + " " + name() + ": ";

	for (const QString& a : mAlias)
		str += a + ", ";

	if (isBackground())
		str += "[background]";

	return str;
}

QDataStream& operator<<(QDataStream& s, const LabelInfo& ll) {

	s << ll.toString();
	return s;
}

QDebug operator<<(QDebug d, const LabelInfo& ll) {

	d << qPrintable(ll.toString());
	return d;
}

LabelInfo LabelInfo::fromString(const QString & str) {

	// expecting a string like:
	// #ID, #Name, #Alias1, #Alias2, ..., #AliasN
	QStringList list = str.split(",");

	if (list.size() < 2) {
		qWarning() << "illegal label string: " << str;
		qInfo() << "I expected: ID, Name, Alias1, ..., AliasN";
		return LabelInfo();
	}

	LabelInfo ll;

	// parse ID
	bool ok = false;
	ll.mId = list[0].toInt(&ok);

	if (!ok) {
		qWarning() << "first entry must be an int, but it is: " << list[0];
		return LabelInfo();
	}

	// parse name
	ll.mName = list[1];

	for (int idx = 2; idx < list.size(); idx++) {
		ll.mAlias << list[idx];
	}

	return ll;
}

LabelInfo LabelInfo::fromJson(const QJsonObject & jo) {

	//"Class": {
	//	"id": 5,
	//	"name": "image",
	//	"alias": ["ImageRegion", "ChartRegion", "GraphicRegion"],
	//	"color": "#990066", 
	//},

	LabelInfo ll;
	ll.mId = jo.value("id").toInt(label_unknown);
	ll.mName = jo.value("name").toString();
	ll.mIsBackground = jo.value("isBackground").toBool(false);
	ll.mVisColor.setNamedColor(jo.value("color").toString());

	for (const QJsonValue& jv : jo.value("alias").toArray()) {
		const QString alias = jv.toString();
		if (!alias.isEmpty())
			ll.mAlias << alias;
	}

	// print warning
	if (ll.id() == label_unknown && ll.mName.compare("unknown", Qt::CaseInsensitive)) {
		QJsonDocument jd(jo);
		qCritical().noquote() << "could not parse" << jd.toJson();
		return LabelInfo::unknownLabel();
	}

	return ll;
}

void LabelInfo::toJson(QJsonObject & jo) const {

	QJsonObject joc;
	joc.insert("id", QJsonValue(mId));
	joc.insert("name", QJsonValue(mName));
	joc.insert("color", QJsonValue(mVisColor.name()));
	joc.insert("isBackground", QJsonValue(mIsBackground));

	QJsonArray ja;
	for (const QString& a : mAlias)
		ja.append(a);

	joc.insert("alias", ja);

	jo.insert(jsonKey(), joc);
}

QString LabelInfo::jsonKey() {
	return QString("Class");
}

// LabelManager --------------------------------------------------------------------
LabelManager::LabelManager() {
	add(LabelInfo::unknownLabel());
}

bool LabelManager::isEmpty() const {
	return mLookups.empty();
}

int LabelManager::size() const {
	return mLookups.size();
}

LabelManager LabelManager::read(const QString & filePath) {

	return fromJson(Utils::readJson(filePath));
}

LabelManager LabelManager::fromJson(const QJsonObject & jo) {

	LabelManager manager;

	// parse the lookups
	QJsonArray labels = jo.value(LabelManager::jsonKey()).toArray();
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
		
		if (fc.id() != LabelInfo::label_unknown) {
			QJsonObject cJo;
			fc.toJson(cJo);
			ja << cJo;
		}
	}

	jo.insert(jsonKey(), ja);
}

void LabelManager::add(const LabelInfo & label) {

	if (contains(label)) {
		
		// tell the user if it's not a default label
		if (label.id() < LabelInfo::label_end)
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

int LabelManager::indexOf(int id) const {

	// try to directly find the entry
	for (int idx = 0; idx < mLookups.size(); idx++) {

		if (mLookups[idx].id() == id)
			return idx;
	}

	return -1;
}

/// <summary>
/// Returns the first label that is marked as background label.
/// </summary>
/// <returns></returns>
LabelInfo LabelManager::backgroundLabel() const {
	
	for (const LabelInfo& li : mLookups)
		if (li.isBackground())
			return li;
	
	return LabelInfo();
}

QVector<LabelInfo> LabelManager::labelInfos() const {
	return mLookups;
}

QString LabelManager::jsonKey() {
	return "Labels";
}

void LabelManager::draw(QPainter & p) const {

	int lineSpacing = 30;
	Vector2D margin(5,5);

	QPen oldPen = p.pen();
	Rect r(10, 10, 20, 20);

	Rect bgRect(r.topLeft()-margin, Vector2D(200, lineSpacing*mLookups.size())+margin);
	p.setPen(ColorManager::white());
	p.setBrush(ColorManager::white(0.4));
	p.drawRect(bgRect.toQRect());

	for (const LabelInfo& li : mLookups) {
		
		p.setPen(li.visColor());
		p.setBrush(li.visColor());
		p.drawRect(r.toQRect());

		// draw label
		Vector2D tp = r.bottomRight();
		tp += Vector2D(10, -5);
		p.drawText(tp.toQPoint(), li.name());
		r.move(Vector2D(0, lineSpacing));
	}
	p.setPen(oldPen);
}

// PixelLabel --------------------------------------------------------------------
PixelLabel::PixelLabel(const QString & id) : BaseElement(id) {
}

void PixelLabel::setLabel(const LabelInfo & label) {
	mLabel = label;
}

LabelInfo PixelLabel::predicted() const {
	return mLabel;
}

void PixelLabel::setTrueLabel(const LabelInfo & label) {
	mTrueLabel = label;
}

LabelInfo PixelLabel::trueLabel() const {
	return mTrueLabel;
}

void PixelLabel::setVotes(const PixelVotes & votes) {
	mVotes = votes;
}

PixelVotes PixelLabel::votes() const {
	return mVotes;
}

/// <summary>
/// Checks if GT and prediction are present
/// </summary>
/// <returns>
///   <c>true</c> if trueLabel() and label() are set; otherwise, <c>false</c>.
/// </returns>
bool PixelLabel::isEvaluated() const {
	return !mTrueLabel.isNull() && !mLabel.isNull();
}

// SuperPixelModel --------------------------------------------------------------------
SuperPixelModel::SuperPixelModel(const LabelManager & labelManager, const cv::Ptr<cv::ml::StatModel>& model) {
	mModel = model;
	mManager = labelManager;
}

bool SuperPixelModel::isEmpty() const {
	return !mModel || mManager.isEmpty();
}

cv::Ptr<cv::ml::StatModel> SuperPixelModel::model() const {
	return mModel;
}

cv::Ptr<cv::ml::RTrees> SuperPixelModel::randomTrees() const {
	return mModel.dynamicCast<cv::ml::RTrees>();
}

LabelManager SuperPixelModel::manager() const {
	return mManager;
}

QVector<PixelLabel> SuperPixelModel::classify(const cv::Mat & features) const {

	Timer dt;

	cv::Mat cFeatures = features;
	IP::normalize(cFeatures);

	QVector<PixelLabel> labelInfos;

	for (int rIdx = 0; rIdx < cFeatures.rows; rIdx++) {

		// TODO: get weights
		const cv::Mat& cr = cFeatures.row(rIdx);
		cv::Mat out(1, cr.cols, cr.depth());

		cv::Mat rawVotes;
		float rawLabel = 0;

		PixelLabel pLabel;

#if CV_MAJOR_VERSION >= 3 && CV_MINOR_VERSION >= 3
		
		if (randomTrees()) {
			randomTrees()->getVotes(cr, rawVotes, cv::ml::DTrees::RAW_OUTPUT);

			// get pixel votes
			PixelVotes pv(mManager);
			pv.setRawVotes(rawVotes);

			rawLabel = (float)pv.labelIndex();
			pLabel.setVotes(pv);
		}
		else {
			rawLabel = mModel->predict(cr);
		}
#else
		rawLabel = mModel->predict(cr);
#endif

		// get label
		int labelId = qRound(rawLabel);

		LabelInfo label = mManager.find(labelId);
		assert(label.id() != LabelInfo::label_unknown);

		pLabel.setLabel(label);
		labelInfos << pLabel;
	}
	
	qInfo() << cFeatures.rows << "features predicted in" << dt;

	return labelInfos;
}

bool SuperPixelModel::write(const QString & filePath) const {

	if (mModel && !mModel->isTrained())
		qWarning() << "writing trainer that is NOT trained!";

	// write all label data
	QJsonObject jo;
	mManager.toJson(jo);

	// write RTrees classifier
	toJson(jo);

	int64 bw = Utils::writeJson(filePath, jo);

	return bw > 0;	// if we wrote more than 0 bytes, it's ok
}

void SuperPixelModel::toJson(QJsonObject& jo) const {

	if (!mModel) {
		qWarning() << "cannot save SuperPixelModel because it is NULL.";
		return;
	}

	cv::FileStorage fs(".xml", cv::FileStorage::WRITE | cv::FileStorage::MEMORY | cv::FileStorage::FORMAT_XML);
	mModel->write(fs);
#if CV_MAJOR_VERSION == 3 && CV_MINOR_VERSION == 1
	fs << "format" << 3;	// fixes bug #4402
#endif
	std::string data = fs.releaseAndGetString();

	QByteArray ba(data.c_str(), (int)data.length());
	QString ba64Str = ba.toBase64();

	jo.insert("SuperPixelModel", ba64Str);
}

QSharedPointer<SuperPixelModel> SuperPixelModel::read(const QString & filePath) {

	Timer dt;
	QSharedPointer<SuperPixelModel> sm = QSharedPointer<SuperPixelModel>::create();

	QJsonObject jo = Utils::readJson(filePath);
	sm->mManager = LabelManager::fromJson(jo);
	sm->mModel = SuperPixelModel::readRTreesModel(jo);

	if (!sm->mManager.isEmpty() && sm->mModel) {
		qInfo() << "var count:" << sm->model()->getVarCount() << "is classifier" << sm->model()->isClassifier();
		qInfo() << "SuperPixel classifier loaded from" << filePath << "in" << dt;
	}
	else {
		qCritical() << "Could not load model from" << filePath;
		return QSharedPointer<SuperPixelModel>::create();
	}

	return sm;
}

cv::Ptr<cv::ml::RTrees> SuperPixelModel::readRTreesModel(QJsonObject & jo) {

	// decode data
	QByteArray ba = jo.value("SuperPixelModel").toVariant().toByteArray();
	ba = QByteArray::fromBase64(ba);

	if (!ba.length()) {
		qCritical().noquote() << "cannot read model";
		return cv::Ptr<cv::ml::RTrees>();
	}

	// read model from memory
	cv::String dataStr(ba.data(), ba.length());
	cv::FileStorage fs(dataStr, cv::FileStorage::READ | cv::FileStorage::MEMORY | cv::FileStorage::FORMAT_XML);
	cv::FileNode root = fs.root();

	if (root.empty()) {
		qCritical().noquote() << "cannot read model from: " << ba;
		return cv::Ptr<cv::ml::RTrees>();
	}

	cv::Ptr<cv::ml::RTrees> model = cv::Algorithm::read<cv::ml::RTrees>(root);
	return model;
}

// -------------------------------------------------------------------- PixelVotes 
PixelVotes::PixelVotes(const LabelManager & lm, const QString & id) : BaseElement(id) {
	mManager = lm;
}

bool PixelVotes::isEmpty() const {
	return mVotes.empty();
}

void PixelVotes::setRawVotes(const cv::Mat & rawVotes) {

	if (rawVotes.empty())
		return;

	mVotes = cv::Mat(1, mManager.size(), CV_32FC1, cv::Scalar(0));

	const float* rvp = rawVotes.ptr<float>();
	float* vp = mVotes.ptr<float>();

	for (int rIdx = 0; rIdx < rawVotes.cols; rIdx++) {

		int cl = qRound(rvp[rIdx]);
		int labelIndex = mManager.indexOf(cl);

		if (labelIndex != -1) {

			assert(labelIndex >= 0 && labelIndex < mVotes.cols);
			vp[labelIndex] += 1;
		}
		else
			qWarning() << "unknown class index: " << cl;
	}

	mNumTrees = cv::sum(mVotes)[0];

	if (mNumTrees > 0)
		mVotes /= mNumTrees;
	else
		qWarning() << "sum votes is zero - that's weird!";
}

cv::Mat PixelVotes::data() const {
	return mVotes;
}

int PixelVotes::labelIndex() const {
	return mManager.labelInfos()[maxVote()].id();
}

int PixelVotes::maxVote() const {
	
	cv::Point pMaxIdx;
	cv::minMaxLoc(mVotes, 0, 0, 0, &pMaxIdx);
	return pMaxIdx.x;
}

}