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
#include "PixelSet.h"
#include "Image.h"

#pragma warning(push, 0)	// no warnings from includes
// Qt Includes
#pragma warning(pop)

#ifndef DllCoreExport
#ifdef DLL_CORE_EXPORT
#define DllCoreExport Q_DECL_EXPORT
#else
#define DllCoreExport Q_DECL_IMPORT
#endif
#endif

// Qt defines
class GCoptimizationGeneralGraph;

namespace rdf {

// read defines
class PixelGraph;

class DllCoreExport GraphCutConfig : public ModuleConfig {

public:
	GraphCutConfig(const QString& name = "Graph Cut");

	double scaleFactor() const;
	int numIter() const;

protected:
	void load(const QSettings& settings) override;
	void save(QSettings& settings) const override;

	double mScaleFactor = 1000.0;	// scale factor to use (faster) int instead of double
	int mGcIter = 2;				// # iterations of graph-cut (expansion)
};

/// <summary>
/// The base class for all graphcuts operating on pixels.
/// </summary>
/// <seealso cref="Module" />
class DllCoreExport GraphCutPixel : public Module {

public:
	GraphCutPixel(const PixelSet& set);

	virtual bool isEmpty() const override;

	QSharedPointer<GraphCutConfig> config() const;

	// results - available after compute() is called
	PixelSet set() const;

protected:

	// input/output
	PixelSet mSet;
	PixelDistance::EdgeWeightFunction mWeightFnc;
	QSharedPointer<PixelConnector> mConnector;

	/// <summary>
	/// Performs the graphcut.
	/// The graphcut globally optimizes the pixel states
	/// w.r.t. the costs given
	/// </summary>
	/// <param name="graph">The pixel graph.</param>
	/// <returns></returns>
	QSharedPointer<GCoptimizationGeneralGraph> graphCut(const PixelGraph& graph) const;
	
	/// <summary>
	/// Returns a matrix with the costs for each state.
	/// The matrix must be mSet.size() x numLabels 32SC1.
	/// It contains cost values for each element given a state.
	/// Normalized costs are usually multiplied by mScaleFactor
	/// to fit the data format.
	/// </summary>
	/// <param name="numLabels">The number labels.</param>
	/// <returns></returns>
	virtual cv::Mat costs(int numLabels) const = 0;

	/// <summary>
	/// Indicates the costs to move from one label to another.
	/// High values indicate high costs.
	/// Usually this matrix is symmetric.
	/// </summary>
	/// <param name="numLabels">The number labels.</param>
	/// <returns>A numLabels x numLabels 32SC1 matrix.</returns>
	virtual cv::Mat labelDistMatrix(int numLabels) const = 0;

	virtual int numLabels() const = 0;
};

/// <summary>
/// Graph cut for local orientation estimation.
/// </summary>
/// <seealso cref="GraphCutPixel" />
class DllCoreExport GraphCutOrientation : public GraphCutPixel {

public:
	GraphCutOrientation(const PixelSet& set);

	virtual bool compute() override;

	cv::Mat draw(const cv::Mat& img, const QColor& col = QColor()) const;

private:

	bool checkInput() const override;

	cv::Mat costs(int numLabels) const override;
	cv::Mat labelDistMatrix(int numLabels) const override;
	int numLabels() const override;
};

/// <summary>
/// Graph cut for pixel labeling.
/// </summary>
/// <seealso cref="GraphCutPixel" />
class DllCoreExport GraphCutPixelLabel : public GraphCutPixel {

public:
	GraphCutPixelLabel(const PixelSet& set);

	virtual bool compute() override;

	cv::Mat draw(const cv::Mat& img, const QColor& col = QColor()) const;

	void setLabelManager(const LabelManager& m);

private:

	bool checkInput() const override;

	cv::Mat costs(int numLabels) const override;
	cv::Mat labelDistMatrix(int numLabels) const override;
	int numLabels() const override;

	LabelManager mManager;
};

class DllCoreExport GraphCutLineSpacingConfig : public GraphCutConfig {

public:
	GraphCutLineSpacingConfig();

	int numLabels() const;

protected:
	void load(const QSettings& settings) override;
	void save(QSettings& settings) const override;

	int mNumLabels = 15;		// number of scale bins (empirically set to 15)
};


/// <summary>
/// Graph cut for line spacing.
/// </summary>
/// <seealso cref="GraphCutPixel" />
class DllCoreExport GraphCutLineSpacing : public GraphCutPixel {

public:
	GraphCutLineSpacing(const PixelSet& set);

	virtual bool compute() override;

	QSharedPointer<GraphCutLineSpacingConfig> config() const;

	cv::Mat draw(const cv::Mat& img, const QColor& col = QColor()) const;

private:

	bool checkInput() const override;

	cv::Mat costs(int numLabels) const override;
	cv::Mat labelDistMatrix(int numLabels) const override;
	int numLabels() const override;

	Histogram spacingHist() const;
	//cv::Mat spacingHist() const;

	Histogram mSpaceHist;
};

/// <summary>
/// Textline clustering using graph-cut.
/// </summary>
/// <seealso cref="GraphCutPixel" />
class DllCoreExport GraphCutTextLine : public GraphCutPixel {

public:
	GraphCutTextLine(const QVector<PixelSet>& sets);

	virtual bool compute() override;

	cv::Mat draw(const cv::Mat& img, const QColor& col = QColor()) const;

	QVector<PixelSet> textLines();

private:

	QVector<PixelSet> mTextLines;

	bool checkInput() const override;

	cv::Mat costs(int numLabels) const override;
	cv::Mat labelDistMatrix(int numLabels) const override;
	int numLabels() const override;

	cv::Mat mahalanobisDists(const PixelSet& tl, const cv::Mat& centers) const;
	cv::Mat euclideanDists(const PixelSet& tl) const;
	cv::Mat pixelSetCentersToMat(const PixelSet& set) const;

	void saveDistsDebug(const QString& filePath, const cv::Mat& img) const;

	template <typename num>
	cv::Mat makeSymmetric(const cv::Mat& m) const {

		assert(m.cols == m.rows);
		cv::Mat s = m.clone();

		// make it a metric (symmetric)
		num* lp = s.ptr<num>();

		for (int rIdx = 0; rIdx < s.rows; rIdx++) {

			for (int cIdx = rIdx+1; cIdx < s.cols; cIdx++) {
			
				num* rl = lp + (rIdx * s.rows + cIdx);	// row label
				num* cl = lp + (rIdx + cIdx * s.cols);	// col label

				num val = qMin(*rl, *cl);
				*rl = val;
				*cl = val;
			}
		}

		return s;
	}
};

}
