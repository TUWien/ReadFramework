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
#include "ImageProcessor.h"


#pragma warning(push, 0)	// no warnings from includes
#include <QDebug>
#include <QSettings>
#include <QFileInfo>
#include <QDir>
#include <opencv2/imgproc.hpp>
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
		mConfig->loadSettings();
	}

	//bool FormFeatures::loadTemplateDatabase(QString db)	{

	//	QString dbDir = db.isEmpty() ? config()->templDatabase() : db;
	//	mTemplates.clear();
	//	
	//	////QFileInfo fileInfo(dbDir);
	//	QDir dir = QDir(db);

	//	if (!dir.exists()) {
	//		qWarning() << "I cannot load the forms from" << dir.absolutePath() << "since it is not existing...";
	//		return false;
	//	}

	//	// only load files which have the same basename as the nmf with an index
	//	// e.g.: nmf.yml -> nmf-01.yml
	//	QRegExp filePattern("*.xml");
	//	filePattern.setPatternSyntax(QRegExp::Wildcard);
	//	QStringList files = dir.entryList();
	//	qSort(files.begin(), files.end());

	//	if (files.empty()) {
	//		qWarning() << "sorry, I could not load form templates from: " << dir.absolutePath();
	//		return false;
	//	}

	//	for (const QString& fp : files) {

	//		if (filePattern.exactMatch(fp)) {
	//			//load templates
	//			rdf::PageXmlParser parser;
	//			parser.read(QFileInfo(dir, fp).absoluteFilePath());
	//			auto pe = parser.page();

	//			//read xml separators and store them to testinfo
	//			QVector<rdf::Line> hLines;
	//			QVector<rdf::Line> vLines;

	//			QVector<QSharedPointer<rdf::Region>> test = rdf::Region::allRegions(pe->rootRegion());// pe->rootRegion()->children();
	//			for (auto i : test) {
	//				if (i->type() == i->type_separator) {
	//					rdf::SeparatorRegion* tSep = dynamic_cast<rdf::SeparatorRegion*>(i.data());
	//					if (tSep) {
	//						if (tSep->line().isHorizontal(5.0))
	//							hLines.push_back(tSep->line());

	//						if (tSep->line().isVertical(5.0))
	//							vLines.push_back(tSep->line());
	//					}
	//				}
	//			}

	//			FormFeatures templ;
	//			templ.setVerLines(vLines);
	//			templ.setHorLines(hLines);
	//			templ.setFormName(fp);
	//			templ.setSize(cv::Size(pe->imageSize().width(), pe->imageSize().height()));
	//			mTemplates.push_back(templ);
	//		}
	//	}

	//	if (mTemplates.empty()) {
	//		qWarning() << "Sorry, I could not load the form templates from " << dir.absolutePath();
	//		return false;
	//	}

	//	return true;
	//}

	//QVector<rdf::FormFeatures> FormFeatures::templatesDb() const {
	//	return mTemplates;
	//}

	//cv::Mat FormFeatures::getMatchedLineImg(const cv::Mat& srcImg, const Vector2D& offset) const {

	//	cv::Mat finalImg = srcImg.clone();

	//	QVector<rdf::Line> hl, vl;

	//	for (auto h : mHorLinesMatched) {
	//		hl.push_back(rdf::Line(h.p1() + offset, h.p2() + offset, h.thickness()));
	//	}
	//	for (auto v : mVerLinesMatched) {
	//		vl.push_back(rdf::Line(v.p1() + offset, v.p2() + offset, v.thickness()));
	//	}

	//	rdf::LineTrace::generateLineImage(hl, vl, finalImg, cv::Scalar(0,255,0), cv::Scalar(0,0,255));

	//	return finalImg;
	//}

	void FormFeatures::setInputImg(const cv::Mat & img) {
		mSrcImg = img;
	}

	void FormFeatures::setMask(const cv::Mat & mask) {
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
		mBwImg = lt.lineImage();

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
			mBwImg = IP::preFilterArea(mBwImg, preFilterArea);

		return true;
	}

	//bool FormFeatures::compareWithTemplate(const FormFeatures & fTempl)	{

	//	//if empty, create it.. can happen if the only xmls are used to set the line information
	//	//function errLine needs it for 'tracing'
	//	if (mSrcImg.empty()) {
	//		mSrcImg = cv::Mat(mSizeSrc, CV_8UC1);
	//		mSrcImg.setTo(0);
	//	}

	//	std::sort(mHorLines.begin(), mHorLines.end(), rdf::Line::lessY1);
	//	std::sort(mVerLines.begin(), mVerLines.end(), rdf::Line::lessX1);
	//	

	//	QVector<rdf::Line> horLinesTemp = fTempl.horLines();
	//	QVector<rdf::Line> verLinesTemp = fTempl.verLines();

	//	std::sort(horLinesTemp.begin(), horLinesTemp.end(), rdf::Line::lessY1);
	//	std::sort(verLinesTemp.begin(), verLinesTemp.end(), rdf::Line::lessX1);

	//	cv::Mat lineTempl(mSizeSrc, CV_8UC1);
	//	lineTempl = 0;
	//	LineTrace::generateLineImage(horLinesTemp, verLinesTemp, lineTempl);
	//	lineTempl = 255 - lineTempl;
	//	cv::Mat distImg;
	//	cv::distanceTransform(lineTempl, distImg, CV_DIST_L1, CV_DIST_MASK_3, CV_32FC1); //cityblock
	//	//cv::distanceTransform(lineTempl, distImg, CV_DIST_L2, 3); //euclidean

	//	//rdf::Image::imageInfo(distImg, "distImg");
	//	//rdf::Image::save(lineTempl, "D:\\tmp\\linetmpl.png");
	//	//lineTempl = 0;
	//	//LineTrace::generateLineImage(mHorLines, mVerLines, lineTempl);
	//	//rdf::Image::save(lineTempl, "D:\\tmp\\lineImg.png");
	//	//normalize(distImg, distImg, 0.0, 1.0, cv::NORM_MINMAX);
	//	//rdf::Image::save(distImg, "D:\\tmp\\distImg.png");

	//	double hLen = 0, hLenTemp = 0;
	//	double vLen = 0, vLenTemp = 0;
	//	
	//	//calculate entire horizontal and vertical line lengths of the detected lines
	//	//of the current document
	//	for (auto i : mHorLines) {
	//		hLen += i.length();
	//	}
	//	for (auto i : mVerLines) {
	//		vLen += i.length();
	//	}
	//	//calculate entire horizontal and vertical line lengths of the template
	//	for (auto i : horLinesTemp) {
	//		hLenTemp += i.length();
	//	}
	//	for (auto i : verLinesTemp) {
	//		vLenTemp += i.length();
	//	}

	//	double ratioHor = hLen < hLenTemp ? hLen / hLenTemp : hLenTemp / hLen;
	//	double ratioVer = vLen < vLenTemp ? vLen / vLenTemp : vLenTemp / vLen;

	//	//at least mThreshLineLenRatio (default: 60%) of the lines must be detected in the current document
	//	if (ratioHor < config()->threshLineLenRation() || ratioVer < config()->threshLineLenRation()) {
	//		qDebug() << "form rejected: less lines as specified in the threshold (threshLineLenRatio)";
	//		return false;
	//	}

	//	QVector<int> offsetsX;
	//	QVector<int> offsetsY;
	//	//mSrcImg.rows
	//	int sizeDiffY = std::abs(mSizeSrc.height - fTempl.sizeImg().height) + config()->searchYOffset();
	//	int sizeDiffX = std::abs(mSizeSrc.width - fTempl.sizeImg().width) + config()->searchXOffset();

	//	findOffsets(horLinesTemp, verLinesTemp, offsetsX, offsetsY);

	//	float horizontalError = 0;
	//	float verticalError = 0;
	//	float finalErrorH = 0;
	//	float finalErrorV = 0;
	//	cv::Point offSet(0, 0);
	//	double acceptedHor = 0;
	//	double acceptedVer = 0;
	//	double finalAcceptedHor = 0;
	//	double finalAcceptedVer = 0;
	//	double minError = std::numeric_limits<double>::max();
	//	QVector<rdf::Line> horLinesMatched, verLinesMatched;

	//	for (int iX = 0; iX < offsetsX.size(); iX++) {
	//		for (int iY = 0; iY < offsetsY.size(); iY++) {
	//			//for for maximal translation
	//			if (std::abs(offsetsX[iX]) <= sizeDiffX && std::abs(offsetsY[iY]) <= sizeDiffY) {
	//				
	//				//int ox = offsetsX[iX];
	//				//int oy = offsetsY[iY];
	//				//qDebug() << ox << " " << oy;

	//				for (int i = 0; i < mHorLines.size(); i++) {
	//					float tmp = errLine(distImg, mHorLines[i], cv::Point(offsetsX[iX], offsetsY[iY]));
	//					//accept line or not depending on the distance, otherwise assumed as noise
	//					if (tmp < config()->distThreshold()*mHorLines[i].length()) {
	//						horizontalError += tmp < std::numeric_limits<double>::max() ? tmp : 0;
	//						acceptedHor += mHorLines[i].length();
	//						horLinesMatched.push_back(mHorLines[i]);
	//					}
	//				}
	//				for (int i = 0; i < mVerLines.size(); i++) {
	//					float tmp = errLine(distImg, mVerLines[i], cv::Point(offsetsX[iX], offsetsY[iY]));
	//					//accept line or not depending on the distance, otherwise assumed as noise
	//					if (tmp < config()->distThreshold()*mVerLines[i].length()) {
	//						verticalError += tmp < std::numeric_limits<double>::max() ? tmp : 0;
	//						acceptedVer += mVerLines[i].length();
	//						verLinesMatched.push_back(mVerLines[i]);
	//					}
	//				}
	//				float error = horizontalError + verticalError;
	//				if (error <= minError) {
	//					//check also if at least threshLineLenRatio (default: 60%) of the lines are detected and matched with the template image
	//					if (acceptedHor / hLenTemp > config()->threshLineLenRation() && acceptedVer / vLenTemp > config()->threshLineLenRation()) {
	//						minError = error;
	//						finalAcceptedHor = acceptedHor;
	//						finalAcceptedVer = acceptedVer;
	//						finalErrorH = horizontalError;
	//						finalErrorV = verticalError;
	//						offSet.x = offsetsX[iX];
	//						offSet.y = offsetsY[iY];
	//						mHorLinesMatched = horLinesMatched;
	//						mVerLinesMatched = verLinesMatched;
	//					}
	//				}

	//				horizontalError = 0;
	//				verticalError = 0;
	//				acceptedHor = 0;
	//				acceptedVer = 0;
	//				horLinesMatched.clear();
	//				verLinesMatched.clear();
	//			}

	//		}
	//	}

	//	qDebug() << "current Error: " << minError;
	//	qDebug() << "current offSet: " << offSet.x << " " << offSet.y;
	//	if (minError < std::numeric_limits<float>::max()) {
	//		//at least threshLineLenRatio (default: 60%) of the lines are detected and matched with the template image

	//		//check if the average distance of the matched lines is smaller then the errorThr (default: 15px)
	//		if (minError/(finalAcceptedHor+finalAcceptedVer) < config()->errorThr()) {
	//			mOffset = offSet;
	//			mMinError = (double)minError;
	//			return true;
	//		}
	//	}

	//	return false;
	//}


bool FormFeatures::readTemplate(QSharedPointer<rdf::FormFeatures> templateForm) {
	
	if (mTemplateName.isEmpty()) {
		return false;
	}

	QString loadXmlPath = rdf::PageXmlParser::imagePathToXmlPath(mTemplateName);
	
	rdf::PageXmlParser parser;
	if (!parser.read(loadXmlPath)) {
		qWarning() << "could not read template from" << loadXmlPath;
		return false;
	}

	auto pe = parser.page();

	//read xml separators and store them to testinfo
	QVector<rdf::Line> hLines;
	QVector<rdf::Line> vLines;

	QVector<QSharedPointer<rdf::Region>> test = rdf::Region::allRegions(pe->rootRegion().data());
	
	QVector<QSharedPointer<rdf::TableCell>> cells;
	QSharedPointer<rdf::TableRegion> region;

	for (auto i : test) {

		if (i->type() == i->type_table_region) {
			region = i.dynamicCast<rdf::TableRegion>();

		}
		else if (i->type() == i->type_table_cell) {
			//rdf::TableCell* tCell = dynamic_cast<rdf::TableCell*>(i.data());
			QSharedPointer<rdf::TableCell> tCell = i.dynamicCast<rdf::TableCell>();
			cells.push_back(tCell);

			//check if tCell has a Textline as child, if yes, mark as table header;
			if (!tCell->children().empty()) {
				QVector<QSharedPointer<rdf::Region>> childs = tCell->children();
				for (auto child : childs) {
					if (child->type() == child->type_text_line) {
						tCell->setHeader(true);
						//qDebug() << imgC->filePath() << "detected header...";
						qDebug() << "detected header...";
						break;
					}
				}
			}


			float thickness = 10.0; //30
									//if (tCell && tCell->header()) {
			if (tCell) {

				if (tCell->topBorderVisible()) {
					rdf::Line tmpL = tCell->topBorder();
					tmpL.setThickness(thickness);
					hLines.push_back(tmpL);
				}
				if (tCell->bottomBorderVisible()) {
					rdf::Line tmpL = tCell->bottomBorder();
					tmpL.setThickness(thickness);
					hLines.push_back(tmpL);
				}
				if (tCell->leftBorderVisible()) {
					rdf::Line tmpL = tCell->leftBorder();
					tmpL.setThickness(thickness);
					vLines.push_back(tmpL);
				}
				if (tCell->rightBorderVisible()) {
					rdf::Line tmpL = tCell->rightBorder();
					tmpL.setThickness(thickness);
					vLines.push_back(tmpL);
				}
			}

		}
	}

	std::sort(cells.begin(), cells.end(), rdf::TableCell::compareCells);

	templateForm->setSize(cv::Size(pe->imageSize().width(), pe->imageSize().height()));
	templateForm->setFormName(mTemplateName);
	templateForm->setHorLines(hLines);
	templateForm->setVerLines(vLines);

	templateForm->setRegion(region);
	templateForm->setCells(cells);
	
	mTemplateForm = templateForm;

	return true;
}

bool FormFeatures::estimateRoughAlignment(bool useBinaryImg) {

	if (!mTemplateForm) {
		qWarning() << "no template provided for form matching - aborting";
		return false;
	}

	if (isEmptyLines() || mTemplateForm->isEmptyLines())
		return false;

	cv::Mat lineImg;
		
	//use or generate line form/table image
	if (useBinaryImg && !mBwImg.empty()) {
		lineImg = mBwImg;
	}
	else {
		lineImg = cv::Mat(mSizeSrc, CV_8UC1);
		lineImg = 0;
		rdf::LineTrace::generateLineImage(mHorLines, mVerLines, lineImg, cv::Scalar(255), cv::Scalar(255));
	}

	//calculate distanceTransform
	lineImg = 255 - lineImg;
	cv::Mat distLineImg;
	cv::distanceTransform(lineImg, distLineImg, CV_DIST_L1, CV_DIST_MASK_3, CV_32FC1);

	//generate line form/table template image
	QPointF sizeTemplate = mTemplateForm->region()->rightDownCorner() - mTemplateForm->region()->leftUpperCorner();
	//use 10 pixel as offset
	QPointF offsetSize = QPointF(60, 60);
	sizeTemplate += offsetSize;
	cv::Point2d lU((int)mTemplateForm->region()->leftUpperCorner().x(), (int)mTemplateForm->region()->leftUpperCorner().y());
	cv::Point2d offSetLines = cv::Point2d(offsetSize.x() / 2, offsetSize.y() / 2);
	lU -= offSetLines;


	cv::Size templSize = cv::Size((int)sizeTemplate.x(), (int)sizeTemplate.y());
	//cv::Mat tmplImg(templSize, CV_32FC1);
	//tmplImg.setTo(0.0);
	cv::Mat lineTempl(templSize, CV_8UC1);
	lineTempl = 0;
	rdf::LineTrace::generateLineImage(mTemplateForm->horLines(), mTemplateForm->verLines(), lineTempl, cv::Scalar(255), cv::Scalar(255), -lU);
	lineTempl = 255 - lineTempl;
	cv::Mat distTmplImg;
	cv::distanceTransform(lineTempl, distTmplImg, CV_DIST_L1, CV_DIST_MASK_3, CV_32FC1); //cityblock

	double minVtest, maxVtest;
	cv::minMaxLoc(distTmplImg, &minVtest, &maxVtest);

	//cut higher distance values in form/table line image (occurs if shifts are present)
	cv::threshold(distLineImg, distLineImg, maxVtest, maxVtest, cv::THRESH_TRUNC);

	//normalize the images
	cv::normalize(distTmplImg, distTmplImg, 0, 1.0, cv::NORM_MINMAX);
	distTmplImg = 1.0 - distTmplImg;
	//rdf::Image::save(distTmplImg, "D:\\tmp\\templateImg.png");
	cv::normalize(distLineImg, distLineImg, 0, 1.0, cv::NORM_MINMAX);
	distLineImg = 1.0 - distLineImg;

	//calculate row and column sum
	cv::Mat tmplRowSum, tmplColSum;
	cv::Mat formRowSum, formColSum;

	cv::reduce(distTmplImg, tmplRowSum, 1, cv::REDUCE_SUM);
	cv::reduce(distTmplImg, tmplColSum, 0, cv::REDUCE_SUM);
	cv::reduce(distLineImg, formRowSum, 1, cv::REDUCE_SUM);
	cv::reduce(distLineImg, formColSum, 0, cv::REDUCE_SUM);

	//determine alignment using correlation
	cv::Mat outIndex;
	cv::matchTemplate(formRowSum, tmplRowSum, outIndex, cv::TM_CCOEFF_NORMED);
	cv::Point2d shift;
	double minV, maxV;
	cv::Point minLoc, maxLoc;
	cv::minMaxLoc(outIndex, &minV, &maxV, &minLoc, &maxLoc);
	qDebug() << "Shift y: " << "  " << maxLoc.y;
	shift.y = maxLoc.y;

	cv::matchTemplate(formColSum, tmplColSum, outIndex, cv::TM_CCOEFF_NORMED);
	cv::minMaxLoc(outIndex, &minV, &maxV, &minLoc, &maxLoc);
	qDebug() << "Shift x: " << maxLoc.x << "  ";
	shift.x = maxLoc.x;

	mOffset = -lU + shift;

	//rdf::LineTrace::generateLineImage(mTemplateForm->horLines(), mTemplateForm->verLines(), mSrcImg, cv::Scalar(255, 0, 0), cv::Scalar(255, 0, 0), -lU + shift);

	return true;
}

cv::Mat FormFeatures::drawAlignment(cv::Mat img) {

	if (!mTemplateForm)
		return cv::Mat();

	if (!img.empty()) {
		cv::Mat tmp = img.clone();
		rdf::LineTrace::generateLineImage(mTemplateForm->horLines(), mTemplateForm->verLines(), tmp, cv::Scalar(255,0,0), cv::Scalar(255,0,0), mOffset);
		return tmp;
	} else if (mSrcImg.empty()) {
		cv::Mat alignmentImg(mSizeSrc, CV_8UC3);

		rdf::LineTrace::generateLineImage(mHorLines, mVerLines, alignmentImg, cv::Scalar(0, 255, 0), cv::Scalar(0, 255, 0), mOffset);
		rdf::LineTrace::generateLineImage(mTemplateForm->horLines(), mTemplateForm->verLines(), alignmentImg, cv::Scalar(255, 0, 0), cv::Scalar(255, 0, 0), mOffset);
		
		return alignmentImg;
	} else {
		cv::Mat tmp = mSrcImg.clone();
		rdf::LineTrace::generateLineImage(mTemplateForm->horLines(), mTemplateForm->verLines(), tmp, cv::Scalar(255), cv::Scalar(255), mOffset);

		return tmp;
	}

	
}

cv::Mat FormFeatures::drawMatchedForm(cv::Mat img, float t) {

	QVector<rdf::Line> hLines, vLines;
	//create line vectors

	//for (int i = 0; i < mCells.size(); i++) {
		//if (i == 3) break;
		//QSharedPointer<rdf::TableCell> c = mCells[i];
		for (auto c : mCells) {
			if (c->topBorderVisible()) {
				rdf::Line tmp = c->topBorder();
				tmp.setThickness(t);
				hLines.push_back(tmp);
			}
			if (c->bottomBorderVisible()) {
				rdf::Line tmp = c->bottomBorder();
				tmp.setThickness(t);
				hLines.push_back(tmp);
			}
			if (c->leftBorderVisible()) {
				rdf::Line tmp = c->leftBorder();
				tmp.setThickness(t);
				vLines.push_back(tmp);
			}
			if (c->rightBorderVisible()) {
				rdf::Line tmp = c->rightBorder();
				tmp.setThickness(t);
				vLines.push_back(tmp);
			}
		//}
	}

	if (!img.empty()) {
		cv::Mat tmp = img.clone();
		rdf::LineTrace::generateLineImage(hLines, vLines, tmp, cv::Scalar(255,255,255), cv::Scalar(255,255,255));
		return tmp;
	} else {
		cv::Mat tmp = mSrcImg.clone();
		rdf::LineTrace::generateLineImage(hLines, vLines, tmp, cv::Scalar(255,255,255), cv::Scalar(255,255,255));
		return tmp;
	}

	
}

cv::Mat FormFeatures::drawLinesNotUsedForm(cv::Mat img, float t) {


	QVector<rdf::Line> hLines, vLines;
	//create line vectors

	//hLines = notUsedHorLines();
	//vLines = notUseVerLines();

	hLines = filterHorLines();
	vLines = filterVerLines();

	for (auto i : hLines) {
		i.setThickness(t);
	}
	for (auto i : vLines) {
		i.setThickness(t);
	}

	if (!img.empty()) {
		cv::Mat tmp = img.clone();
		if (tmp.channels() == 1) {
			rdf::LineTrace::generateLineImage(hLines, vLines, tmp, cv::Scalar(255), cv::Scalar(255));
		} else {
			rdf::LineTrace::generateLineImage(hLines, vLines, tmp, cv::Scalar(0,0,255), cv::Scalar(0,255,0));
		}
	
		return	tmp;
	}
	else {
		cv::Mat tmp = mSrcImg.clone();
		rdf::LineTrace::generateLineImage(hLines, vLines, tmp, cv::Scalar(255), cv::Scalar(255));

		return tmp;
	}

	
}

cv::Mat FormFeatures::drawMaxClique(cv::Mat img, float t) {
	
	QVector<rdf::Line> hLines, vLines;
	//create line vectors

	QSet<int> mMaxVer;
	if (mMaxCliquesVer.size() > 0)
		mMaxVer = mMaxCliquesVer[mMaxCliquesVer.size() - 1];

	QSet<int>::iterator it;
	for (it = mMaxVer.begin(); it != mMaxVer.end(); ++it) {
		//check id -> int vs string!!
		QSharedPointer<rdf::TableCellRaw> cell = mCellsR[mANodesVertical[*it]->cellIdx()]; //= getCellId(cellsR, mANodesVertical[*it]->cellIdx());
		rdf::Line imgLine = mANodesVertical[*it]->matchedLine();
		imgLine.setThickness(t);
		vLines.push_back(imgLine);
	}

	QSet<int> mMaxHor;
	if (mMaxCliquesHor.size() > 0)
		mMaxHor = mMaxCliquesHor[mMaxCliquesHor.size() - 1];

	for (it = mMaxHor.begin(); it != mMaxHor.end(); ++it) {
		//check id -> int vs string!!
		QSharedPointer<rdf::TableCellRaw> cell = mCellsR[mANodesHorizontal[*it]->cellIdx()]; //= getCellId(cellsR, mANodesVertical[*it]->cellIdx());
		rdf::Line imgLine = mANodesHorizontal[*it]->matchedLine();
		imgLine.setThickness(t);
		hLines.push_back(imgLine);
	}

	if (!img.empty()) {
		cv::Mat tmp = img.clone();
		if (tmp.channels() == 1) {
			rdf::LineTrace::generateLineImage(hLines, vLines, tmp, cv::Scalar(255), cv::Scalar(255));
		}
		else {
			rdf::LineTrace::generateLineImage(hLines, vLines, tmp, cv::Scalar(0, 0, 255), cv::Scalar(0, 255, 0));
		}

		return	tmp;
	}
	else {
		cv::Mat tmp = mSrcImg.clone();
		rdf::LineTrace::generateLineImage(hLines, vLines, tmp, cv::Scalar(255), cv::Scalar(255));

		return tmp;
	}
}

QSharedPointer<rdf::TableRegion> FormFeatures::tableRegion() {


	if (mRegion.isNull()) {
		QSharedPointer<rdf::TableRegion> region(new rdf::TableRegion());
		return region;
	} else {

		//for (auto i : mCells) {
		for (int i = 0; i < mCells.size(); i++) {

			// ------------- children of cells are now set in matchTemplate() ------------------------
			//is done in matchTemplate
			//if (!mTemplateForm.isNull()) {
			//	//set children of parent?
			//	QVector<QSharedPointer<rdf::TableCell>> templateCells =  mTemplateForm->cells();
			//	if (i < templateCells.size()) {
			//		mCells[i]->setChildren(templateCells[i]->children());
			//	}
			//}
			// --------------------------------------------------------------------------------------
			mRegion->addChild(mCells[i]);
		}
	}
		
	return mRegion;
}

QVector<QSharedPointer<rdf::TableCellRaw>> FormFeatures::createRawTableFromTemplate() {

	QVector<QSharedPointer<rdf::TableCell>> cells = mTemplateForm->cells();

	QVector<QSharedPointer<rdf::TableCellRaw>> cellsR;
	//generate cells
	//build raw table from template
	//new structure is initialised
	for (int cellIdx = 0; cellIdx < cells.size(); cellIdx++) {
		QSharedPointer<rdf::TableCellRaw> newCellR(new rdf::TableCellRaw());

		//set cell information (a priori known structure)
		newCellR->setHeader(cells[cellIdx]->header());
		newCellR->setId(cells[cellIdx]->id());
		newCellR->setCol(cells[cellIdx]->col());
		newCellR->setRow(cells[cellIdx]->row());
		newCellR->setColSpan(cells[cellIdx]->colSpan());
		newCellR->setRowSpan(cells[cellIdx]->rowSpan());
		newCellR->setTopBorderVisible(cells[cellIdx]->topBorderVisible());
		newCellR->setBottomBorderVisible(cells[cellIdx]->bottomBorderVisible());
		newCellR->setLeftBorderVisible(cells[cellIdx]->leftBorderVisible());
		newCellR->setRightBorderVisible(cells[cellIdx]->rightBorderVisible());
		newCellR->setPolygon(cells[cellIdx]->polygon());
		newCellR->setCornerPts(cells[cellIdx]->cornerPty());

		cellsR.push_back(newCellR);
	}

	//search for all cell neighbours of a cell
	for (int cellIdx = 0; cellIdx < cells.size(); cellIdx++) {
		QSharedPointer<rdf::TableCellRaw> newCellR;
		newCellR = cellsR[cellIdx];

		//set left Neighbour and vice versa (multiple neighbours are possible
		//cells are sorted -> previous cell is left neighbour if it is the same row,
		//otherwise find previous cell of the same row (happens if rowspan is active)
		if (cellIdx > 0 && newCellR->col() > 0) {
			//due to sorting: if row index is the same of the current and the previous element,
			//the previous element is the left neighbour
			if (cellsR[cellIdx - 1]->row() == newCellR->row()) {
				newCellR->setLeftIdx(cellIdx - 1);
				cellsR[cellIdx - 1]->setRightIdx(cellIdx);
				//check if mutiple left cells exist...
				if (newCellR->rowSpan() > 1) {
					int tmpIdx = cellIdx + 1;
					for (; tmpIdx < cells.size(); tmpIdx++) {
						if (cells[tmpIdx]->col() + cells[tmpIdx]->colSpan() == newCellR->col()) {
							//it is left neighbour
							if (cells[tmpIdx]->row() < newCellR->row() + newCellR->rowSpan() && cells[tmpIdx]->row() > newCellR->row()) {
								newCellR->setLeftIdx(tmpIdx);
								cellsR[tmpIdx]->setRightIdx(cellIdx);
							}
						}
					}
				}

			}
			else { //search for left neighbour (backwards until colIdx = currentColIdx-1)
				int tmpIdx = cellIdx - 1;
				for (; tmpIdx >= 0; tmpIdx--) {
					if (cells[tmpIdx]->col() == (newCellR->col() - 1)) {
						newCellR->setLeftIdx(tmpIdx);
						cellsR[tmpIdx]->setRightIdx(cellIdx);
						break;
					}
				}
			}
		}

		//set upper Neighbour  and vice versa
		if (cellIdx > 0 && newCellR->row() > 0) {
			int tmpIdx = cellIdx - 1;

			for (; tmpIdx >= 0; tmpIdx--) {
				//if row+rowspan of current cell equals the current row, the previous row is detected
				if ((cellsR[tmpIdx]->row() + cellsR[tmpIdx]->rowSpan()) == newCellR->row()) {
					//collIdx is the same as current cell -> it is upper Neighbour
					if (newCellR->col() == cellsR[tmpIdx]->col()) {
						//newCellR->setTopIdx(tmpIdx);
						//if current colSpan > 1 -> add neighbour for all spanned cells, happens in the following case:
						//          +------+----------+
						//          |  X   | add also |
						//          +------+----------+
						//          |   cellIdx       |
						//			+-----------------+
						for (int tmpColSpan = newCellR->colSpan(); tmpColSpan > 0; tmpColSpan--) {
							int cellIdxTmp = newCellR->colSpan() - tmpColSpan;
							newCellR->setTopIdx(tmpIdx + cellIdxTmp);
							cellsR[tmpIdx + cellIdxTmp]->setBottomIdx(cellIdx);
						}
					}

					//if upperColIdx doesn't exist because of colSpan - check if the cell spans the current cell: happens in the following case:
					//			+-----------------+
					//          |                 |
					//          +------+----------+
					//          |      |  cellIdx |
					//          +------+----------+
					if (cellsR[tmpIdx]->col() < newCellR->col() && (cellsR[tmpIdx]->col() + cellsR[tmpIdx]->colSpan() > newCellR->col())) {
						newCellR->setTopIdx(tmpIdx);
						cellsR[tmpIdx]->setBottomIdx(cellIdx);
					}
				}
			}
		}
	}


	return cellsR;
}

void FormFeatures::createAssociationGraphNodes(QVector<QSharedPointer<rdf::TableCellRaw>> cellsR) {

	//QVector<QSharedPointer<rdf::AssociationGraphNode>> nodes;
	//find all nodes for the association graph
	for (int cellIdx = 0; cellIdx < cellsR.size(); cellIdx++) {

		qDebug() << "try to match cell lines of cell for associaton graph nodes: " << cellsR[cellIdx]->row() << " " << cellsR[cellIdx]->col() << " isHeader: " << cellsR[cellIdx]->header();


		for (int i = AssociationGraphNode::LinePosition::pos_left; i <= AssociationGraphNode::LinePosition::pos_bottom; i++) {
			rdf::Line l;
			bool visible = false;
			bool horizontal = false;
			double d = 0;
			QVector<int> neighbourIdx;
			AssociationGraphNode::LinePosition lp;

			switch (i)
			{
			case AssociationGraphNode::LinePosition::pos_top:
				//check if node already exists...
				neighbourIdx = cellsR[cellIdx]->topIdx();
				//if top neighbour idx exist, do not add node
				if (neighbourIdx.size() == 0) {
					l = cellsR[cellIdx]->topBorder();
					visible = cellsR[cellIdx]->topBorderVisible();
					d = findMinWidth(cellsR, cellIdx, i);
					lp = AssociationGraphNode::LinePosition::pos_top;
					horizontal = true;
				}
				break;
			case AssociationGraphNode::LinePosition::pos_bottom:
				l = cellsR[cellIdx]->bottomBorder();
				visible = cellsR[cellIdx]->bottomBorderVisible();
				d = findMinWidth(cellsR, cellIdx, i);
				lp = AssociationGraphNode::LinePosition::pos_bottom;
				horizontal = true;
				break;
			case AssociationGraphNode::LinePosition::pos_left:
				//check if node already exists...
				neighbourIdx = cellsR[cellIdx]->leftIdx();
				//if top neighbour idx exist, do not add node
				if (neighbourIdx.size() == 0) {
					l = cellsR[cellIdx]->leftBorder();
					visible = cellsR[cellIdx]->leftBorderVisible();
					d = findMinWidth(cellsR, cellIdx, i);
					lp = AssociationGraphNode::LinePosition::pos_left;
					horizontal = false;
				}
				break;
			case AssociationGraphNode::LinePosition::pos_right:
				l = cellsR[cellIdx]->rightBorder();
				visible = cellsR[cellIdx]->rightBorderVisible();
				d = findMinWidth(cellsR, cellIdx, i);
				lp = AssociationGraphNode::LinePosition::pos_right;
				horizontal = false;
				break;
			default:
				qWarning() << "no Cell Border found in matchTemplate";
				break;
			}
			if (visible) {
				//one line added -> add 1 to minimum graph size
				if (horizontal)
					mMinGraphSizeHor++;
				else
					mMinGraphSizeVer++;

				d = d < config()->distThreshold() ? config()->distThreshold() : d; //search size is minimum width of the neighbouring cell
				d = d == std::numeric_limits<double>::max() ? config()->distThreshold() : d;
				l.translate(mOffset);

				LineCandidates lC = findLineCandidates(l, d, horizontal);
				QVector<int> lineIdx = lC.candidatesIdx();
				QVector<double> overlaps = lC.overlaps();
				QVector<double> distances = lC.distances();

				for (int lI = 0; lI < lineIdx.size(); lI++) {

					QSharedPointer<rdf::AssociationGraphNode> newNode(new rdf::AssociationGraphNode());
					newNode->setLineCell(cellsR[cellIdx]->row(), cellsR[cellIdx]->col());
					newNode->setCellIdx(cellIdx);
					newNode->setLinePos(lp);
					newNode->setReferenceLine(l);
					rdf::Line cLine = horizontal ? mHorLines[lineIdx[lI]] : mVerLines[lineIdx[lI]];
					newNode->setMatchedLine(cLine, overlaps[lI], distances[lI]);
					newNode->setMatchedLineIdx(lineIdx[lI]);
					if (horizontal)
						mANodesHorizontal.push_back(newNode);
					else
						mANodesVertical.push_back(newNode);
				}
			}

		}

	}
}

void FormFeatures::createAssociationGraph() {

	//create graph for vertical lines
	for (int currentNodeIdx = 0; currentNodeIdx < mANodesVertical.size(); currentNodeIdx++) {
		for (int compareNodeIdx = currentNodeIdx + 1; compareNodeIdx < mANodesVertical.size(); compareNodeIdx++) {

			//test if nodes can be associated
			if (mANodesVertical[currentNodeIdx]->testAdjacency(mANodesVertical[compareNodeIdx])) {
				mANodesVertical[currentNodeIdx]->addAdjacencyNode(compareNodeIdx);
				mANodesVertical[compareNodeIdx]->addAdjacencyNode(currentNodeIdx);
			}
		}
	}

	//create graph for horizontal lines
	for (int currentNodeIdx = 0; currentNodeIdx < mANodesHorizontal.size(); currentNodeIdx++) {
		for (int compareNodeIdx = currentNodeIdx + 1; compareNodeIdx < mANodesHorizontal.size(); compareNodeIdx++) {

			//test if nodes can be associated
			if (mANodesHorizontal[currentNodeIdx]->testAdjacency(mANodesHorizontal[compareNodeIdx])) {
				mANodesHorizontal[currentNodeIdx]->addAdjacencyNode(compareNodeIdx);
				mANodesHorizontal[compareNodeIdx]->addAdjacencyNode(currentNodeIdx);
			}
		}
	}


}

void FormFeatures::findMaxCliques() {

	//QVector<int> nodesIdx;
	QSet<int> c, p;
	QSet<int> nodesIdx;
	QVector<QSet<int>> *pMaxCliques;
	//int minSize = 4;

	////only test of Bron Kerbosch
	//QSharedPointer<rdf::AssociationGraphNode> newNode0(new rdf::AssociationGraphNode());
	//newNode0->addAdjacencyNode(1); newNode0->addAdjacencyNode(2); newNode0->addAdjacencyNode(3); newNode0->addAdjacencyNode(4);
	//QSharedPointer<rdf::AssociationGraphNode> newNode1(new rdf::AssociationGraphNode());
	//newNode1->addAdjacencyNode(0); newNode1->addAdjacencyNode(2); newNode1->addAdjacencyNode(3); newNode1->addAdjacencyNode(4);
	//QSharedPointer<rdf::AssociationGraphNode> newNode2(new rdf::AssociationGraphNode());
	//newNode2->addAdjacencyNode(0); newNode2->addAdjacencyNode(1);
	//QSharedPointer<rdf::AssociationGraphNode> newNode3(new rdf::AssociationGraphNode());
	//newNode3->addAdjacencyNode(0); newNode3->addAdjacencyNode(1); newNode3->addAdjacencyNode(4);
	//QSharedPointer<rdf::AssociationGraphNode> newNode4(new rdf::AssociationGraphNode());
	//newNode4->addAdjacencyNode(0); newNode4->addAdjacencyNode(1); newNode4->addAdjacencyNode(3);
	//testNodes.push_back(newNode0);
	//testNodes.push_back(newNode1);
	//testNodes.push_back(newNode2);
	//testNodes.push_back(newNode3);
	//testNodes.push_back(newNode4);
	//for (int i = 0; i < testNodes.size(); i++) {
	//	//nodesIdx << i;
	//	nodesIdx.insert(i);
	//}
	//pMaxCliques = &mMaxCliquesVer;
	//BronKerbosch(c, nodesIdx, p, pMaxCliques, minSize);
	////end of test
	
	//test
	mMinGraphSizeVer = 28;

	//create set of nodeIdx for vertical nodes
	for (int i = 0; i < mANodesVertical.size(); i++) {
		//nodesIdx << i;
		nodesIdx.insert(i);
	}
	pMaxCliques = &mMaxCliquesVer;
	BronKerbosch(c, nodesIdx, p, pMaxCliques, &mMinGraphSizeVer);



	//nodesIdx.clear();
	////create set of nodeIdx for horizontal nodes
	//for (int i = 0; i < mANodesHorizontal.size(); i++) {
	//	//nodesIdx << i;
	//	nodesIdx.insert(i);
	//}
	//pMaxCliques = &mMaxCliquesHor;

}

void FormFeatures::BronKerbosch(QSet<int> cliqueIdx, QSet<int> nextExpansionsIdx, QSet<int> previousExpansionsIdx, QVector<QSet<int>> *maxCliques, int *minSize) {

	if (nextExpansionsIdx.isEmpty() && previousExpansionsIdx.isEmpty()) {

		//cliqueIdx is maximal clique
		*minSize = cliqueIdx.size() > *minSize ? cliqueIdx.size() : *minSize;
		qDebug() << "max Clique found....." << *minSize;
		maxCliques->append(cliqueIdx);
		//return;
	}

	////check if this could moved to for loop!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	if (!previousExpansionsIdx.isEmpty()) {
		QSet<int>::iterator it;
		for (it = previousExpansionsIdx.begin(); it != previousExpansionsIdx.end(); ++it) {
			QSet<int> tmp = mANodesVertical[*it]->adjacencyNodesSet();
			tmp = tmp.intersect(nextExpansionsIdx);
			if (tmp.size() == nextExpansionsIdx.size())
				return;
		}
	}

	QSet<int>::iterator it;
	QSet<int> nextExpansionsCopy = nextExpansionsIdx;
	//QSet<int> prevExpansionsNeighbours;

	for (it = nextExpansionsIdx.begin(); it != nextExpansionsIdx.end(); ++it) {

		QSet<int> neighbourNodes = mANodesVertical[*it]->adjacencyNodesSet();

		//QSet<int> neighbourNodes = testNodes[*it]->adjacencyNodesSet();
		//QSet<int> NN = nextExpansionsIdx.intersect(neighbourNodes);
		QSet<int> NN = nextExpansionsCopy;	//we need a copy since intersect changes the original set
		NN = NN.intersect(neighbourNodes);
		//QSet<int> PN = previousExpansionsIdx.intersect(neighbourNodes);
		QSet<int> PN = previousExpansionsIdx;
		PN = PN.intersect(neighbourNodes);
		QSet<int> CN = cliqueIdx;
		//qDebug() << "key: " << (int)(*it);
		CN += (*it);

		//prevExpansionsNeighbours = prevExpansionsNeighbours.intersect(nextExpansionsCopy);
		//if (nextExpansionsCopy.size() == prevExpansionsNeighbours.size())
			//break;

		//iterate only if minSize could be achieved
		if (CN.size() + NN.size() > *minSize)
			BronKerbosch(CN, NN, PN, maxCliques, minSize);

		//nextExpansionsIdx.remove(*it);
		nextExpansionsCopy.remove(*it);
		previousExpansionsIdx.insert(*it);
		
		//prevExpansionsNeighbours = neighbourNodes;
	}
}

QVector<QSet<int>> FormFeatures::getMaxCliqueHor() const {
	return mMaxCliquesHor;
}

QVector<QSet<int>> FormFeatures::getMaxCliqueVer() const {
	return mMaxCliquesVer;
}


QVector<QSharedPointer<rdf::TableCellRaw>> FormFeatures::findLineCandidatesForCells(QVector<QSharedPointer<rdf::TableCellRaw>> cellR) {

	//find all line candidates for all cells
	for (int cellIdx = 0; cellIdx < cellR.size(); cellIdx++) {

		qDebug() << "try to match cell : " << cellR[cellIdx]->row() << " " << cellR[cellIdx]->col() << " isHeader: " << cellR[cellIdx]->header();

		//shift cell lines according offset
		rdf::Line tL = cellR[cellIdx]->topBorder();
		tL.translate(mOffset);
		rdf::Line lL = cellR[cellIdx]->leftBorder();
		lL.translate(mOffset);
		rdf::Line rL = cellR[cellIdx]->rightBorder();
		rL.translate(mOffset);
		rdf::Line bL = cellR[cellIdx]->bottomBorder();
		bL.translate(mOffset);

		//find all line candidates width a minimum distance of cell width/height /2
		//overlap can also be 0 for a line candidate
		//0: left 1: right 2; upper 3: bottom
		double d = 0;
		d = findMinWidth(cellR, cellIdx, 2); //if no neighbours are found, threshold is based on the config value
		d = d < config()->distThreshold() ? config()->distThreshold() : d; //search size is minimum width of the neighbouring cell
		d = d == std::numeric_limits<double>::max() ? config()->distThreshold() : d;
		LineCandidates topL = findLineCandidates(tL, d, true);

		d = findMinWidth(cellR, cellIdx, 0);
		d = d < config()->distThreshold() ? config()->distThreshold() : d; //search size is minimum width of the neighbouring cell
		d = d == std::numeric_limits<double>::max() ? config()->distThreshold() : d;
		LineCandidates leftL = findLineCandidates(lL, d, false);

		d = findMinWidth(cellR, cellIdx, 1);
		d = d < config()->distThreshold() ? config()->distThreshold() : d; //search size is minimum width of the neighbouring cell
		d = d == std::numeric_limits<double>::max() ? config()->distThreshold() : d;
		LineCandidates rightL = findLineCandidates(rL, d, false);

		d = findMinWidth(cellR, cellIdx, 3);
		d = d < config()->distThreshold() ? config()->distThreshold() : d; //search size is minimum width of the neighbouring cell
		d = d == std::numeric_limits<double>::max() ? config()->distThreshold() : d;
		LineCandidates bottomL = findLineCandidates(bL, d, true);


		cellR[cellIdx]->setLineCandidatesLeftLine(leftL);
		cellR[cellIdx]->setLineCandidatesRightLine(rightL);
		cellR[cellIdx]->setLineCandidatesTopLine(topL);
		cellR[cellIdx]->setLineCandidatesBottomLine(bottomL);
	}
			
	//what we have: raw table structure; all neighbours of a cell are known by index; for all lines CandidateLines are know
	//TODO: find global optimum of line matching

	return cellR;
}


bool FormFeatures::matchTemplate() {

	if (mTemplateForm.isNull())
		return false;
	
	QVector<QSharedPointer<rdf::TableCell>> cells = mTemplateForm->cells();
	QSharedPointer<rdf::TableRegion> region(new rdf::TableRegion());
	

	//shift table region by offset
	rdf::Polygon newTableRegionPoly = mTemplateForm->region()->polygon();
	QPointF tmpOffset = QPointF((float)mOffset.x, (float)mOffset.y);
	QPolygonF tmp = newTableRegionPoly.polygon();
	tmp.translate(tmpOffset);
	newTableRegionPoly.setPolygon(tmp);
	
	//set region properties
	region->setPolygon(newTableRegionPoly);
	region->setRows(mTemplateForm->region()->rows());
	region->setCols(mTemplateForm->region()->cols());
	region->setId(mTemplateForm->region()->id());
	region->setCustom(mTemplateForm->region()->custom());

	mRegion = region;
	
	QVector<QSharedPointer<rdf::TableCellRaw>> cellsR = createRawTableFromTemplate();
	//create AssociationGraphNodes
	qDebug() << "create Association Graph nodes...";
	createAssociationGraphNodes(cellsR);

	qDebug() << "create Association Graph...";
	//create AssociationGraph
	createAssociationGraph();

	qDebug() << "create Association Graph...";
	//find maximal cliques
	findMaxCliques();
	mCellsR = cellsR;

	////max clique
	//QSet<int> maxCliqueVer;
	//if (mMaxCliquesVer.size() > 0)
	//	maxCliqueVer = mMaxCliquesVer[mMaxCliquesVer.size() - 1];

	//QSet<int>::iterator it;
	//for (it = maxCliqueVer.begin(); it != maxCliqueVer.end(); ++it) {
	//	//check id -> int vs string!!
	//	QSharedPointer<rdf::TableCellRaw> cell = cellsR[mANodesVertical[*it]->cellIdx()]; //= getCellId(cellsR, mANodesVertical[*it]->cellIdx());
	//	rdf::Line imgLine = mANodesVertical[*it]->matchedLine();
	//	int lineIdx = mANodesVertical[*it]->matchedLineIdx();
	//	rdf::AssociationGraphNode::LinePosition lp = mANodesVertical[*it]->linePosition();
	//	
	//	rdf::Line tmpLine;
	//	rdf::LineCandidates tmpC;
	//	cell->clearCandidates();

	//	switch (lp) {
	//	case AssociationGraphNode::LinePosition::pos_top:
	//		tmpC = cell->topLineC();
	//		tmpC.addCandidate(imgLine, lineIdx);
	//		cell->setLineCandidatesTopLine(tmpC);
	//		break;
	//	case AssociationGraphNode::LinePosition::pos_bottom:
	//		tmpC = cell->bottomLineC();
	//		tmpC.addCandidate(imgLine, lineIdx);
	//		cell->setLineCandidatesBottomLine(tmpC);
	//		break;
	//	case AssociationGraphNode::LinePosition::pos_left:
	//		tmpC = cell->leftLineC();
	//		tmpC.addCandidate(imgLine, lineIdx);
	//		cell->setLineCandidatesLeftLine(tmpC);
	//		break;
	//	case AssociationGraphNode::LinePosition::pos_right:
	//		tmpC = cell->rightLineC();
	//		tmpC.addCandidate(imgLine, lineIdx);
	//		cell->setLineCandidatesRightLine(tmpC);
	//		break;
	//	default:
	//		qWarning() << "no Cell Border found in matchTemplate";
	//		break;
	//	}
	//}

	//for (int i = 0; i < cells.size(); i++) {

	//	QSharedPointer<rdf::TableCell> newCell(new rdf::TableCell());

	//	qDebug() << "create new table cell : " << cells[i]->row() << " " << cells[i]->col() << " isHeader: " << cells[i]->header();

	//	//copy attributes
	//	newCell->setHeader(cells[i]->header());
	//	newCell->setId(cells[i]->id());
	//	newCell->setCol(cells[i]->col());
	//	newCell->setRow(cells[i]->row());
	//	newCell->setColSpan(cells[i]->colSpan());
	//	newCell->setRowSpan(cells[i]->rowSpan());
	//	newCell->setTopBorderVisible(cells[i]->topBorderVisible());
	//	newCell->setBottomBorderVisible(cells[i]->bottomBorderVisible());
	//	newCell->setLeftBorderVisible(cells[i]->leftBorderVisible());
	//	newCell->setRightBorderVisible(cells[i]->rightBorderVisible());

	//	//get same cell from rawCells
	//	rdf::Line lL = cellsR[i]->leftLineC().mergedLine();
	//	rdf::Line tL = cellsR[i]->topLineC().mergedLine();
	//	rdf::Line rL = cellsR[i]->rightLineC().mergedLine();
	//	rdf::Line bL = cellsR[i]->bottomLineC().mergedLine();
	//	rdf::Polygon p = createPolygon(tL, lL, rL, bL);
	//		//newCell->setPolygon(p);
	//		//newCell->setCustom(customTmp);
	//		//if (p.size() == 4) {
	//		//	cornerPts << 0 << 1 << 2 << 3;
	//		//	newCell->setCornerPts(cornerPts);
	//		//}
	//		//else {
	//		//	qWarning() << "Wrong number of corners for tablecell...";
	//		//}


	//	//?
	//	//tL.translate(mOffset);
	//	//lL.translate(mOffset);
	//	//tL.translate(mOffset);
	//	//lb.translate(mOffset);

	//	//bool found = false;
	//	//QString customTmp;

	//	////orientation of cornerpts stored by Transkribus:
	//	//// topleft: 0, bottomleft: 1, bottomright: 2, topright: 3 
	//	//QVector<int> cornerPts;
	//	//customTmp = customTmp + (found ? QString("true") : QString("false")) + " ";
	//	////if (newCell->topBorderVisible())
	//	////	newCell->setTopBorderVisible(found);

	//	//lL = findLine(lL, thr, found, false);
	//	//customTmp = customTmp + (found ? QString("true") : QString("false")) + " ";
	//	////if (newCell->leftBorderVisible())
	//	////	newCell->setLeftBorderVisible(found);

	//	//rL = findLine(rL, thr, found, false);
	//	//customTmp = customTmp + (found ? QString("true") : QString("false")) + " ";
	//	////if (newCell->rightBorderVisible())
	//	////	newCell->setRightBorderVisible(found);

	//	//bL = findLine(bL, thr, found);
	//	//customTmp = customTmp + (found ? QString("true") : QString("false"));
	//	////if (newCell->bottomBorderVisible())
	//	////	newCell->setBottomBorderVisible(found);

	////	rdf::Polygon p = createPolygon(tL, lL, rL, bL);
	////	newCell->setPolygon(p);
	////	newCell->setCustom(customTmp);
	////	if (p.size() == 4) {
	////		cornerPts << 0 << 1 << 2 << 3;
	////		newCell->setCornerPts(cornerPts);
	////	}
	////	else {
	////		qWarning() << "Wrong number of corners for tablecell...";
	////	}

	////	rdf::Vector2D offSet = newCell->upperLeft() - c->upperLeft();

	////	//copy children
	////	QVector<QSharedPointer<rdf::Region>> templateChildren = c->children();	//get children from template
	////	QVector<QSharedPointer<rdf::Region>> newChildren;						//will contain the new children vector
	////	if (!templateChildren.isEmpty() && config()->saveChilds()) {

	////		for (QSharedPointer<rdf::Region> ci : templateChildren) {

	////			if (ci->type() == ci->type_text_line) {

	////				QSharedPointer<rdf::TextLine> tTextLine = ci.dynamicCast<rdf::TextLine>();
	////				tTextLine = QSharedPointer<rdf::TextLine>(new rdf::TextLine(*tTextLine));
	////				//get baseline and polygon
	////				rdf::BaseLine tmpBL = tTextLine->baseLine();
	////				rdf::Polygon tmpP = tTextLine->polygon();
	////				//shift by offset;
	////				tmpBL.translate(offSet.toQPointF());
	////				tmpP.translate(offSet.toQPointF());
	////				//set shifted polygon and baseline
	////				tTextLine->setBaseLine(tmpBL);
	////				tTextLine->setPolygon(tmpP);

	////				newChildren.push_back(tTextLine);	//push back altered text line
	////			}
	////			else {
	////				newChildren.push_back(ci);			//otherwise push back pointer to original element
	////			}
	////		}
	////		newCell->setChildren(newChildren);
	////	}

	////	mCells.push_back(newCell);
	//}



	////
	////qDebug() << "size of horizontal max cliques: " << mMaxCliquesHor.size();
	////qDebug() << "size of vertical max cliques: " << mMaxCliquesVer.size();

	////TODO: find global optimum of line matchin
	////-> find largest maximal clique


	//----------------------------------------------------------------------------------------------------------------------------------------------------
	////newer version but not tested...
	//cellsR = findLineCandidatesForCells(cellsR);
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	//old version
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	//generate cells
	for (auto c : cells) {
		
		//QSharedPointer<rdf::TableCell> newCell(new rdf::TableCell(*c));
		QSharedPointer<rdf::TableCell> newCell(new rdf::TableCell());

		qDebug() << "try to match cell : " << c->row() << " " << c->col() << " isHeader: " << c->header();
		
		//copy attributes
		newCell->setHeader(c->header());
		newCell->setId(c->id());
		newCell->setCol(c->col());
		newCell->setRow(c->row());
		newCell->setColSpan(c->colSpan());
		newCell->setRowSpan(c->rowSpan());
		newCell->setTopBorderVisible(c->topBorderVisible());
		newCell->setBottomBorderVisible(c->bottomBorderVisible());
		newCell->setLeftBorderVisible(c->leftBorderVisible());
		newCell->setRightBorderVisible(c->rightBorderVisible());

		rdf::Line tL = c->topBorder();
		tL.translate(mOffset);
		rdf::Line lL = c->leftBorder();
		lL.translate(mOffset);
		rdf::Line rL = c->rightBorder();
		rL.translate(mOffset);
		rdf::Line bL = c->bottomBorder();
		bL.translate(mOffset);

		bool found = false;
		QString customTmp;

		//orientation of cornerpts stored by Transkribus:
		// topleft: 0, bottomleft: 1, bottomright: 2, topright: 3 
		QVector<int> cornerPts;
		double thr = config()->distThreshold();
		//tL = findLine(tL, 100, found);
		tL = findLine(tL, thr, found);
		customTmp = customTmp + (found ? QString("true") : QString("false")) + " ";
		//if (newCell->topBorderVisible())
		//	newCell->setTopBorderVisible(found);

		lL = findLine(lL, thr, found, false);
		customTmp = customTmp + (found ? QString("true") : QString("false")) + " ";
		//if (newCell->leftBorderVisible())
		//	newCell->setLeftBorderVisible(found);

		rL = findLine(rL, thr, found, false);
		customTmp = customTmp + (found ? QString("true") : QString("false")) + " ";
		//if (newCell->rightBorderVisible())
		//	newCell->setRightBorderVisible(found);

		bL = findLine(bL, thr, found);
		customTmp = customTmp + (found ? QString("true") : QString("false"));
		//if (newCell->bottomBorderVisible())
		//	newCell->setBottomBorderVisible(found);

		rdf::Polygon p = createPolygon(tL, lL, rL, bL);
		newCell->setPolygon(p);
		newCell->setCustom(customTmp);
		if (p.size() == 4) {
			cornerPts << 0 << 1 << 2 << 3;
			newCell->setCornerPts(cornerPts);
		} else {
			qWarning() << "Wrong number of corners for tablecell...";
		}

		rdf::Vector2D offSet = newCell->upperLeft() - c->upperLeft();
		
		//copy children
		QVector<QSharedPointer<rdf::Region>> templateChildren = c->children();	//get children from template
		QVector<QSharedPointer<rdf::Region>> newChildren;						//will contain the new children vector
		if (!templateChildren.isEmpty() && config()->saveChilds()) {
			
			for (QSharedPointer<rdf::Region> ci : templateChildren) {

				if (ci->type() == ci->type_text_line) {

					QSharedPointer<rdf::TextLine> tTextLine = ci.dynamicCast<rdf::TextLine>();
					tTextLine = QSharedPointer<rdf::TextLine>(new rdf::TextLine(*tTextLine));
					//get baseline and polygon
					rdf::BaseLine tmpBL = tTextLine->baseLine();
					rdf::Polygon tmpP = tTextLine->polygon();
					//shift by offset;
					tmpBL.translate(offSet.toQPointF());
					tmpP.translate(offSet.toQPointF());
					//set shifted polygon and baseline
					tTextLine->setBaseLine(tmpBL);
					tTextLine->setPolygon(tmpP);

					newChildren.push_back(tTextLine);	//push back altered text line
				} else {
					newChildren.push_back(ci);			//otherwise push back pointer to original element
				}
			}
			newCell->setChildren(newChildren);
		}

		mCells.push_back(newCell);
	}

	return true;
}

rdf::Line FormFeatures::findLine(rdf::Line l, double distThreshold, bool &found, bool horizontal) {

	int index = -1;
	double distance = std::numeric_limits<double>::max();

	if (horizontal) {
		for (int lidx = 0; lidx < mHorLines.size(); lidx++) {
			double d = lineDistance(l, mHorLines[lidx], 0.1);
			if (d < distance) {
				distance = d;
				index = lidx;
			}
		}
	} else {
		for (int lidx = 0; lidx < mVerLines.size(); lidx++) {
			double d = lineDistance(l, mVerLines[lidx], 0.1, false);
			if (d < distance) {
				distance = d;
				index = lidx;
			}
		}
	}


	if (distance > distThreshold)
		index = -1;

	//return correct line
	if (index >= 0 && horizontal) {
		qDebug() << "...matched horizontal line";
		if (std::find(mUsedHorLineIdx.begin(), mUsedHorLineIdx.end(), index) == mUsedHorLineIdx.end()) {
			mUsedHorLineIdx.append(index);
		}
		found = true;
		return mHorLines[index];
	}
	else if (index >= 0 && !horizontal) {
		qDebug() << "...matched vertical line";
		if (std::find(mUsedVerLineIdx.begin(), mUsedVerLineIdx.end(), index) == mUsedVerLineIdx.end()) {
			mUsedVerLineIdx.append(index);
		}
		found = true;
		return mVerLines[index];
	} else {
		found = false;
		return l;
	}
}

rdf::LineCandidates FormFeatures::findLineCandidates(rdf::Line l, double distThreshold, bool horizontal) {
	
	LineCandidates lC;
	lC.setReferenceLine(l);

	if (horizontal) {

		l.sortEndpoints(true);
		for (int lidx = 0; lidx < mHorLines.size(); lidx++) {

			rdf::Line cLine = mHorLines[lidx];
			cLine.sortEndpoints(true);
			double distance = cLine.distance(l.center());

			if (distance < distThreshold) {
				double overlap = l.horizontalOverlap(cLine);
				double len = cLine.length() < l.length() ? cLine.length() : l.length();
				//only add candidate if overlap is larger than 80% in reference to the smaller line
				if ((overlap/len) > 0.8)
					lC.addCandidate(lidx, overlap, distance);
			}
		}

	} else {

		l.sortEndpoints(false);
		for (int lidx = 0; lidx < mVerLines.size(); lidx++) {

			rdf::Line cLine = mVerLines[lidx];
			cLine.sortEndpoints(false);
		
			double distance = cLine.distance(l.center());

			if (distance < distThreshold) {
				double overlap = l.verticalOverlap(cLine);
				double len = cLine.length() < l.length() ? cLine.length() : l.length();
				//only add candidate if overlap is larger than 80% in reference to the smaller line
				if ((overlap / len) > 0.8)
					lC.addCandidate(lidx, overlap, distance);
			}
		}
	}

	return lC;
}

double FormFeatures::findMinWidth(QVector<QSharedPointer<rdf::TableCellRaw>> cellsR, int cellIdx, int neighbour) {
	QVector<int> l;
	double width = std::numeric_limits<double>::max();

	//0: left 1: right 2; upper 3: bottom
	switch (neighbour) {

	case 0: l = cellsR[cellIdx]->leftIdx(); break;
	case 1: l = cellsR[cellIdx]->rightIdx(); break;
	case 2: l = cellsR[cellIdx]->topIdx(); break;
	case 3: l = cellsR[cellIdx]->bottomIdx(); break;
	default:
		break;
	}

	for (int i = 0; i < l.size(); i++) {

		double w = cellsR[l[i]]->width();
		if (w < width) {
			width = w;
		}
	}

	return width;
}


rdf::Polygon FormFeatures::createPolygon(rdf::Line tl, rdf::Line ll, rdf::Line rl, rdf::Line bl) {


	rdf::Vector2D upperLeft, upperRight, bottomLeft, bottomRight;

	//upperLeft = tl.intersection(ll, QLineF::UnboundedIntersection);
	//upperRight = tl.intersection(rl, QLineF::UnboundedIntersection);
	//bottomLeft = bl.intersection(ll, QLineF::UnboundedIntersection);
	//bottomRight = bl.intersection(rl, QLineF::UnboundedIntersection);
	
	upperLeft = tl.intersectionUnrestricted(ll);
	upperRight = tl.intersectionUnrestricted(rl);
	bottomLeft = bl.intersectionUnrestricted(ll);
	bottomRight = bl.intersectionUnrestricted(rl);

	QVector<QPointF> tmp;

	tmp.push_back(upperLeft.toQPointF());
	tmp.push_back(bottomLeft.toQPointF());
	tmp.push_back(bottomRight.toQPointF());
	tmp.push_back(upperRight.toQPointF());

	QPolygonF tmpPoly(tmp);

	return rdf::Polygon(tmpPoly);
}

bool FormFeatures::isEmptyLines() const {
	if (mVerLines.isEmpty() && mHorLines.isEmpty())
		return true;
	else
		return false;
}

bool FormFeatures::isEmptyTable() const {
	if (mRegion.isNull() || mCells.isEmpty())
		return true;
	else
		return false;
}

void FormFeatures::setTemplateName(QString s) {
	mTemplateName = s;
}

QString FormFeatures::templateName() const {
	return mTemplateName;
}

cv::Size FormFeatures::sizeImg() const
	{
		return mSizeSrc;
	}

	void FormFeatures::setSize(cv::Size s)	{
		mSizeSrc = s;
	}

	QVector<rdf::Line> FormFeatures::horLines() const 	{
		return mHorLines;
	}

	void FormFeatures::setHorLines(const QVector<rdf::Line>& h)	{
		mHorLines = h;
	}

	//QVector<rdf::Line> FormFeatures::horLinesMatched() const
	//{
	//	return mHorLinesMatched;
	//}

	QVector<rdf::Line> FormFeatures::verLines() const 	{
		return mVerLines;
	}

	void FormFeatures::setVerLines(const QVector<rdf::Line>& v)	{
		mVerLines = v;
	}

	QVector<rdf::Line> FormFeatures::usedHorLines() const {

		QVector<rdf::Line> usedHor;

		for (auto i : mUsedHorLineIdx) {

			usedHor.push_back(mHorLines[i]);

		}
		return usedHor;
	}

	QVector<rdf::Line> FormFeatures::notUsedHorLines() const {
		QVector<rdf::Line> notUsedHor;

		for (int idx = 0; idx < mHorLines.size(); idx++) {

			if (std::find(mUsedHorLineIdx.begin(), mUsedHorLineIdx.end(), idx) == mUsedHorLineIdx.end()) {
				notUsedHor.push_back(mHorLines[idx]);
			}
		}

		return notUsedHor;
	}

	QVector<rdf::Line> FormFeatures::filterHorLines(double minOverlap, double distThreshold) const {

		QVector<rdf::Line> newHor;
		//QVector<rdf::Line> newVer;

		//create new Table lines
		for (auto c : mCells) {
			newHor.push_back(c->topBorder());
			newHor.push_back(c->bottomBorder());
			//newVer.push_back(c->leftBorder());
			//newVer.push_back(c->rightBorder());
		}

		QVector<rdf::Line> notUsedHorTmp;
		QVector<rdf::Line> filteredLines;

		notUsedHorTmp = notUsedHorLines();
		//QVector<rdf::Line> notUsedVerTmp;

		bool found = false;
		for (auto testLine : notUsedHorTmp) {

			for (auto templateLine : newHor) {
				templateLine.sortEndpoints();
				testLine.sortEndpoints();
				double ol = testLine.horizontalDistance(templateLine, distThreshold);
				ol = ol / (testLine.length() < templateLine.length() ? testLine.length() : templateLine.length());
				if (ol > minOverlap) {
					found = true;
					break;
				}
			}

			if (found) {
				found = false;
				//do not add line
			} else {
				filteredLines.push_back(testLine);
			}
		}

		return filteredLines;
	}

	QVector<rdf::Line> FormFeatures::useVerLines() const {
		QVector<rdf::Line> usedVer;

		for (auto i : mUsedVerLineIdx) {

			usedVer.push_back(mVerLines[i]);

		}
		return usedVer;

	}

	QVector<rdf::Line> FormFeatures::notUseVerLines() const {
		QVector<rdf::Line> notUsedVer;

		for (int idx = 0; idx < mVerLines.size(); idx++) {

			if (std::find(mUsedVerLineIdx.begin(), mUsedVerLineIdx.end(), idx) == mUsedVerLineIdx.end()) {
				notUsedVer.push_back(mVerLines[idx]);
			}
		}

		return notUsedVer;

	}

	QVector<rdf::Line> FormFeatures::filterVerLines(double minOverlap, double distThreshold) const {
		
		QVector<rdf::Line> newVer;

		//create new Table lines
		for (auto c : mCells) {
			newVer.push_back(c->leftBorder());
			newVer.push_back(c->rightBorder());
		}

		QVector<rdf::Line> notUsedVerTmp;
		QVector<rdf::Line> filteredLines;

		notUsedVerTmp = notUseVerLines();

		bool found = false;
		for (auto testLine : notUsedVerTmp) {

			for (auto templateLine : newVer) {
				templateLine.sortEndpoints(false);
				testLine.sortEndpoints(false);
				double ol = testLine.verticalDistance(templateLine, distThreshold);
				ol = ol / (testLine.length() < templateLine.length() ? testLine.length() : templateLine.length());
				if (ol > minOverlap) {
					found = true;
					break;
				}
			}

			if (found) {
				found = false;
				//do not add line
			}
			else {
				filteredLines.push_back(testLine);
			}
		}

		return filteredLines;
	}


	double FormFeatures::lineDistance(rdf::Line templateLine, rdf::Line formLine, double minOverlap, bool horizontal) 	{

		double overlap, length;
		double distance = 0;
		//double midpointDist;

		formLine.sortEndpoints(horizontal);
		templateLine.sortEndpoints(horizontal);

		length = templateLine.length();
		if (horizontal) {

			overlap = templateLine.horizontalOverlap(formLine);
		}
		else {
			overlap = templateLine.verticalOverlap(formLine);
		}

		//templateLine.center()
		distance = formLine.distance(templateLine.center());

		if (overlap / length > minOverlap) {
			distance = (1.0 / (overlap / length)) * distance;
		} else {
			distance = std::numeric_limits<double>::max();
		}

		return distance;
	}

	//QVector<rdf::Line> FormFeatures::verLinesMatched() const	{
	//	return mVerLinesMatched;
	//}

	cv::Point FormFeatures::offset() const	{
		return mOffset;
	}

	double FormFeatures::error() const	{
		return mMinError;
	}

	QSharedPointer<FormFeaturesConfig> FormFeatures::config() const	{
		return qSharedPointerDynamicCast<FormFeaturesConfig>(mConfig);
	}

	void FormFeatures::setConfig(QSharedPointer<FormFeaturesConfig> c) 	{
		mConfig = c;
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

	void FormFeatures::setCells(QVector<QSharedPointer<rdf::TableCell>> c) 	{
		mCells = c;
	}

	QVector<QSharedPointer<rdf::TableCell>> FormFeatures::cells() const 	{
		return mCells;
	}

	void FormFeatures::setRegion(QSharedPointer<rdf::TableRegion> r) 	{
		mRegion = r;
	}

	QSharedPointer<rdf::TableRegion> FormFeatures::region() const 	{
		return mRegion;
	}

	void FormFeatures::setSeparators(QSharedPointer<rdf::Region> r)	{

		//QVector<rdf::Line> tmp = notUsedHorLines();
		QVector<rdf::Line> tmp = filterHorLines();

		//horizontalLines
		for (int i = 0; i < tmp.size(); i++) {

			QSharedPointer<rdf::SeparatorRegion> pSepR(new rdf::SeparatorRegion());
			pSepR->setLine(tmp[i].qLine());

			r->addUniqueChild(pSepR);
		}
		//vertical lines
		//tmp = notUseVerLines();
		tmp = filterVerLines();
		for (int i = 0; i < tmp.size(); i++) {

			QSharedPointer<rdf::SeparatorRegion> pSepR(new rdf::SeparatorRegion());
			pSepR->setLine(tmp[i].qLine());
			r->addUniqueChild(pSepR);
		}
	}

	//QSharedPointer<rdf::TableCellRaw> FormFeatures::getCellId(QVector<QSharedPointer<rdf::TableCellRaw>> cells, int id) const 	{

	//	for (auto i : cells) {
	//		if (i->id() == id)
	//			return i;
	//	}
	//	return QSharedPointer<rdf::TableCellRaw>();
	//}

	//float FormFeatures::errLine(const cv::Mat & distImg, const rdf::Line l, cv::Point offset)
	//{

	//	cv::LineIterator it(mSrcImg, l.p1().toCvPoint2f(), l.p2().toCvPoint2f());
	//	float distance = 0;
	//	float outsidePixel = 0;
	//	float max = 0;

	//	for (int i = 0; i < it.count; i++, ++it) {

	//		cv::Point pos = it.pos();
	//		pos = pos + offset;

	//		if (pos.x < 0 || pos.y < 0 || pos.x >= distImg.cols || pos.y >= distImg.rows) {
	//			//we are outside the image
	//			outsidePixel++;
	//		}
	//		else {
	//			float dist = distImg.at<float>(pos.y, pos.x);
	//			distance += dist;
	//			max = dist > max ? dist : max;
	//		}
	//	}

	//	distance += (max*outsidePixel);
	//	if (distance == 0)
	//		distance = std::numeric_limits<float>::max();

	//	return distance;
	//}

	//void FormFeatures::findOffsets(const QVector<Line>& hT, const QVector<Line>& vT, QVector<int>& offX, QVector<int>& offY) const	{

	//	offY.clear();
	//	offY.push_back(0);
	//	offX.clear();
	//	offX.push_back(0);

	//	//find vertical offsets
	//	if (!hT.empty() && !mHorLines.empty()) {
	//		//use Y difference of horizontal lines if template and current Image contains horizontal lines
	//		for (int i = 0; i < hT.size(); i++) {
	//			double yLineTemp = hT[i].p1().toCvPoint2d().y;
	//			for (int j = 0; j < mHorLines.size(); j++) {
	//				double diffYLine = yLineTemp - mHorLines[j].p1().y();
	//				offY.push_back(qRound(diffYLine));
	//			}
	//		}
	//	}
	//	else if (!vT.empty() && !mVerLines.empty()) {
	//		//use Y difference of starting point of vertical lines if template or current image contains no horizontal lines
	//		for (int i = 0; i < vT.size(); i++) {
	//			double yLineTemp = vT[i].p1().y();
	//			for (int j = 0; j < mVerLines.size(); j++) {
	//				double diffYLine = yLineTemp - mVerLines[j].p1().y();
	//				offY.push_back(qRound(diffYLine));
	//			}
	//		}
	//	}
	//	


	//	//find horizontal offsets
	//	if (!vT.empty() && !mVerLines.empty()) {
	//		//use X difference of vertical lines if template and current Image contains vertical lines
	//		for (int i = 0; i < vT.size(); i++) {
	//			double xLineTemp = vT[i].p1().x();
	//			for (int j = 0; j < mVerLines.size(); j++) {
	//				double diffXLine = xLineTemp - mVerLines[j].p1().x();
	//				offX.push_back(qRound(diffXLine));
	//			}
	//		}
	//	}
	//	else if (!hT.empty() && !mHorLines.empty()) {
	//		//use Y difference of starting point of vertical lines if template or current image contains no horizontal lines
	//		for (int i = 0; i < hT.size(); i++) {
	//			double xLineTemp = hT[i].p1().x();
	//			for (int j = 0; j < mHorLines.size(); j++) {
	//				double diffXLine = xLineTemp - mHorLines[j].p1().x();
	//				offX.push_back(qRound(diffXLine));
	//			}
	//		}
	//	}

	//}

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
	double FormFeaturesConfig::threshLineLenRation() const	{
		return mThreshLineLenRatio;
	}
	void FormFeaturesConfig::setThreshLineLenRation(double s)	{
		mThreshLineLenRatio = s;
	}

	double FormFeaturesConfig::distThreshold() const	{
		return mDistThreshold;
	}

	void FormFeaturesConfig::setDistThreshold(double d)	{
		mDistThreshold = d;
	}

	double FormFeaturesConfig::errorThr() const	{
		return mErrorThr;
	}

	void FormFeaturesConfig::setErrorThr(double e)	{
		mErrorThr = e;
	}

	int FormFeaturesConfig::searchXOffset() const	{
		return mSearchXOffset;
	}

	int FormFeaturesConfig::searchYOffset() const	{
		return mSearchYOffset;
	}

	bool FormFeaturesConfig::saveChilds() const {
		return mSaveChilds;
	}

	void FormFeaturesConfig::setSaveChilds(bool c) 	{
		mSaveChilds = c;
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
		msg += "  mformTemplate: " + mTemplDatabase;
		msg += "  mSaveChilds: " + mSaveChilds;
		return msg;
	}
	
	void FormFeaturesConfig::load(const QSettings & settings)	{
		mThreshLineLenRatio = settings.value("threshLineLenRatio", mThreshLineLenRatio).toDouble();
		mDistThreshold = settings.value("distThreshold", mDistThreshold).toDouble();
		mErrorThr = settings.value("errorThr", mErrorThr).toDouble();
		mTemplDatabase = settings.value("formTemplate", mTemplDatabase).toString();
		mSaveChilds = settings.value("saveChilds", mSaveChilds).toBool();
	}

	void FormFeaturesConfig::save(QSettings & settings) const	{
		settings.setValue("threshLineLenRatio", mThreshLineLenRatio);
		settings.setValue("distThreshold", mDistThreshold);
		settings.setValue("errorThr", mErrorThr);
		settings.setValue("formTemplate", mTemplDatabase);
		settings.setValue("saveChilds", mSaveChilds);

	}

	AssociationGraphNode::AssociationGraphNode() {
	}

	void AssociationGraphNode::setLinePos(const AssociationGraphNode::LinePosition & type) {
		mLinePos = type;
	}

	AssociationGraphNode::LinePosition AssociationGraphNode::linePosition() const {
		return mLinePos;
	}
	void AssociationGraphNode::setReferenceLine(Line l) 	{
		mReferenceLine = l;
	}

	Line AssociationGraphNode::referenceLine() const 	{
		return mReferenceLine;
	}

	int AssociationGraphNode::getRowIdx() const 	{
		return mRefRowIdx;
	}

	int AssociationGraphNode::getColIdx() const 	{
		return mRefColIdx;
	}

	void AssociationGraphNode::setLineCell(int rowIdx, int colIdx) 	{
		mRefColIdx = colIdx;
		mRefRowIdx = rowIdx;
	}

	void AssociationGraphNode::setMatchedLine(Line l) 	{
		mMatchedLine = l;
	}

	void AssociationGraphNode::setMatchedLine(Line l, double overlap, double distance) 	{
		mMatchedLine = l;
		mDistance = distance;
		mOverlap = overlap;
	}

	Line AssociationGraphNode::matchedLine() const	{
		return mMatchedLine;
	}

	void AssociationGraphNode::setMatchedLineIdx(int idx) 	{
		mMatchedLineIdx = idx;
	}

	int AssociationGraphNode::matchedLineIdx() const {
		return mMatchedLineIdx;
	}

	void AssociationGraphNode::setCellIdx(int idx) 	{
		mCellIdx = idx;
	}

	int AssociationGraphNode::cellIdx() const 	{
		return mCellIdx;
	}

	QVector<int> AssociationGraphNode::adjacencyNodes() const 	{
		return mAdjacencyNodesIdx;
	}

	QSet<int> AssociationGraphNode::adjacencyNodesSet() const {
		
		QSet<int> aN;

		for (int i = 0; i < mAdjacencyNodesIdx.size(); i++) {
			aN.insert(mAdjacencyNodesIdx[i]);
		}

		return aN;
	}

	void AssociationGraphNode::addAdjacencyNode(int idx) 	{
		mAdjacencyNodesIdx.push_back(idx);
	}

	bool AssociationGraphNode::testAdjacency(QSharedPointer<AssociationGraphNode> neighbour, double distThreshold) {

		bool horizontal = mLinePos == LinePosition::pos_top || mLinePos == LinePosition::pos_bottom ? true : false;

		//same reference line (same cell and same line position for the reference line - two different matched lines)
		if (mCellIdx == neighbour->cellIdx() && mLinePos == neighbour->linePosition()) {

			//match only, if there is no overlap and lines have the same vertical position
			//rdf::Line m1 = mReferenceLine;			//error?
			rdf::Line m1 = mMatchedLine;
			rdf::Line m2 = neighbour->matchedLine(); 
			m1.sortEndpoints(horizontal);
			m2.sortEndpoints(horizontal);
			double overlap = horizontal ? m1.horizontalOverlap(m2) : m1.verticalOverlap(m2);
			double distance = m1.distance(m2.center());
			//no overlap and same vertical position -> line is split
			//can co-exist
			if (overlap == 0 && distance < distThreshold) {
				//qDebug() << "TEST!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!";
				return true;
			}

		} else {
			//not the same reference line
			rdf::Line ref1 = mReferenceLine;
			rdf::Line ref2 = neighbour->referenceLine();
			rdf::Line m1 = mMatchedLine;
			rdf::Line m2 = neighbour->matchedLine();

			if (!horizontal) {
				ref1.sortEndpoints(horizontal);
				ref2.sortEndpoints(horizontal);
				m1.sortEndpoints(horizontal);
				m2.sortEndpoints(horizontal);
				double dm = std::abs(m1.center().x() - m2.center().x());
				double dref = std::abs(ref1.center().x() - ref2.center().x());

				if (dref*0.8 < dm && dm < dref*1.2) {
					//bool overlapRef = ref1.verticalOverlap(ref2) > 10 ? true : false;
					//bool overlapM = m1.verticalOverlap(m2) > 10 ? true : false;
					//reference line is left from second reference line, same must apply to matched lines
					if (ref1.center().x() < ref2.center().x() && m1.center().x() < m2.center().x()) {
						//if reference lines have an overlap, same must apply to matched lines
						//if ((overlapM && overlapRef) || (!overlapM && !overlapRef))
						return true;
					}
					//reference line is right from second reference line, same must apply to matched lines
					else if (ref1.center().x() > ref2.center().x() && m1.center().x() > m2.center().x()) {
						//if reference lines have an overlap, same must apply to matched lines
						//if ((overlapM && overlapRef) || (!overlapM && !overlapRef))
						return true;
					}
				}
			} else {
				ref1.sortEndpoints(horizontal);
				ref2.sortEndpoints(horizontal);
				m1.sortEndpoints(horizontal);
				m2.sortEndpoints(horizontal);
				double dm = std::abs(m1.center().y() - m2.center().y());
				double dref = std::abs(ref1.center().y() - ref2.center().y());
				if (dref*0.8 < dm && dm < dref*1.2) {
					//bool overlapRef = ref1.horizontalOverlap(ref2) > 10 ? true : false;
					//bool overlapM = m1.horizontalOverlap(m2) > 10 ? true : false;
					//reference line is above from second reference line, same must apply to matched lines
					if (ref1.center().y() < ref2.center().y() && m1.center().y() < m2.center().y()) {
						//if reference lines have an overlap, same must apply to matched lines
						//if ((overlapM && overlapRef) || (!overlapM && !overlapRef))
						return true;
					}
					//reference line is beneath from second reference line, same must apply to matched lines
					else if (ref1.center().y() > ref2.center().y() && m1.center().y() > m2.center().y()) {
						//if reference lines have an overlap, same must apply to matched lines
						//if ((overlapM && overlapRef) || (!overlapM && !overlapRef))
						return true;
					}
				}
			}

		}

		//no association possible
		return false;
	}

	void AssociationGraphNode::clearAdjacencyList() 	{
		mAdjacencyNodesIdx.clear();
	}
	int AssociationGraphNode::degree() const 	{
		return (int)mAdjacencyNodesIdx.size();
	}

}