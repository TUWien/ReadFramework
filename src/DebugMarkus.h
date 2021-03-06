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

 You should have received a copy of the GNU General Public Licensex
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

#include "DebugUtils.h"

#pragma warning(push, 0)	// no warnings from includes
// Qt Includes
#pragma warning(pop)

// Qt/CV defines
namespace cv {
	class Mat;
}

namespace rdf {

// read defines

class XmlTest {

public:
	XmlTest(const DebugConfig& config = DebugConfig());

	void parseXml();
	void linesToXml();

protected:
	DebugConfig mConfig;
};

class LayoutTest {
	
public:
	LayoutTest(const DebugConfig& config = DebugConfig());

	void testComponents();
	void layoutToXml() const;
	void layoutToXmlDebug() const;

protected:
	void testFeatureCollector(const cv::Mat& src) const;
	void testTrainer();
	void testClassifier(const cv::Mat& src) const;
	void testLineDetector(const cv::Mat& src) const;

	void testLayout(const cv::Mat& src) const;
	void pageSegmentation(const cv::Mat& src) const;

	double scaleFactor(const cv::Mat& img) const;
	void eval() const;
	void eval(const QString& toolPath, const QString& gtPath, const QString& resultPath) const;

	DebugConfig mConfig;
};

class DeepMergeTest {

public:
	DeepMergeTest(const DebugConfig& config = DebugConfig());

	void run();

protected:

	void merge(const cv::Mat& src) const;
	cv::Mat thresh(const cv::Mat& src, double thr = 0.5) const;

	DebugConfig mConfig;
};

}
