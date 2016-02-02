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

#pragma warning(push, 0)	// no warnings from includes
#include <QObject>

#include "opencv2/core/core.hpp"
#pragma warning(pop)


#pragma warning (disable: 4251)	// inlined Qt functions in dll interface

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
class DllModuleExport SimpleBinarization : public Module {

public:
	SimpleBinarization(const cv::Mat& img);
	
	bool isEmpty() const override;
	bool compute() override;

	cv::Mat binaryImage() const;

	//void setThresh(int thresh);
	//int thresh() const;

	QString toString() const override;

private:
	cv::Mat mSrcImg;
	cv::Mat mBwImg;

	// parameters
	int mThresh = 100;

	bool checkInput() const override;

	void load(const QSettings& settings) override;
	void save(QSettings& settings) const override;
};


class DllModuleExport BaseBinarizationSu : public Module {

public:
	BaseBinarizationSu(const cv::Mat& img, const cv::Mat& mask);
	bool isEmpty() const override;
	bool compute() override;

	cv::Mat binaryImage() const;

	QString toString() const override;

private:
	cv::Mat mSrcImg;									//the input image  either 3 channel or 1 channel [0 255]
	cv::Mat mBwImg;										//the binarized image [0 255]
	cv::Mat mMask;										//the mask image [0 255]
	cv::Mat mContrastImg;
	cv::Mat mBinContrastImg;
	cv::Mat mThrImg;

	//parameters
	int mErodeMaskSize = 3 * 6;							//size for the boundary erosion
	float mStrokeW = 5;									//estimated strokeWidth

	bool checkInput() const override;
	void load(const QSettings& settings) override;
	void save(QSettings& settings) const override;

	void compContrastImg();
	void compBinContrastImg();
	//void compThrImg();
	//void compDisHist();

};


}
