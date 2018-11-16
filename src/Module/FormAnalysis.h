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

#pragma once

#include "BaseModule.h"
#include "LineTrace.h"
#include "Elements.h"
#pragma warning(push, 0)	// no warnings from includes
#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QDirIterator>
#include <QJsonDocument>

#include <opencv2/core.hpp>
#pragma warning(pop)

// TODO: add DllExport magic

// Qt defines

namespace rdf {

	class DllCoreExport FormFeaturesConfig : public ModuleConfig {

	public:
		FormFeaturesConfig();

		//double threshLineLenRation() const;
		//void setThreshLineLenRation(double s);

		double distThreshold() const;
		void setDistThreshold(double d);

		double errorThr() const;
		void setErrorThr(double e);

		double variationThrLower() const;
		void setVariationThrLower(double v);

		double variationThrUpper() const;
		void setVariationThrUpper(double v);

		double coLinearityThr() const;
		void setCoLinearityThr(double c);

		//int searchXOffset() const;
		//int searchYOffset() const;

		bool saveChilds() const;
		void setSaveChilds(bool c);

		QString templDatabase() const;
		void setTemplDatabase(QString s);

		QString evalPath() const;
		void setevalPath(QString s);

		QString toString() const override;

	private:
		void load(const QSettings& settings) override;
		void save(QSettings& settings) const override;

		//QString mTemplDatabase;
		QString mTemplDatabase = QString("C:\\Users\\flo\\projects\\READ\\formTest\\form - gt\\Table_Template_M_Freyung_014_01\\page\\M_Freyung_014 - 01_0112.xml");
		QString mEvalPath = QString("C:\\Users\\flo\\projects\\READ\\formTest\\form - gt\\Table_Template_M_Freyung_014_01\\page\\");

		//double mThreshLineLenRatio = 0.6;
		//double mDistThreshold = 30.0;
		double mDistThreshold = 200.0;			//threshold is set dynamically - fallback value to find line candidates within mDistThreshold
		double mColinearityThreshold = 20;		//up to which distance a line is colinear
		double mErrorThr = 15.0;				//currently not used
		double mVariationThrLower = 0.2;				//allowed variation for width/height of cells in % (lower bound)
		double mVariationThrUpper = 0.3;				//allowed variation for width/height of cells in % (upper bound)

		bool mSaveChilds = false;

		//int mSearchXOffset = 200;
		//int mSearchYOffset = 200;

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

		int rowSpan() const;
		int colSpan() const;
		void setSpan(int rowSpan, int colSpan);

		void setMatchedLine(Line l);
		void setMatchedLine(Line l, double overlap, double distance);
		Line matchedLine() const;
		double overlap() const;
		double distance() const;

		void addBrokenLine(Line l, int lineIdx);
		bool brokenLinesPresent() const;
		QVector<Line> brokenLines() const;
		QVector<int> brokenLinesIdx() const;

		void setMatchedLineIdx(int idx);
		int matchedLineIdx() const;

		void setCellIdx(int idx);
		int cellIdx() const;

		void setNeighbourCellIdx(QVector<int> n);
		QVector<int> neighbourCellIDx();

		double weight();

		QVector<int> adjacencyNodes() const;
		//QSet<int> adjacencyNodesSet() const;
		//void createAdjacencyNodesSet();
		void addAdjacencyNode(int idx);
		bool testAdjacency(QSharedPointer<AssociationGraphNode> neighbour, double distThreshold = 20, double variationThrLower = 0.2, double variationThrUpper = 0.2);
		void clearAdjacencyList();

		int degree() const;

		bool operator< (const AssociationGraphNode& node) const;
		static bool compareNodes(const QSharedPointer<rdf::AssociationGraphNode> n1, const QSharedPointer<rdf::AssociationGraphNode> n2);

	protected:
		int mCellIdx = 1;
		QVector<int> mMergedNeighbourCells;
		Line mReferenceLine;
		LinePosition mLinePos;
		int mRefRowIdx = -1;
		int mRefColIdx = -1;
		int mRowSpan = 0;
		int mColSpan = 0;

		//line that represents the current cell line
		Line mMatchedLine;
		int mMatchedLineIdx;

		//broken line pieces, but all have a shorter overlap compared to matchedLine
		QVector<Line> mBrokenLines;
		QVector<int> mBrokenLinesIdx;

		double mOverlap = -1;
		double mDistance = -1;

		QVector<int> mAdjacencyNodesIdx;
		QSet<int> mAn;

	};

	class DllCoreExport FormEvaluation {

	public:

		FormEvaluation();
		void setSize(cv::Size s);
		bool setTemplate(QString templateName);
		void setTable(QSharedPointer<rdf::TableRegion> table);
		cv::Mat computeTableImage(QSharedPointer<rdf::TableRegion> table, bool mergeCells = false);
		void computeEvalCells();
		void computeEvalTableRegion();

		double tableJaccard();
		double tableMatch();

		QVector<double> cellJaccards();
		double meanCellJaccard();
		QVector<double> cellMatches();
		double meanCellMatch();

		double missedCells(double threshold = 0.2);
		double underSegmented(double threshold = 0.2);
		QVector<double> underSegmentedC();

	protected:

		QSharedPointer<rdf::TableRegion> mTableRegionTemplate;
		QSharedPointer<rdf::TableRegion> mTableRegionMatched;

		cv::Size mImageSize;
		cv::Mat mTableTemplate;
		cv::Mat mTableMatched;

		double mCellsTemplate = -1;
		double mCellsMatched = -1;

		double mJaccardTable;
		double mMatchTable;

		QVector<double> mJaccardCell;
		QVector<double> mCellMatch;
		QVector<double> mUnderSegmented;

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
		cv::Mat drawLines(cv::Mat img = cv::Mat(), float t = 10.0);
		cv::Mat drawMaxClique(cv::Mat img = cv::Mat(), float t = 10.0, int idx = 0);
		cv::Mat drawMaxCliqueNeighbours(int cellIdx, AssociationGraphNode::LinePosition lp, int nodeCnt = -1, cv::Mat img = cv::Mat(), float t = 10.0);
		QSharedPointer<rdf::TableRegion> tableRegion();
		QSharedPointer<rdf::TableRegion> tableRegionTemplate();
		QVector<QSharedPointer<rdf::TableCellRaw>> createRawTableFromTemplate();
		void createAssociationGraphNodes(QVector<QSharedPointer<rdf::TableCellRaw>> cellsR);
		void createReducedAssociationGraphNodes(QVector<QSharedPointer<rdf::TableCellRaw>> cellsR);
		QVector<QSharedPointer<rdf::AssociationGraphNode>> mergeColinearNodes(QVector<QSharedPointer<rdf::AssociationGraphNode>> &tmpNodes);
		void createAssociationGraph();
		bool** adjacencyMatrix(const QVector<QSharedPointer<rdf::AssociationGraphNode>> &associationGraphNodes);
		void findMaxCliques();
		void createTableFromMaxClique(const QVector<QSharedPointer<rdf::TableCell>> &cells);
		void createTableFromMaxCliqueReduced(const QVector<QSharedPointer<rdf::TableCell>> &cells);
		//void plausibilityCheck();

		QVector<QSet<int>> getMaxCliqueHor() const;
		QVector<QSet<int>> getMaxCliqueVer() const;

		
		bool matchTemplate();
		//QVector<QSharedPointer<rdf::TableCellRaw>> findLineCandidatesForCells(QVector<QSharedPointer<rdf::TableCellRaw>> cellR);
		//rdf::Line findLine(rdf::Line l, double distThreshold, bool &found, bool horizontal = true);
		rdf::LineCandidates findLineCandidates(rdf::Line l, double distThreshold, bool horizontal = true);
		//0: left 1: right 2; upper 3: bottom
		double findMinWidth(QVector<QSharedPointer<rdf::TableCellRaw>> cellsR, int cellIdx, int neighbour);
		rdf::Polygon createPolygon(rdf::Line tl, rdf::Line ll, rdf::Line rl, rdf::Line bl);
		void createCellfromLineCandidates(QVector<QSharedPointer<rdf::TableCellRaw>> cellsR);

		bool isEmptyLines() const;
		bool isEmptyTable() const;

		bool setTemplateName(QString s);
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

		//QSharedPointer<rdf::TableCellRaw> getCellId(QVector<QSharedPointer<rdf::TableCellRaw>> cells, int id) const;

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
		//int mMinGraphSizeHor = 0;
		//int mMinGraphSizeVer = 0;
		//QVector<QSharedPointer<rdf::AssociationGraphNode>> testNodes;

		QVector<QSet<int>> mMaxCliquesHor;
		QVector<QSet<int>> mMaxCliquesVer;
		
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

		//only for drawing
		QVector<QSharedPointer<rdf::TableCellRaw>> mCellsR;
		//void load(const QSettings& settings) override;
		//void save(QSettings& settings) const override;
	};


	class DllCoreExport PieData : public Module {

	public:
		//PieData();
		PieData(const QString xmlDir, const QString jsonFile);
		QJsonObject getImgObject(const QString xmlDoc = QString());
		void saveJsonDatabase();
		

	protected:
		bool calculateFeatures(const QJsonObject &document, QString xmlDoc) const;

	private:
		QString mXmlDir;
		QString mJsonFile;

	};


	//don't care
	//QtJson::JsonObject document;
	//document["name"] = "page1.png"
	//
	//QtJson::JsonArray imgRegions;
	//QtJson::JsonObject image1, image2;
	//image1["size"] = 20;
	//image2["size"] = 40;
	//imgRegions.append(image1);
	//imgRegions.append(image2);

	//QtJson::JsonArray txtRegions;
	//QtJson::JsonObject text1, text2;
	//text1["size"] = 20;
	//text2["size"] = 40;
	//imgRegions.append(text1);
	//imgRegions.append(text2);
	//	
	//document["images"] = imgRegions;
	//document["text"] = txtRegions;
	

}
