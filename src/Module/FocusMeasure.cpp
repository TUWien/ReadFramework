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

#include "FocusMeasure.h"

#pragma warning(push, 0)	// no warnings from includes
#include "opencv2/imgproc/imgproc.hpp"
#pragma warning(pop)

namespace rdf {


	BasicFM::BasicFM()
	{
	}

	BasicFM::BasicFM(const cv::Mat & img)
	{
		mSrcImg = img;
	}

	double BasicFM::computeBREN()
	{
		if (mSrcImg.empty())
			return mVal;

		if (mSrcImg.depth() != CV_64F)
			mSrcImg.convertTo(mSrcImg, CV_64F);

		if (mSrcImg.channels() != 1)
			cv::cvtColor(mSrcImg, mSrcImg, CV_RGB2GRAY);

		cv::Mat dH = mSrcImg(cv::Range::all(), cv::Range(2, mSrcImg.cols)) - mSrcImg(cv::Range::all(), cv::Range(0, mSrcImg.cols - 2));
		cv::Mat dV = mSrcImg(cv::Range(2, mSrcImg.rows), cv::Range::all()) - mSrcImg(cv::Range(0, mSrcImg.rows - 2), cv::Range::all());
		dH = cv::abs(dH);
		dV = cv::abs(dV);

		cv::Mat FM = cv::max(dH, dV);
		FM = FM.mul(FM);

		cv::Scalar fm = cv::mean(FM);
		mVal = fm[0];

		return mVal;
	}

	double BasicFM::computeGLVA()
	{
		if (mSrcImg.empty())
			return mVal;

		if (mSrcImg.depth() != CV_64F)
			mSrcImg.convertTo(mSrcImg, CV_64F);

		if (mSrcImg.channels() != 1)
			cv::cvtColor(mSrcImg, mSrcImg, CV_RGB2GRAY);


		cv::Scalar m, v;
		cv::meanStdDev(mSrcImg, m, v);
		mVal = v[0];


		return mVal;
	}

	double BasicFM::computeGLVN()
	{
		if (mSrcImg.empty())
			return mVal;

		if (mSrcImg.depth() != CV_64F)
			mSrcImg.convertTo(mSrcImg, CV_64F);

		if (mSrcImg.channels() != 1)
			cv::cvtColor(mSrcImg, mSrcImg, CV_RGB2GRAY);

		cv::Scalar m, v;
		cv::meanStdDev(mSrcImg, m, v);
		mVal = v[0] * v[0] / (m[0] * m[0] + std::numeric_limits<double>::epsilon());

		return mVal;
	}

	double BasicFM::computeGLLV()
	{
		if (mSrcImg.empty())
			return mVal;

		if (mSrcImg.depth() != CV_64F)
			mSrcImg.convertTo(mSrcImg, CV_64F);

		if (mSrcImg.channels() != 1)
			cv::cvtColor(mSrcImg, mSrcImg, CV_RGB2GRAY);

		cv::Mat sqdImg = mSrcImg.mul(mSrcImg);
		cv::Mat meanImg, meanSqdImg;
		cv::boxFilter(mSrcImg, meanImg, CV_64FC1, cv::Size(mWindowSize, mWindowSize));
		cv::boxFilter(sqdImg, meanSqdImg, CV_64FC1, cv::Size(mWindowSize, mWindowSize));

		cv::Mat localStdImg = meanSqdImg - meanImg.mul(meanImg);
		localStdImg = localStdImg.mul(localStdImg);

		cv::Scalar m, v;
		cv::meanStdDev(localStdImg, m, v);
		mVal = v[0] * v[0];


		return mVal;
	}

	double BasicFM::computeGRAT()
	{
		if (mSrcImg.empty())
			return mVal;

		if (mSrcImg.depth() != CV_64F)
			mSrcImg.convertTo(mSrcImg, CV_64F);

		if (mSrcImg.channels() != 1)
			cv::cvtColor(mSrcImg, mSrcImg, CV_RGB2GRAY);

		cv::Mat dH = mSrcImg(cv::Range::all(), cv::Range(1, mSrcImg.cols)) - mSrcImg(cv::Range::all(), cv::Range(0, mSrcImg.cols - 1));
		cv::Mat dV = mSrcImg(cv::Range(1, mSrcImg.rows), cv::Range::all()) - mSrcImg(cv::Range(0, mSrcImg.rows - 1), cv::Range::all());
		dH = cv::abs(dH);
		dV = cv::abs(dV);

		cv::Mat FM = cv::max(dH, dV);
		double thr = 0;
		cv::Mat mask = FM >= thr;
		mask.convertTo(mask, CV_64FC1, 1 / 255.0);

		FM = FM.mul(mask);

		cv::Scalar fm = cv::sum(FM) / cv::sum(mask);

		mVal = fm[0];

		return mVal;
	}

	double BasicFM::computeGRAS()
	{

		cv::Mat dH = mSrcImg(cv::Range::all(), cv::Range(1, mSrcImg.cols)) - mSrcImg(cv::Range::all(), cv::Range(0, mSrcImg.cols - 1));
		dH = dH.mul(dH);

		cv::Scalar fm = cv::mean(dH);
		mVal = fm[0];

		return mVal;
	}

	void BasicFM::setImg(const cv::Mat & img)
	{
		mSrcImg = img;
	}

	double BasicFM::val() const
	{
		return mVal;
	}

	void BasicFM::setWindowSize(int s)
	{
		mWindowSize = s;
	}

	int BasicFM::windowSize() const
	{
		return mWindowSize;
	}


	FocusEstimation::FocusEstimation()
	{
	}

	FocusEstimation::FocusEstimation(const cv::Mat & img)
	{
		mSrcImg = img;
	}

	double FocusEstimation::compute()
	{
		return 0.0;
	}

	void FocusEstimation::setImg(const cv::Mat & img)
	{
		mSrcImg = img;
	}

	void FocusEstimation::setWindowSize(int s)
	{
		mWindowSize = s;
	}

	int FocusEstimation::windowSize() const
	{
		return mWindowSize;
	}

}