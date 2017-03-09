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
#include "Pixel.h"
#include "PixelSet.h"

#pragma warning(push, 0)	// no warnings from includes

#pragma warning(pop)

#ifndef DllCoreExport
#ifdef DLL_CORE_EXPORT
#define DllCoreExport Q_DECL_EXPORT
#else
#define DllCoreExport Q_DECL_IMPORT
#endif
#endif

// Qt defines

namespace rdf {

// read defines

class DllCoreExport TextLineConfig : public ModuleConfig {

public:
	TextLineConfig();

	virtual QString toString() const override;

	void setMinLineLength(int length);
	int minLineLength() const;

	void setMinPointDistance(double dist);
	double minPointDistance() const;

	void setErrorMultiplier(double multiplier);
	double errorMultiplier() const;

	QString debugPath() const;

protected:

	int mMinLineLength = 10;			// minimum text line length when clustering
	double mMinPointDist = 75.0;		// acceptable minimal distance of a point to a line
	double mErrorMultiplier = 1.5;		// maximal increase of error when merging two lines
	QString mDebugPath = "C:/temp/cluster/";

	void load(const QSettings& settings) override;
	void save(QSettings& settings) const override;
};

class DllCoreExport TextLineSegmentation : public Module {

public:
	TextLineSegmentation(const PixelSet& set = PixelSet());

	bool isEmpty() const override;
	bool compute() override;
	bool compute(const cv::Mat& img);
	QSharedPointer<TextLineConfig> config() const;

	cv::Mat draw(const cv::Mat& img) const;
	static cv::Mat draw(const cv::Mat& img, const QVector<QSharedPointer<TextLineSet> >& textLines);
	QString toString() const override;

	void addSeparatorLines(const QVector<Line>& lines);
	QVector<QSharedPointer<TextLine> > textLines() const;
	QVector<QSharedPointer<TextLineSet> > textLineSets() const;

	// functions applied to the results
	void scale(double s);

private:
	PixelSet mSet;
	//QVector<QSharedPointer<PixelEdge> > mRemovedEdges;		// this is nice for debugging - but I would remove it in the end
	QVector<QSharedPointer<TextLineSet> > mTextLines;
	QVector<Line> mStopLines;

	bool checkInput() const override;

	QVector<QSharedPointer<TextLineSet> > clusterTextLines(const PixelGraph& graph, QVector<QSharedPointer<PixelEdge> >* removedEdges = 0) const;
	QVector<QSharedPointer<TextLineSet> > clusterTextLinesDebug(const PixelGraph& graph, const cv::Mat& img) const;
	int locate(const QSharedPointer<Pixel>& pixel, const QVector<QSharedPointer<TextLineSet> >& sets) const;
	bool addPixel(QSharedPointer<TextLineSet>& set, const QSharedPointer<Pixel>& pixel, double heat) const;
	bool mergeTextLines(const QSharedPointer<TextLineSet>& tln1, const QSharedPointer<TextLineSet>& tln2, double heat) const;
	void filterDuplicates(PixelSet& set) const;

	// post processing
	void mergeUnstableTextLines(QVector<QSharedPointer<TextLineSet> >& textLines) const;

};

};