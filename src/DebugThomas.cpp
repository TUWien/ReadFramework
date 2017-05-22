#include "DebugThomas.h"
#include "PageParser.h"
#include "Utils.h"
#include "Image.h"
#include "Binarization.h"
#include "LineTrace.h"
#include "Elements.h"
#include "ElementsHelper.h"
#include "Settings.h"

#include "SuperPixel.h"
#include "TabStopAnalysis.h"
#include "TextLineSegmentation.h"
#include "PageSegmentation.h"
#include "SuperPixelClassification.h"
#include "SuperPixelTrainer.h"
#include "LayoutAnalysis.h"

#pragma warning(push, 0)	// no warnings from includes
#include <QDebug>
#include <QImage>
#include <QFileInfo>
#include <QProcess>

#include <QJsonObject>

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/ml/ml.hpp>
#pragma warning(pop)

namespace rdf {

ThomasTest::ThomasTest(const DebugConfig& config)
	: mConfig(config) {
}

void ThomasTest::test() {
	//testXml();
	testFeatureCollection();
	testTraining();
	//testClassification();
}

void ThomasTest::testXml() {
	QString xmlPath = PageXmlParser::imagePathToXmlPath(mConfig.imagePath());
	PageXmlParser parser;

	bool success = parser.read(xmlPath);
	if (!success) {
		qDebug() << "failed to read xml file";
		return;
	}
	auto pageElement = parser.page();

	//auto& utils = Utils::instance();	// diem: warning c4189
	QString xmlPathOut = Utils::createFilePath(xmlPath, "-out");
	parser.write(xmlPathOut, pageElement);
}

void ThomasTest::testFeatureCollection() {
	QImage imgQt(mConfig.imagePath());
	cv::Mat img = Image::qImage2Mat(imgQt);

	Timer dt;
	QString loadXmlPath = PageXmlParser::imagePathToXmlPath(mConfig.imagePath());

	PageXmlParser parser;
	parser.read(loadXmlPath, true);
	auto pageElement = parser.page();

	qDebug() << "layers: ";
	for (const auto& layer : pageElement->layers()) {
		qDebug() << "zIndex = " << layer->zIndex() << ", size = " << layer->regions().size();
	}

	// define layers
	QVector<Region::Type> layerTypeAssignment{
		Region::type_text_region,
		Region::type_image,
		Region::type_graphic,
		Region::type_chart,
		Region::type_table_region
	};
	pageElement->redefineLayersByType(layerTypeAssignment);

	qDebug() << "layers: ";
	for (const auto& layer : pageElement->layers()) {
		qDebug() << "zIndex = " << layer->zIndex() << ", size = " << layer->regions().size();
	}

	// super pixels

	SuperPixel sp(img);
	if (!sp.compute()) {
		qDebug() << "error during SuperPixel computation";
	}

	cv::Mat imgOut = sp.drawMserBlobs(img);
	QString imgPath = Utils::createFilePath(mConfig.outputPath(), "-sp");
	Image::save(imgOut, imgPath);
	qDebug() << "sp debug image added" << imgPath;

	// collect features

	LabelManager lm = LabelManager::read(mConfig.labelConfigPath());
	qInfo().noquote() << lm.toString();

	// feed the label lookup
	SuperPixelLabeler spl(sp.getMserBlobs(), Rect(img));
	spl.setLabelManager(lm);
	spl.setFilePath(mConfig.imagePath());	// parse filepath for gt

	if (pageElement) {
		spl.setPage(pageElement);
		spl.setRootRegion(pageElement->rootRegion());
	}

	if (!spl.compute())
		qCritical() << "could not compute SuperPixel labeling!";

	imgOut = img.clone();
	imgOut = spl.draw(imgOut);
	imgPath = Utils::createFilePath(mConfig.outputPath(), "-spl");
	Image::save(imgOut, imgPath);
	qDebug() << "spl debug image added " << imgPath;

	// compute features

	SuperPixelFeature spf(img, spl.set());
	if (!spf.compute())
		qCritical() << "could not compute SuperPixel features!";

	FeatureCollectionManager fcm(spf.features(), spf.set());
	fcm.write(mConfig.featureCachePath());

	// read it back (test)
	//fcm.read(mConfig.featureCachePath());
}

void ThomasTest::testTraining() {
	auto fcm = FeatureCollectionManager::read(mConfig.featureCachePath());

	SuperPixelTrainer spt(fcm);
	if (!spt.compute())
		qCritical() << "could not train data...";

	spt.write(mConfig.classifierPath());

	qInfo() << "classifierPath: " << mConfig.classifierPath();
	// test - read back the model
	auto model = SuperPixelModel::read(mConfig.classifierPath());

	auto f = model->model();
	if (f && f->isTrained()) {
		qDebug() << "the classifier I loaded is trained...";
	}
}

void ThomasTest::testClassification() {
	QImage imgQt(mConfig.imagePath());
	cv::Mat img = Image::qImage2Mat(imgQt);

	SuperPixel sp(img);
	if (!sp.compute()) {
		qDebug() << "error during SuperPixel computation";
	}

	auto model = SuperPixelModel::read(mConfig.classifierPath());

	auto f = model->model();
	if (f && f->isTrained()) {
		qDebug() << "the classifier I loaded is trained...";
	}

	// classify

	SuperPixelClassifier spc(img, sp.pixelSet());
	spc.setModel(model);

	if (!spc.compute())
		qWarning() << "could not classify SuperPixels";

	cv::Mat imgOut = img.clone();
	imgOut = spc.draw(imgOut);
	QString imgPath = Utils::createFilePath(mConfig.outputPath(), "-spc");
	Image::save(imgOut, imgPath);
	qDebug() << "spc (classified features) debug image added " << imgPath;
}

}
