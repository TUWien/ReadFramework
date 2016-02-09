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

/**
* The class binarize a grayvalue image. The segmentation algorithm is based on
* "Binarization of Historical Document Images Using Local Maximum and Minimum", Bolan Su, Shijian Lu and Chew Lim Tan, DAS 2010.
**/
class DllModuleExport BaseBinarizationSu : public Module {

public:
	BaseBinarizationSu(const cv::Mat& img, const cv::Mat& mask = cv::Mat());
	bool isEmpty() const override;
	virtual bool compute() override;

	cv::Mat binaryImage() const;

	virtual QString toString() const override;

protected:
	cv::Mat compContrastImg(const cv::Mat& srcImg, const cv::Mat& mask) const;
	cv::Mat compBinContrastImg(const cv::Mat& contrastImg) const;
	virtual float contrastVal(const unsigned char* maxVal, const unsigned char * minVal) const;
	virtual void calcFilterParams(int &filterS, int &Nm);
	virtual float strokeWidth(const cv::Mat& contrastImg) const;
	float thresholdVal(float *mean, float *std) const;
	void computeDistHist(const cv::Mat& src, QList<int> *maxDiffList, QList<float> *localIntensity, float gSigma) const;
	void computeThrImg(const cv::Mat& grayImg32F, const cv::Mat& binContrast, cv::Mat& thresholdImg, cv::Mat& thresholdContrastPxImg);
	bool checkInput() const override;
	//void compThrImg();
	//void compDisHist();

	cv::Mat mSrcImg;									//the input image  either 3 channel or 1 channel [0 255]
	cv::Mat mBwImg;										//the binarized image [0 255]
	cv::Mat mMask;										//the mask image [0 255]
														//cv::Mat mContrastImg;
														//cv::Mat mBinContrastImg;
														//cv::Mat mThrImg;

														//parameters
	int mErodeMaskSize = 3 * 6;							//size for the boundary erosion
	float mStrokeW = 4;									//estimated strokeWidth

private:

	
	void load(const QSettings& settings) override;
	void save(QSettings& settings) const override;


};

/**
* The class binarize a grayvalue image. The segmentation algorithm is based on
* "Binarization of Historical Document Images Using Local Maximum and Minimum", Bolan Su, Shijian Lu and Chew Lim Tan, DAS 2010.
* In contrast to DkSegmentationSu the ContrastImg is adapted and the strokeWidth is constant.
**/
class DllModuleExport BinarizationSuAdapted : public BaseBinarizationSu {

public:
	BinarizationSuAdapted(const cv::Mat& img, const cv::Mat& mask = cv::Mat()) : BaseBinarizationSu(img, mask) { mModuleName = "BaseBinarizationAdapted"; };
	virtual bool compute() override;
	virtual QString toString() const override;
	void setFiltering(bool medianFilter) { mMedianFilter = medianFilter; };

protected:
	float setStrokeWidth(float strokeW);
	virtual float contrastVal(const unsigned char* maxVal, const unsigned char * minVal) const override;
	virtual void calcFilterParams(int &filterS, int &Nm) override;

	cv::Mat mContrastImg;
	cv::Mat mBinContrastImg;
	cv::Mat mThrImg;
private:
	bool mMedianFilter = true;
};


/**
* The class binarize a rgb color image. To make the algorithm robust against noise,
* the foreground is estimated and the image is weighted with the foreground.
* (Foreground estimation is performed by a Mean Filter with a size of 32x32 px)
**/
class DllModuleExport BinarizationSuFgdWeight : public BinarizationSuAdapted {

public:
	BinarizationSuFgdWeight(const cv::Mat& img, const cv::Mat& mask = cv::Mat()) : BinarizationSuAdapted(img, mask) { mModuleName = "BinarizationSuFgdWeight"; };
	virtual bool compute() override;
	//virtual QString toString() const override;

protected:
	cv::Mat computeFgd() const;
	cv::Mat computeMeanFgdEst(const cv::Mat& grayImg32F, const cv::Mat& mask) const;
	cv::Scalar computeConfidence() const;
	//virtual void init();
	virtual void weightFunction(cv::Mat& grayImg, cv::Mat& thrImg, const cv::Mat& mask);

	int mFgdEstFilterSize = 32;								//the filter size for the foreground estimation
	float mSigmSlope = 15.0f;
	cv::Mat mFgdEstImg;
	cv::Scalar mMeanContrast = cv::Scalar(-1.0f,-1.0f,-1.0f,-1.0f);
	cv::Scalar mStdContrast = cv::Scalar(-1.0f,-1.0f,-1.0f,-1.0f);
	float mConfidence = -1.0f;

private:
};



}
