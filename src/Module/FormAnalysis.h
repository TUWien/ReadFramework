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

#pragma once

#include "BaseModule.h"
#include "LineTrace.h"
#include "Elements.h"
#pragma warning(push, 0)	// no warnings from includes
#include <QObject>

#include "opencv2/core/core.hpp"
#pragma warning(pop)

// TODO: add DllExport magic

// Qt defines

namespace rdf {

	class DllCoreExport FormFeaturesConfig : public ModuleConfig {

	public:
		FormFeaturesConfig();

		double threshLineLenRation() const;
		void setThreshLineLenRation(double s);

		double distThreshold() const;
		void setDistThreshold(double d);

		double errorThr() const;
		void setErrorThr(double e);

		int searchXOffset() const;
		int searchYOffset() const;

		bool saveChilds() const;
		void setSaveChilds(bool c);

		QString templDatabase() const;
		void setTemplDatabase(QString s);


		QString toString() const override;

	private:
		void load(const QSettings& settings) override;
		void save(QSettings& settings) const override;

		//QString mTemplDatabase;
		QString mTemplDatabase = QString("D:\\projects\\READ\\formTest\\form-gt\\Table_Template_M_Aigen-am-Inn_003_01\\page\\M_Aigen_am_Inn_003-01_0001.xml");

		double mThreshLineLenRatio = 0.6;
		//double mDistThreshold = 30.0;
		double mDistThreshold = 100.0;
		double mErrorThr = 15.0;

		bool mSaveChilds = false;

		int mSearchXOffset = 200;
		int mSearchYOffset = 200;

	};


	class DllCoreExport AssociationGraphNode {

	public:
		//0: left 1: right 2; upper 3: bottom
		enum LinePosition {
			pos_left = 0,
			pos_right,
			pos_top,
			pos_bottom
		};
		

		AssociationGraphNode();

		void setLinePos(const AssociationGraphNode::LinePosition& type);
		AssociationGraphNode::LinePosition linePosition() const;

		void setReferenceLine(Line l);
		Line referenceLine() const;

		int getRowIdx() const;
		int getColIdx() const;
		void setLineCell(int rowIdx, int colIdx);

		void setMatchedLine(Line l);
		void setMatchedLine(Line l, double overlap, double distance);
		Line matchedLine() const;

		void setMatchedLineIdx(int idx);
		int matchedLineIdx() const;

	protected:

		Line mReferenceLine;
		LinePosition mLinePos;
		int mRefRowIdx, mRefColIdx;

		Line mMatchedLine;
		int mMatchedLineIdx;
		double mOverlap = -1;
		double mDistance = -1;

	};



	class DllCoreExport FormFeatures : public Module {

	public:
		FormFeatures();
		FormFeatures(const cv::Mat& img, const cv::Mat& mask = cv::Mat());

		void setInputImg(const cv::Mat& img);
		void setMask(const cv::Mat& mask);
		bool isEmpty() const override;
		bool compute() override;
		bool computeBinaryInput();

		//old version
		//bool loadTemplateDatabase(QString db);
		//QVector<rdf::FormFeatures> templatesDb() const;
		//bool compareWithTemplate(const FormFeatures& fTempl);
		//cv::Mat getMatchedLineImg(const cv::Mat& srcImg, const Vector2D& offset = Vector2D(0, 0)) const;
		//QVector<rdf::Line> horLinesMatched() const;
		//QVector<rdf::Line> verLinesMatched() const;
		bool readTemplate(QSharedPointer<rdf::FormFeatures> templateForm);
		bool estimateRoughAlignment(bool useBinaryImg = false);
		cv::Mat drawAlignment(cv::Mat img = cv::Mat());
		cv::Mat drawMatchedForm(cv::Mat img = cv::Mat(), float t = 10.0);
		cv::Mat drawLinesNotUsedForm(cv::Mat img = cv::Mat(), float t = 10.0);
		QSharedPointer<rdf::TableRegion> tableRegion();
		bool matchTemplate();
		rdf::Line findLine(rdf::Line l, double distThreshold, bool &found, bool horizontal = true);
		rdf::LineCandidates findLineCandidates(rdf::Line l, double distThreshold, bool horizontal = true);
		//0: left 1: right 2; upper 3: bottom
		double findMinWidth(QVector<QSharedPointer<rdf::TableCellRaw>> cellsR, QVector<QSharedPointer<rdf::TableCell>> cells, int cellIdx, int neighbour);
		rdf::Polygon createPolygon(rdf::Line tl, rdf::Line ll, rdf::Line rl, rdf::Line bl);

		bool isEmptyLines() const;
		bool isEmptyTable() const;

		void setTemplateName(QString s);
		QString templateName() const;

		cv::Size sizeImg() const;
		void setSize(cv::Size s);
		QVector<rdf::Line> horLines() const;
		void setHorLines(const QVector<rdf::Line>& h);
		QVector<rdf::Line> verLines() const;
		void setVerLines(const QVector<rdf::Line>& v);

		QVector<rdf::Line> usedHorLines() const;
		QVector<rdf::Line> notUsedHorLines() const;
		QVector<rdf::Line> filterHorLines(double minOverlap = 0.1, double distThreshold=20) const;
		QVector<rdf::Line> useVerLines() const;
		QVector<rdf::Line> notUseVerLines() const;
		QVector<rdf::Line> filterVerLines(double minOverlap = 0.1, double distThreshold=20) const;


		double lineDistance(rdf::Line templateLine, rdf::Line formLine, double minOverlap = 0.1, bool horizontal = true);

		cv::Point offset() const;
		double error() const;

		QSharedPointer<FormFeaturesConfig> config() const;
		void setConfig(QSharedPointer<FormFeaturesConfig> c);

		cv::Mat binaryImage() const;
		void setEstimateSkew(bool s);
		//void setThreshLineLenRatio(float l);
		//void setThresh(int thresh);
		//int thresh() const;
		QString toString() const override;

		void setFormName(QString s);
		QString formName() const;

		
		void setCells(QVector<QSharedPointer<rdf::TableCell>> c);
		QVector<QSharedPointer<rdf::TableCell>> cells() const;
		void setRegion(QSharedPointer<rdf::TableRegion> r);
		QSharedPointer<rdf::TableRegion> region() const;
		void setSeparators(QSharedPointer<rdf::Region> r);

	protected:

		//old version
		//float errLine(const cv::Mat& distImg, const rdf::Line l, cv::Point offset = cv::Point(0,0));
		//void findOffsets(const QVector<Line>& hT, const QVector<Line>& vT, QVector<int>& offX, QVector<int>& offY) const;
		

	private:
		cv::Mat mSrcImg;
		cv::Mat mMask;
		cv::Mat mBwImg;
		bool mEstimateSkew = false;
		bool mPreFilter = true;
		int preFilterArea = 10;
		double mPageAngle = 0.0;
		double mMinError = std::numeric_limits<double>::max();

		QVector<rdf::Line> mHorLines;
		QVector<int> mUsedHorLineIdx;
		QVector<rdf::Line> mVerLines;
		QVector<int> mUsedVerLineIdx;

		QVector<QSharedPointer<rdf::AssociationGraphNode>> mANodesHorizontal;
		QVector<QSharedPointer<rdf::AssociationGraphNode>> mANodesVertical;

		//rdf::FormFeatures mTemplateForm;
		QSharedPointer<rdf::FormFeatures> mTemplateForm;

		//QVector<rdf::Line> mHorLinesMatched;
		//QVector<rdf::Line> mVerLinesMatched;
		cv::Point mOffset = cv::Point(0,0);
		cv::Size mSizeSrc;

		// parameters

		bool checkInput() const override;
		QString mFormName;
		QString mTemplateName;

		QVector<QSharedPointer<rdf::TableCell>> mCells;
		QSharedPointer<rdf::TableRegion> mRegion;

		//void load(const QSettings& settings) override;
		//void save(QSettings& settings) const override;
	};

}
