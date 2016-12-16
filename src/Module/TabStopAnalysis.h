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
// Qt Includes
#pragma warning(pop)

#ifndef DllModuleExport
#ifdef DLL_MODULE_EXPORT
#define DllModuleExport Q_DECL_EXPORT
#else
#define DllModuleExport Q_DECL_IMPORT
#endif
#endif

// Qt defines

namespace rdf {

// read defines

class DllModuleExport TabStopConfig : public ModuleConfig {

public:
	TabStopConfig();

	virtual QString toString() const override;

protected:

	//void load(const QSettings& settings) override;
	//void save(QSettings& settings) const override;
};

class DllModuleExport TabStopCluster {

public:
	TabStopCluster(const QSharedPointer<PixelSet>& ps);

	void setLine(const Line& line);
	Line line() const;
	QSharedPointer<PixelSet> set() const;

	void setAngle(double angle);
	double angle() const;

	void draw(QPainter& p) const;

private:
	QSharedPointer<PixelSet> mSet;
	Line mLine;
	double mMedAngle;
};

class DllModuleExport TabStopAnalysis : public Module {

public:
	TabStopAnalysis(const PixelSet& set = PixelSet());

	bool isEmpty() const override;
	bool compute() override;
	QSharedPointer<TabStopConfig> config() const;

	cv::Mat draw(const cv::Mat& img) const;
	QString toString() const override;

	// getter
	QVector<QSharedPointer<TabStopCluster> > tabStopClusters() const;
	QVector<Line> tabStopLines(double offset = 0.0) const;

private:
	PixelSet mSet;
	QSharedPointer<PixelGraph> mGraph;

	QVector<QSharedPointer<TabStopCluster> > mTabStops;

	bool checkInput() const override;
	
	// find tabs
	QVector<QSharedPointer<Pixel> > findTabStopCandidates(const QSharedPointer<PixelGraph>& graph) const;
	QVector<QSharedPointer<TabStopCluster> > findTabs(const QVector<QSharedPointer<Pixel> >& pixel) const;
	double medianOrientation(const QSharedPointer<PixelSet>& set) const;
	void updateTabStopCandidates(const QSharedPointer<PixelSet>& set, double orientation, const PixelTabStop::Type& newType = PixelTabStop::type_none) const;
};

};