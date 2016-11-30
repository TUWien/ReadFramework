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

class DllModuleExport TextLineConfig : public ModuleConfig {

public:
	TextLineConfig();

	virtual QString toString() const override;

	double minDistFactor() const;
	void setMinDistFactor(double val);

protected:

	double mMinDistFactor = 10.0;

	void load(const QSettings& settings) override;
	void save(QSettings& settings) const override;
};

class DllModuleExport TextLineSegmentation : public Module {

public:
	TextLineSegmentation(const QVector<QSharedPointer<Pixel> >& superPixels = QVector<QSharedPointer<Pixel> >());

	bool isEmpty() const override;
	bool compute() override;
	QSharedPointer<TextLineConfig> config() const;

	cv::Mat draw(const cv::Mat& img) const;
	QString toString() const override;

	void addLines(const QVector<Line>& lines);
	QVector<QSharedPointer<TextLine> > textLines() const;

private:
	QVector<QSharedPointer<Pixel> > mSuperPixels;
	//QVector<QSharedPointer<LineEdge> > mEdges;
	QVector<PixelSet> mSets;
	//QVector<QSharedPointer<LineEdge> > mDbgEdges;	// remove
	QVector<Line> mStopLines;

	bool checkInput() const override;
	QVector<QSharedPointer<LineEdge> > filterEdges(const QVector<QSharedPointer<LineEdge> >& edges, double factor = 10.0) const;
	QVector<QSharedPointer<LineEdge> > filterEdges(const QVector<QSharedPointer<LineEdge> >& edges, const QVector<Line>& lines) const;
	QVector<PixelSet> toSets(const QVector<QSharedPointer<LineEdge> > & edges) const;

	QVector<PixelSet> merge(const QVector<PixelSet>& sets, double overlap = 2.0) const;	// TODO: delete
	QVector<PixelSet> filter(const QVector<PixelSet>& sets, double sizeRatio = 0.5) const;	// TODO: delete
	PixelSet findSet(const QVector<PixelSet>& sets, const QString& id) const;	// TODO: move this to a PixelSetManager


	void slac(const QVector<QSharedPointer<LineEdge> >& edges) const;

};

};