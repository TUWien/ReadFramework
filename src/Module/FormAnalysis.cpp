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

#include "FormAnalysis.h"
#include "Binarization.h"
#include "Algorithms.h"
#include "SkewEstimation.h"
#include "Image.h"
#include "PageParser.h"
#include "Elements.h"


#pragma warning(push, 0)	// no warnings from includes
#include <QDebug>
#include <QSettings>
#include <QFileInfo>
#include <QDir>
#include "opencv2/imgproc/imgproc.hpp"
#pragma warning(pop)

namespace rdf {
	FormFeatures::FormFeatures(){
		mConfig = QSharedPointer<FormFeaturesConfig>::create();
	}
	FormFeatures::FormFeatures(const cv::Mat & img, const cv::Mat & mask)
	{
		mSrcImg = img;
		mMask = mask;

		mSizeSrc = mSrcImg.size();

		mConfig = QSharedPointer<FormFeaturesConfig>::create();
	}

	bool FormFeatures::loadTemplateDatabase(QString db)	{

		QString dbDir = db.isEmpty() ? config()->templDatabase() : db;

		mTemplates.clear();
		
		////QFileInfo fileInfo(dbDir);
		QDir dir = QDir(db);

		if (!dir.exists()) {
			qWarning() << "I cannot load the forms from" << dir.absolutePath() << "since it is not existing...";
			return false;
		}

		// only load files which have the same basename as the nmf with an index
		// e.g.: nmf.yml -> nmf-01.yml
		QRegExp filePattern("*.xml");
		QStringList files = dir.entryList();
		qSort(files.begin(), files.end());

		if (files.empty()) {
			qWarning() << "sorry, I could not load form templates from: " << dir.absolutePath();
			return false;
		}

		for (const QString& fp : files) {

			if (filePattern.exactMatch(fp)) {
				//load templates
				rdf::PageXmlParser parser;
				parser.read(QFileInfo(dir, fp).absoluteFilePath());
				auto pe = parser.page();

				//read xml separators and store them to testinfo
				QVector<rdf::Line> hLines;
				QVector<rdf::Line> vLines;

				QVector<QSharedPointer<rdf::Region>> test = rdf::Region::allRegions(pe->rootRegion());// pe->rootRegion()->children();
				for (auto i : test) {
					if (i->type() == i->type_separator) {
						rdf::SeparatorRegion* tSep = dynamic_cast<rdf::SeparatorRegion*>(i.data());
						if (tSep) {
							if (tSep->line().isHorizontal(5.0))
								hLines.push_back(tSep->line());

							if (tSep->line().isVertical(5.0))
								vLines.push_back(tSep->line());
						}
					}
				}
				FormFeatures templ;
				templ.setVerLines(vLines);
				templ.setHorLines(hLines);
				templ.setFormName(fp);
				mTemplates.push_back(templ);
			}
		}

		if (mTemplates.empty()) {
			qWarning() << "Sorry, I could not load the form templates from " << dir.absolutePath();
			return false;
		}

		return true;
	}

	cv::Mat FormFeatures::getMatchedLineImg(const cv::Mat srcImg, cv::Point offset) const	{

		cv::Mat finalImg = srcImg.clone();

		QVector<rdf::Line> hl, vl;

		for (auto h : mHorLinesMatched) {
			hl.push_back(rdf::Line(h.startPointCV() + offset, h.endPointCV() + offset, h.thickness()));
		}
		for (auto v : mVerLinesMatched) {
			vl.push_back(rdf::Line(v.startPointCV() + offset, v.endPointCV() + offset, v.thickness()));
		}

		rdf::LineTrace::generateLineImage(hl, vl, finalImg, cv::Scalar(0,255,0), cv::Scalar(0,0,255));

		return finalImg;
	}
	void FormFeatures::setInputImg(const cv::Mat & img)
	{
		mSrcImg = img;
	}
	void FormFeatures::setMask(const cv::Mat & mask)
	{
		mMask = mask;
	}
	bool FormFeatures::isEmpty() const
	{
		return mSrcImg.empty();
	}
	bool FormFeatures::compute()
	{
		if (!checkInput())
			return false;

		if (mBwImg.empty()) {
			if (!computeBinaryInput()) {
				mWarning << "binary image was not set and could not be calculated";
				return false;
			}

			qDebug() << "binary image was not set - was calculated";
		}

		if (mEstimateSkew) {
			BaseSkewEstimation skewE(mSrcImg, mMask);
			skewE.compute();
			mPageAngle = skewE.getAngle();
		}

		//compute Lines
		LineTrace lt(mBwImg, mMask);
		if (mEstimateSkew) {
			lt.setAngle(mPageAngle);
		} else {
			lt.setAngle(0);
		}
		lt.compute();

		mHorLines = lt.getHLines();
		mVerLines = lt.getVLines();

		return true;
	}

	bool FormFeatures::computeBinaryInput()
	{
		if (mSrcImg.empty() || mSrcImg.depth() != CV_8U) {
			mWarning << "image is empty or illegal image depth: " << mSrcImg.depth();
			return false;
		}

		if (!mMask.empty() && mMask.depth() != CV_8U && mMask.channels() != 1) {
			mWarning << "illegal image depth or channel for mask: " << mMask.depth();
			return false;
		}

		BinarizationSuAdapted binarizeImg(mSrcImg, mMask);
		binarizeImg.compute();
		mBwImg = binarizeImg.binaryImage();
		if (mPreFilter)
			mBwImg = Algorithms::instance().preFilterArea(mBwImg, preFilterArea);

		return true;
	}

	bool FormFeatures::compareWithTemplate(const FormFeatures & fTempl)	{

		std::sort(mHorLines.begin(), mHorLines.end(), rdf::Line::lessY1);
		std::sort(mVerLines.begin(), mVerLines.end(), rdf::Line::lessX1);
		

		QVector<rdf::Line> horLinesTemp = fTempl.horLines();
		QVector<rdf::Line> verLinesTemp = fTempl.verLines();

		std::sort(horLinesTemp.begin(), horLinesTemp.end(), rdf::Line::lessY1);
		std::sort(verLinesTemp.begin(), verLinesTemp.end(), rdf::Line::lessX1);

		cv::Mat lineTempl(mSizeSrc, CV_8UC1);
		lineTempl = 0;
		LineTrace::generateLineImage(horLinesTemp, verLinesTemp, lineTempl);
		lineTempl = 255 - lineTempl;
		cv::Mat distImg;
		cv::distanceTransform(lineTempl, distImg, CV_DIST_L1, CV_DIST_MASK_3, CV_32FC1); //cityblock
		//cv::distanceTransform(lineTempl, distImg, CV_DIST_L2, 3); //euclidean

		//rdf::Image::instance().imageInfo(distImg, "distImg");
		//rdf::Image::instance().save(lineTempl, "D:\\tmp\\linetmpl.png");
		//lineTempl = 0;
		//LineTrace::generateLineImage(mHorLines, mVerLines, lineTempl);
		//rdf::Image::instance().save(lineTempl, "D:\\tmp\\lineImg.png");
		//normalize(distImg, distImg, 0.0, 1.0, cv::NORM_MINMAX);
		//rdf::Image::instance().save(distImg, "D:\\tmp\\distImg.png");

		float hLen = 0, hLenTemp = 0;
		float vLen = 0, vLenTemp = 0;
		
		//calculate entire horizontal and vertical line lengths of the detected lines
		//of the current document
		for (auto i : mHorLines) {
			hLen += i.length();
		}
		for (auto i : mVerLines) {
			vLen += i.length();
		}
		//calculate entire horizontal and vertical line lengths of the template
		for (auto i : horLinesTemp) {
			hLenTemp += i.length();
		}
		for (auto i : verLinesTemp) {
			vLenTemp += i.length();
		}

		float ratioHor = hLen < hLenTemp ? hLen / hLenTemp : hLenTemp / hLen;
		float ratioVer = vLen < vLenTemp ? vLen / vLenTemp : vLenTemp / vLen;

		//at least mThreshLineLenRatio (default: 60%) of the lines must be detected in the current document
		if (ratioHor < config()->threshLineLenRation() || ratioVer < config()->threshLineLenRation()) {
			qDebug() << "form rejected: less lines as specified in the threshold (threshLineLenRatio)";
			return false;
		}

		QVector<int> offsetsX;
		QVector<int> offsetsY;
		//mSrcImg.rows
		int sizeDiffY = std::abs(mSizeSrc.height - fTempl.sizeImg().height) + config()->searchYOffset();
		int sizeDiffX = std::abs(mSizeSrc.width - fTempl.sizeImg().width) + config()->searchXOffset();

		findOffsets(horLinesTemp, verLinesTemp, offsetsX, offsetsY);

		float horizontalError = 0;
		float verticalError = 0;
		float finalErrorH = 0;
		float finalErrorV = 0;
		cv::Point offSet(0, 0);
		float acceptedHor = 0;
		float acceptedVer = 0;
		float finalAcceptedHor = 0;
		float finalAcceptedVer = 0;
		float minError = std::numeric_limits<float>::max();
		QVector<rdf::Line> horLinesMatched, verLinesMatched;

		for (int iX = 0; iX < offsetsX.size(); iX++) {
			for (int iY = 0; iY < offsetsY.size(); iY++) {
				//for for maximal translation
				if (std::abs(offsetsX[iX]) <= sizeDiffX && std::abs(offsetsY[iY]) <= sizeDiffY) {
					
					//int ox = offsetsX[iX];
					//int oy = offsetsY[iY];
					//qDebug() << ox << " " << oy;

					for (int i = 0; i < mHorLines.size(); i++) {
						float tmp = errLine(distImg, mHorLines[i], cv::Point(offsetsX[iX], offsetsY[iY]));
						//accept line or not depending on the distance, otherwise assumed as noise
						if (tmp < config()->distThreshold()*mHorLines[i].length()) {
							horizontalError += tmp < std::numeric_limits<float>::max() ? tmp : 0;
							acceptedHor += mHorLines[i].length();
							horLinesMatched.push_back(mHorLines[i]);
						}
					}
					for (int i = 0; i < mVerLines.size(); i++) {
						float tmp = errLine(distImg, mVerLines[i], cv::Point(offsetsX[iX], offsetsY[iY]));
						//accept line or not depending on the distance, otherwise assumed as noise
						if (tmp < config()->distThreshold()*mVerLines[i].length()) {
							verticalError += tmp < std::numeric_limits<float>::max() ? tmp : 0;
							acceptedVer += mVerLines[i].length();
							verLinesMatched.push_back(mVerLines[i]);
						}
					}
					float error = horizontalError + verticalError;
					if (error <= minError) {
						//check also if at least threshLineLenRatio (default: 60%) of the lines are detected and matched with the template image
						if (acceptedHor / hLenTemp > config()->threshLineLenRation() && acceptedVer / vLenTemp > config()->threshLineLenRation()) {
							minError = error;
							finalAcceptedHor = acceptedHor;
							finalAcceptedVer = acceptedVer;
							finalErrorH = horizontalError;
							finalErrorV = verticalError;
							offSet.x = offsetsX[iX];
							offSet.y = offsetsY[iY];
							mHorLinesMatched = horLinesMatched;
							mVerLinesMatched = verLinesMatched;
						}
					}

					horizontalError = 0;
					verticalError = 0;
					acceptedHor = 0;
					acceptedVer = 0;
					horLinesMatched.clear();
					verLinesMatched.clear();
				}

			}
		}

		qDebug() << "current Error: " << minError;
		qDebug() << "current offSet: " << offSet.x << " " << offSet.y;

		if (minError < std::numeric_limits<float>::max()) {
			//at least threshLineLenRatio (default: 60%) of the lines are detected and matched with the template image

			//check if the average distance of the matched lines is smaller then the errorThr (default: 15px)
			if (minError/(finalAcceptedHor+finalAcceptedVer) < config()->errorThr()) {
				mOffset = offSet;
				return true;
			}
		}

		return false;
	}

	cv::Size FormFeatures::sizeImg() const
	{
		return mSizeSrc;
	}

	void FormFeatures::setSize(cv::Size s)	{
		mSizeSrc = s;
	}

	QVector<rdf::Line> FormFeatures::horLines() const
	{
		return mHorLines;
	}

	void FormFeatures::setHorLines(const QVector<rdf::Line>& h)	{
		mHorLines = h;
	}

	QVector<rdf::Line> FormFeatures::horLinesMatched() const
	{
		return mHorLinesMatched;
	}

	QVector<rdf::Line> FormFeatures::verLines() const
	{
		return mVerLines;
	}

	void FormFeatures::setVerLines(const QVector<rdf::Line>& v)	{
		mVerLines = v;
	}

	QVector<rdf::Line> FormFeatures::verLinesMatched() const	{
		return mVerLinesMatched;
	}

	cv::Point FormFeatures::offset() const	{
		return mOffset;
	}

	QSharedPointer<FormFeaturesConfig> FormFeatures::config() const	{
		return qSharedPointerDynamicCast<FormFeaturesConfig>(mConfig);
	}

	cv::Mat FormFeatures::binaryImage() const	{
		return mBwImg;
	}

	void FormFeatures::setEstimateSkew(bool s)	{
		mEstimateSkew = s;
	}

	//void FormFeatures::setThreshLineLenRatio(float l)
	//{
	//	mThreshLineLenRatio = l;
	//}

	QString FormFeatures::toString() const
	{
		return QString("Form Features class calculates line and layout features for form classification");
	}

	void FormFeatures::setFormName(QString s) 	{
		mFormName = s;
	}

	QString FormFeatures::formName() const 	{
		return mFormName;
	}

	float FormFeatures::errLine(const cv::Mat & distImg, const rdf::Line l, cv::Point offset)
	{

		cv::LineIterator it(mSrcImg, l.startPointCV(), l.endPointCV());
		float distance = 0;
		float outsidePixel = 0;
		float max = 0;

		for (int i = 0; i < it.count; i++, ++it) {

			cv::Point pos = it.pos();
			pos = pos + offset;

			if (pos.x < 0 || pos.y < 0 || pos.x >= distImg.cols || pos.y >= distImg.rows) {
				//we are outside the image
				outsidePixel++;
			}
			else {
				float dist = distImg.at<float>(pos.y, pos.x);
				distance += dist;
				max = dist > max ? dist : max;
			}
		}

		distance += (max*outsidePixel);
		if (distance == 0)
			distance = std::numeric_limits<float>::max();

		return distance;
	}

	void FormFeatures::findOffsets(const QVector<Line>& hT, const QVector<Line>& vT, QVector<int>& offX, QVector<int>& offY) const	{

		offY.clear();
		offY.push_back(0);
		offX.clear();
		offX.push_back(0);

		//find vertical offsets
		if (!hT.empty() && !mHorLines.empty()) {
			//use Y difference of horizontal lines if template and current Image contains horizontal lines
			for (int i = 0; i < hT.size(); i++) {
				int yLineTemp = hT[i].startPointCV().y;
				for (int j = 0; j < mHorLines.size(); j++) {
					int diffYLine = yLineTemp - mHorLines[j].startPointCV().y;
					offY.push_back(diffYLine);
				}
			}
		}
		else if (!vT.empty() && !mVerLines.empty()) {
			//use Y difference of starting point of vertical lines if template or current image contains no horizontal lines
			for (int i = 0; i < vT.size(); i++) {
				int yLineTemp = vT[i].startPointCV().y;
				for (int j = 0; j < mVerLines.size(); j++) {
					int diffYLine = yLineTemp - mVerLines[j].startPointCV().y;
					offY.push_back(diffYLine);
				}
			}
		}
		


		//find horizontal offsets
		if (!vT.empty() && !mVerLines.empty()) {
			//use X difference of vertical lines if template and current Image contains vertical lines
			for (int i = 0; i < vT.size(); i++) {
				int xLineTemp = vT[i].startPointCV().x;
				for (int j = 0; j < mVerLines.size(); j++) {
					int diffXLine = xLineTemp - mVerLines[j].startPointCV().x;
					offX.push_back(diffXLine);
				}
			}
		}
		else if (!hT.empty() && !mHorLines.empty()) {
			//use Y difference of starting point of vertical lines if template or current image contains no horizontal lines
			for (int i = 0; i < hT.size(); i++) {
				int xLineTemp = hT[i].startPointCV().x;
				for (int j = 0; j < mHorLines.size(); j++) {
					int diffXLine = xLineTemp - mHorLines[j].startPointCV().x;
					offX.push_back(diffXLine);
				}
			}
		}

	}

	bool FormFeatures::checkInput() const	{
		if (mSrcImg.empty())
			return false;

		if (mSrcImg.depth() != CV_8U) {
			mSrcImg.convertTo(mSrcImg, CV_8U, 255);
			mWarning << "Input image was not CV_8U - has been converted";
		}

		if (mSrcImg.channels() != 1) {
			cv::cvtColor(mSrcImg, mSrcImg, CV_RGB2GRAY);

			mWarning << "Input image was a color image - has been converted tpo grayscale";
		}

		return true;
	}
	FormFeaturesConfig::FormFeaturesConfig() {
		mModuleName = "FormFeatures";
	}
	float FormFeaturesConfig::threshLineLenRation() const	{
		return mThreshLineLenRatio;
	}
	void FormFeaturesConfig::setThreshLineLenRation(float s)	{
		mThreshLineLenRatio = s;
	}

	float FormFeaturesConfig::distThreshold() const	{
		return mDistThreshold;
	}

	void FormFeaturesConfig::setDistThreshold(float d)	{
		mDistThreshold = d;
	}

	float FormFeaturesConfig::errorThr() const	{
		return mErrorThr;
	}

	void FormFeaturesConfig::setErrorThr(float e)	{
		mErrorThr = e;
	}

	int FormFeaturesConfig::searchXOffset() const	{
		return mSearchXOffset;
	}

	int FormFeaturesConfig::searchYOffset() const	{
		return mSearchYOffset;
	}

	QString FormFeaturesConfig::templDatabase() const {
		return mTemplDatabase;
	}

	void FormFeaturesConfig::setTemplDatabase(QString s) {
		mTemplDatabase = s;
	}

	QString FormFeaturesConfig::toString() const	{
		QString msg;
		msg += "  mThreshLineLenRatio: " + QString::number(mThreshLineLenRatio);
		msg += "  mDistThreshold: " + QString::number(mDistThreshold);
		msg += "  mErrorThr: " + QString::number(mErrorThr);

		return msg;
	}
	void FormFeaturesConfig::load(const QSettings & settings)	{
		mThreshLineLenRatio = settings.value("threshLineLenRatio", mThreshLineLenRatio).toFloat();
		mDistThreshold = settings.value("distThreshold", mDistThreshold).toFloat();
		mErrorThr = settings.value("errorThr", mErrorThr).toFloat();
		mTemplDatabase = settings.value("templDatabase", mTemplDatabase).toString();
	}
	void FormFeaturesConfig::save(QSettings & settings) const	{
		settings.setValue("threshLineLenRatio", mThreshLineLenRatio);
		settings.setValue("distThreshold", mDistThreshold);
		settings.setValue("errorThr", mErrorThr);
		settings.setValue("templDatabase", mTemplDatabase);
	}
}