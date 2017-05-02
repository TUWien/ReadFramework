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

#include "Image.h"
#include "Drawer.h"
#include "Shapes.h"

#pragma warning(push, 0)	// no warnings from includes
#include <QDebug>
#include <QBuffer>
#include <QImageWriter>
#include <QFileInfo>
#include <QPixmap>
#include <QPainter>
#pragma warning(pop)

namespace rdf {

// Image --------------------------------------------------------------------

/// <summary>
/// Converts a QImage to a cv::Mat.
/// </summary>
/// <param name="img">The Qimage.</param>
/// <returns>The cv__Mat image.</returns>
cv::Mat Image::qImage2Mat(const QImage& img) {

	cv::Mat mat2;
	QImage cImg;	// must be initialized here!	(otherwise the data is lost before clone())

	try {
		//if (img.format() == QImage::Format_RGB32)
		//	qDebug() << "we have an RGB32 in memory...";

		if (img.format() == QImage::Format_ARGB32 || img.format() == QImage::Format_RGB32) {
			mat2 = cv::Mat(img.height(), img.width(), CV_8UC4, (uchar*)img.bits(), img.bytesPerLine());
			//qDebug() << "ARGB32 or RGB32";
		}
		else if (img.format() == QImage::Format_RGB888) {
			mat2 = cv::Mat(img.height(), img.width(), CV_8UC3, (uchar*)img.bits(), img.bytesPerLine());
			//qDebug() << "RGB888";
		}
		//// converting to indexed8 causes bugs in the qpainter
		//// see: http://qt-project.org/doc/qt-4.8/qimage.html
		//else if (img.format() == QImage::Format_Indexed8) {
		//	mat2 = Mat(img.height(), img.width(), CV_8UC1, (uchar*)img.bits(), img.bytesPerLine());
		//	//qDebug() << "indexed...";
		//}
		else {
			cImg = img.convertToFormat(QImage::Format_ARGB32);
			mat2 = cv::Mat(cImg.height(), cImg.width(), CV_8UC4, (uchar*)cImg.bits(), cImg.bytesPerLine());
		}

		mat2 = mat2.clone();	// we need to own the pointer
	}
	catch (...) {	// something went seriously wrong (e.g. out of memory)
		qDebug() << "[DkImage::qImage2Mat] could not convert image - something is seriously wrong down here...";
	}

	return mat2; 
}

/// <summary>
/// Converts a cv::Mat to QImage.
/// </summary>
/// <param name="img">The cv::Mat img.</param>
/// <returns>The converted QImage.</returns>
QImage Image::mat2QImage(const cv::Mat& img) {

	QImage qImg;
	cv::Mat cvImg = img;

	// since Mat header is copied, a new buffer should be allocated (check this!)
	if (cvImg.depth() == CV_32F)
		cvImg.convertTo(cvImg, CV_8U, 255);

	if (cvImg.type() == CV_8UC1) {
		qImg = QImage(cvImg.data, (int)cvImg.cols, (int)cvImg.rows, (int)cvImg.step, QImage::Format_Indexed8);	// opencv uses size_t for scaling in x64 applications
	}
	if (cvImg.type() == CV_8UC3) {
		qImg = QImage(cvImg.data, (int)cvImg.cols, (int)cvImg.rows, (int)cvImg.step, QImage::Format_RGB888);
	}
	if (cvImg.type() == CV_8UC4) {
		qImg = QImage(cvImg.data, (int)cvImg.cols, (int)cvImg.rows, (int)cvImg.step, QImage::Format_ARGB32);
	}
	
	qImg = qImg.copy();

	return qImg;
}

cv::Mat Image::qPixmap2Mat(const QPixmap & img) {
	return qImage2Mat(img.toImage());
}

QPixmap Image::mat2QPixmap(const cv::Mat & img) {
	return QPixmap::fromImage(mat2QImage(img));
}

cv::Mat Image::qVector2Mat(const QVector<float>& data) {

	cv::Mat m(1, data.size(), CV_32FC1);

	float* mPtr = m.ptr<float>();
	for (int idx = 0; idx < data.size(); idx++)
		mPtr[idx] = data[idx];

	return m;
}

/// <summary>
/// Saves the specified QImage img.
/// </summary>
/// <param name="img">The img to be saved.</param>
/// <param name="savePath">The save path.</param>
/// <param name="compression">The compression.</param>
/// <returns>True if the image was saved.</returns>
bool Image::save(const QImage& img, const QString& savePath, int compression) {

	bool saved = false;

	QFileInfo fInfo(savePath);

	bool hasAlpha = alphaChannelUsed(img);
	QImage sImg = img;

	// JPEG 2000 can only handle 32 or 8bit images
	if (!hasAlpha && img.colorTable().empty() && !fInfo.suffix().contains(QRegExp("(j2k|jp2|jpf|jpx|png)")))
		sImg = sImg.convertToFormat(QImage::Format_RGB888);
	else if (fInfo.suffix().contains(QRegExp("(j2k|jp2|jpf|jpx)")) && sImg.depth() != 32 && sImg.depth() != 8)
		sImg = sImg.convertToFormat(QImage::Format_RGB32);
	else if (fInfo.suffix().contains(QRegExp("(png)")))
		sImg = sImg.convertToFormat(QImage::Format_RGB32);

	if (compression == -1 && fInfo.suffix().contains(QRegExp("(png)"))) {
		compression = 1;	// we want pngs always to be compressed
	}
	else if (compression == -1)
		compression = 90;	// choose good default compression for jpgs

	//qDebug() << "img has alpha: " << (sImg.format() != QImage::Format_RGB888) << " img uses alpha: " << hasAlpha;
	saved = sImg.save(savePath, 0, compression);

	if (!saved) qWarning() << "could not save " << savePath;

	return saved;
}

/// <summary>
/// Saves the specified cv::Mat img.
/// </summary>
/// <param name="img">The imgto be saved.</param>
/// <param name="savePath">The save path.</param>
/// <param name="compression">The compression.</param>
/// <returns>True if the image was saved.</returns>
bool Image::save(const cv::Mat& img, const QString& savePath, int compression) {

	bool saved = false;

	QImage sImg = rdf::Image::mat2QImage(img);
	saved = rdf::Image::save(sImg, savePath, compression);

	return saved;
}


/// <summary>
/// Checks if the alpha channel is used.
/// </summary>
/// <param name="img">The QImage img.</param>
/// <returns>True if the alpha channel is used.</returns>
bool Image::alphaChannelUsed(const QImage& img) {

	if (img.format() != QImage::Format_ARGB32 && img.format() != QImage::Format_ARGB32_Premultiplied)
		return false;

	// number of bytes per line used
	int bpl = (img.width() * img.depth() + 7) / 8;
	int pad = img.bytesPerLine() - bpl;
	const uchar* ptr = img.bits();

	for (int rIdx = 0; rIdx < img.height(); rIdx++) {

		for (int cIdx = 0; cIdx < bpl; cIdx++, ptr++) {

			if (cIdx % 4 == 3 && *ptr != 255)
				return true;
		}

		ptr += pad;
	}

	return false;
}

/// <summary>
/// Prints the basic image information.
/// </summary>
/// <param name="img">The source img.</param>
/// <param name="name">The name that should be displayed in the command line.</param>
void Image::imageInfo(const cv::Mat& img, const QString name) {

	qDebug().noquote() << "image info: " << name;
	QString info;

	if (img.empty()) {
		qDebug() << "<empty image>";
		return;
	}
	
	qDebug() << "   " << img.rows << "x" << img.cols << " (rows x cols)";

	qDebug() << "    channels: " << img.channels();

	int depth = img.depth();
	switch (depth) {
	case CV_8U:
		info = "CV_8U";
		break;
	case CV_32F:
		info = "CV_32F";
		break;
	case CV_32S:
		info = "CV_32S";
		break;
	case CV_64F:
		info = "CV_64F";
		break;
	default:
		info = "unknown";
		break;
	}
	qDebug().noquote() << "    depth: " << info;

	if (img.channels() == 1) {


		double min, max;
		minMaxLoc(img, &min, &max);
		qDebug().nospace() << "    dynamic range: [" << min << " " << max << "]";
	}

}

/// <summary>
/// Prints the image as a string formatted according Matlab.
/// </summary>
/// <param name="img">The src img.</param>
/// <param name="name">The variable name in Matlab.</param>
/// <returns>A string containing the values of the image</returns>
QString Image::printImage(const cv::Mat& img, const QString name) {
	
	QString imgInfo;
	if (img.depth() == CV_32FC1)
		imgInfo = rdf::Image::printMat<float>(img, name);
	else if (img.depth() == CV_64FC1)
		imgInfo = rdf::Image::printMat<double>(img, name);
	else if (img.depth() == CV_32SC1)
		imgInfo = rdf::Image::printMat<int>(img, name);
	else if (img.depth() == CV_8UC1) {
		cv::Mat tmp;
		img.convertTo(tmp, CV_32FC1);
		imgInfo = rdf::Image::printMat<float>(tmp, name);
	}
	else
		imgInfo = "I could not visualize the mat";

	return imgInfo;
}

QJsonObject Image::matToJson(const cv::Mat & img, bool compress) {

	QJsonObject jo;
	jo.insert("rows", img.rows);
	jo.insert("cols", img.cols);
	jo.insert("type", img.type());
	jo.insert("compressed", compress);

	QByteArray ba(img.ptr<const char>(), img.rows*img.cols*(int)img.elemSize());
	
	// compress the data?
	if (compress)
		ba = qCompress(ba);

	QString db64 = ba.toBase64();
	jo.insert("data", db64);

	return jo;
}

cv::Mat Image::jsonToMat(const QJsonObject & jo) {

	int rows = jo.value("rows").toInt(-1);
	int cols = jo.value("cols").toInt(-1);
	int type = jo.value("type").toInt(-1);
	bool compressed = jo.value("compressed").toBool(false);

	if (rows == -1 || cols == -1 || type == -1) {
		qWarning() << "cannot read mat from Json";
		return cv::Mat();
	}

	// decode data
	QByteArray ba = jo.value("data").toVariant().toByteArray();
	ba = QByteArray::fromBase64(ba);
	
	if (compressed)
		ba = qUncompress(ba);

	if (ba.length() != rows*cols*cv::Mat(1, 1, type).elemSize()) {
		qCritical() << "illegal buffer length when decoding cv::Mat from json";
		return cv::Mat();
	}

	cv::Mat img(rows, cols, type, ba.data());
	img = img.clone();	// then we definitely own the data

	return img;
}

// Histogram --------------------------------------------------------------------
Histogram::Histogram(const cv::Mat & values) {
	assert(values.depth() == CV_32FC1 && values.rows == 1);
	mHist = values;
}

Histogram::Histogram(const QVector<int>& values) {

	mHist = cv::Mat(1, values.size(), CV_32FC1);
	unsigned int* hPtr = mHist.ptr<unsigned int>();

	for (int idx = 0; idx < values.size(); idx++)
		hPtr[idx] = values[idx];
}

cv::Mat Histogram::draw(const QPen & pen, const QColor & bgCol) {

	QPen cp = pen;

	// make a nice default pen
	if (cp == QPen()) {
		cp = QPen(ColorManager::colors()[0]);
	}

	QPixmap pm(hist().cols, 100);
	pm.fill(bgCol);

	QPainter p(&pm);
	p.setPen(cp);
	draw(p);

	return Image::qPixmap2Mat(pm);
}

void Histogram::draw(QPainter & p) const {

	Rect r(0, 0, p.device()->width(), p.device()->height());
	draw(p, r);
}

void Histogram::draw(QPainter & p, const Rect& r) const {

	if (mHist.empty())
		return;

	QBrush oB = p.brush();
	QPen oP = p.pen();
	p.setBrush(oP.color());
	p.setPen(Qt::NoPen);

	double maxVal = max();
	double minVal = min();
	double binWidth = r.width() / mHist.cols;
	double gap = (binWidth >= 2) ? 1.0 : 0.0;

	const float* hPtr = mHist.ptr<float>();
	for (int idx = 0; idx < mHist.cols; idx++) {

		double x = transformX(idx, r);
		QPointF p1(x, transformY((double)hPtr[idx], minVal, maxVal, r));
		QPointF p2(x + binWidth-gap, transformY(0, minVal, maxVal, r));

		QRectF cr(p1, p2);
		p.drawRect(cr);
	}
	
	// draw 0 line
	double tz = transformY(0, minVal, maxVal, r);
	QLineF zeroLine(QPointF(r.left(), tz), QPointF(r.right(), tz));

	p.setPen(oP.color().darker());
	p.setOpacity(0.5);
	p.drawLine(zeroLine);
	p.setOpacity(1.0);

	// reset painter
	p.setPen(oP);
	p.setBrush(oB);
}

double Histogram::transformY(double val, double minV, double maxV, const Rect & r) const {
	
	double tVal = r.bottom() - qRound((double)(val - minV) / (maxV - minV) * r.height());
	
	return tVal;
}

double Histogram::transformX(double val, const Rect & r) const {
	
	double binWidth = r.width() / mHist.cols;
	double tVal = val*binWidth + r.left();

	return tVal;
}

cv::Mat Histogram::hist() const {
	return mHist;
}

double Histogram::max() const {
	double mVal = 0;
	cv::minMaxLoc(mHist, 0, &mVal);
	return mVal;
}

double Histogram::min() const {
	double mVal = 0;
	cv::minMaxLoc(mHist, &mVal);
	return mVal;
}


}