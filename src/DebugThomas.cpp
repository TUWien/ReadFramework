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
	testXml();
	testLayout();
}

void ThomasTest::testXml() {
	QString xmlPath = rdf::PageXmlParser::imagePathToXmlPath(mConfig.imagePath());
	rdf::PageXmlParser parser;
	bool success = parser.read(xmlPath);
	if (!success) {
		qDebug() << "failed to read xml file";
		return;
	}
	auto pageElement = parser.page();

	//auto& utils = rdf::Utils::instance();	// diem: warning c4189
	QString xmlPathOut = rdf::Utils::instance().createFilePath(xmlPath, "-out");
	parser.write(xmlPathOut, pageElement);
}

void ThomasTest::testLayout() {
	QImage imgQt(mConfig.imagePath());
	cv::Mat img = Image::qImage2Mat(imgQt);

	Timer dt;
	QString loadXmlPath = rdf::PageXmlParser::imagePathToXmlPath(mConfig.imagePath());

	rdf::PageXmlParser parser;
	parser.read(loadXmlPath);
	auto pageElement = parser.page();

	// super pixels
	
	rdf::SuperPixel sp(img);
	if (!sp.compute()) {
		qDebug() << "error during SuperPixel computation";
	}

	cv::Mat imgOut = sp.drawMserBlobs(img);
	QString imgPath = rdf::Utils::createFilePath(mConfig.outputPath(), "-sp");
	rdf::Image::save(imgOut, imgPath);
	qDebug() << "sp debug image added" << imgPath;

	// collect features

	rdf::LabelManager lm = rdf::LabelManager::read(mConfig.labelConfigPath());
	qInfo().noquote() << lm.toString();

	// feed the label lookup
	rdf::SuperPixelLabeler spl(sp.getMserBlobs(), rdf::Rect(img));
	spl.setLabelManager(lm);
	spl.setFilePath(mConfig.imagePath());	// parse filepath for gt

	// set the ground truth
	if (pageElement) {
		spl.setPage(pageElement);
		spl.setRootRegion(pageElement->rootRegion());
	}

	if (!spl.compute())
		qCritical() << "could not compute SuperPixel labeling!";

	imgOut = img.clone();
	imgOut = spl.draw(imgOut);
	imgPath = rdf::Utils::createFilePath(mConfig.outputPath(), "-spl");
	rdf::Image::save(imgOut, imgPath);
	qDebug() << "spl debug image added " << imgPath;

	// compute features

	rdf::SuperPixelFeature spf(img, spl.set());
	if (!spf.compute())
		qCritical() << "could not compute SuperPixel features!";

	rdf::FeatureCollectionManager fcm(spf.features(), spf.set());
	fcm.write(mConfig.featureCachePath());
	
	// read it back (test)
	fcm.read(mConfig.featureCachePath());

	// train

	SuperPixelTrainer spt(fcm);
	if (!spt.compute())
		qCritical() << "could not train data...";

	spt.write(mConfig.classifierPath());

	qInfo() << "classifierPath: " << mConfig.classifierPath();
	// test - read back the model
	auto model = rdf::SuperPixelModel::read(mConfig.classifierPath());

	auto f = model->model();
	if (f && f->isTrained()) {
		qDebug() << "the classifier I loaded is trained...";
	}

	// classify

	SuperPixelClassifier spc(img, sp.pixelSet());
	spc.setModel(spt.model());

	if (!spc.compute())
		qWarning() << "could not classify SuperPixels";

	imgOut = img.clone();
	imgOut = spc.draw(imgOut);
	imgPath = rdf::Utils::createFilePath(mConfig.outputPath(), "-spc");
	rdf::Image::save(imgOut, imgPath);
	qDebug() << "spc (classified features) debug image added " << imgPath;
}

}
