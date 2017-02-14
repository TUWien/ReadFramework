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

namespace rdf {

// read defines
class Region;
class Polygon;
class TextLine;

class DllCoreExport LayoutAnalysisConfig : public ModuleConfig {

public:
	LayoutAnalysisConfig();

	enum ScaleSideMode {
		scale_max_side = 0,		// scales w.r.t to the max side usefull if you have free images
		scale_height,			// [default] choose this if you now that you have pages & double pages

		scale_end
	};

	virtual QString toString() const override;

	void setMaxImageSide(int maxSide);
	int maxImageSide() const;

	void setScaleMode(const ScaleSideMode& mode);
	int scaleMode() const;

protected:

	void load(const QSettings& settings) override;
	void save(QSettings& settings) const override;

	int mMaxImageSide = 3000;
	int mScaleMode = scale_height;
};

class DllCoreExport LayoutAnalysis : public Module {

public:
	LayoutAnalysis(const cv::Mat& img);

	bool isEmpty() const override;
	bool compute() override;
	QSharedPointer<LayoutAnalysisConfig> config() const;

	cv::Mat draw(const cv::Mat& img) const;
	QString toString() const override;

	void setRootRegion(const QSharedPointer<Region>& region);
	TextBlockSet textBlockSet() const;

	double scaleFactor() const;

private:
	bool checkInput() const override;

	// input
	cv::Mat mImg;
	QSharedPointer<Region> mRoot;

	// params
	double mScale = 1.0;

	// output
	TextBlockSet mTextBlockSet;

	TextBlockSet createTextBlocks() const;
	QVector<Line> createStopLines() const;

};


};