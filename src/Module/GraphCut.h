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

/// <summary>
/// The base class for all graphcuts operating on pixels.
/// </summary>
/// <seealso cref="Module" />
class DllCoreExport GraphCutPixel : public Module {

public:
	GraphCutPixel(const PixelSet& set);

	virtual bool isEmpty() const override;

	// results - available after compute() is called
	PixelSet set() const;

protected:

	// input/output
	PixelSet mSet;
	PixelDistance::EdgeWeightFunction mWeightFnc;
	double mScaleFactor = 1000.0;	// TODO: think about that

	QSharedPointer<GCoptimizationGeneralGraph> graphCut(const PixelGraph& graph) const;
	virtual cv::Mat costs(int numLabels) const = 0;
	virtual cv::Mat labelDistMatrix(int numLabels) const = 0;
};

class DllCoreExport GraphCutOrientation : public GraphCutPixel {

public:
	GraphCutOrientation(const PixelSet& set);

	virtual bool compute() override;

	cv::Mat draw(const cv::Mat& img) const;

private:

	bool checkInput() const override;

	cv::Mat costs(int numLabels) const override;
	cv::Mat labelDistMatrix(int numLabels) const override;
};


};