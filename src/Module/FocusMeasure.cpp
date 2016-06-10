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

		if (mSrcImg.empty())
			return mVal;

		if (mSrcImg.depth() != CV_64F)
			mSrcImg.convertTo(mSrcImg, CV_64F);

		if (mSrcImg.channels() != 1)
			cv::cvtColor(mSrcImg, mSrcImg, CV_RGB2GRAY);

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

		if (mSrcImg.depth() != CV_64F)
			mSrcImg.convertTo(mSrcImg, CV_64F);

		if (mSrcImg.channels() != 1)
			cv::cvtColor(mSrcImg, mSrcImg, CV_RGB2GRAY);
	}

	FocusEstimation::FocusEstimation(const cv::Mat & img, int wSize)
	{

		mSrcImg = img;

		if (mSrcImg.depth() != CV_64F)
			mSrcImg.convertTo(mSrcImg, CV_64F);

		if (mSrcImg.channels() != 1)
			cv::cvtColor(mSrcImg, mSrcImg, CV_RGB2GRAY);

		mWindowSize = wSize;
	}

	bool FocusEstimation::compute(FocusMeasure fm)
	{

		if (mSrcImg.empty())
			return false;

		BasicFM fmClass;
		double f;

		for (int row = 0; row < mSrcImg.rows; row += (mWindowSize+mSplitSize)) {
			for (int col = 0; col < mSrcImg.cols; col += (mWindowSize+mSplitSize)) {

				cv::Range rR(row, cv::min(row + mWindowSize, mSrcImg.rows));
				cv::Range cR(col, cv::min(col + mWindowSize, mSrcImg.cols));

				cv::Mat tile = mSrcImg(rR, cR);

				fmClass.setImg(tile);

				switch (fm)
				{
				case rdf::FocusEstimation::BREN:
					f = fmClass.computeBREN();
					break;
				case rdf::FocusEstimation::GLVA:
					f = fmClass.computeGLVA();
					break;
				case rdf::FocusEstimation::GLVN:
					f = fmClass.computeGLVN();
					break;
				case rdf::FocusEstimation::GLLV:
					f = fmClass.computeGLLV();
					break;
				case rdf::FocusEstimation::GRAT:
					f = fmClass.computeGRAT();
					break;
				case rdf::FocusEstimation::GRAS:
					f = fmClass.computeGRAS();
					break;
				default:
					f = -1;
					break;
				}
				
				Patch r(cv::Point(col, row), mWindowSize, mWindowSize, f);
				mFmPatches.push_back(r);
			}
		}

		return true;
	}

	std::vector<Patch> FocusEstimation::fmPatches() const
	{
		return mFmPatches;
	}

	void FocusEstimation::setImg(const cv::Mat & img)
	{
		mSrcImg = img;

		if (mSrcImg.depth() != CV_64F)
			mSrcImg.convertTo(mSrcImg, CV_64F);

		if (mSrcImg.channels() != 1)
			cv::cvtColor(mSrcImg, mSrcImg, CV_RGB2GRAY);
	}

	void FocusEstimation::setWindowSize(int s)
	{
		mWindowSize = s;
	}

	void FocusEstimation::setSplitSize(int s)
	{
		mSplitSize = s;
	}

	int FocusEstimation::windowSize() const
	{
		return mWindowSize;
	}

	Patch::Patch()
	{
	}

	Patch::Patch(cv::Point p, int w, int h, double f)
	{
		mUpperLeft = p;
		mWidth = w;
		mHeight = h;
		mFm = f;
	}

	void Patch::setPosition(cv::Point p, int w, int h)
	{
		mUpperLeft = p;
		mWidth = w;
		mHeight = h;
	}

	cv::Point Patch::upperLeft() const
	{
		return mUpperLeft;
	}

	int Patch::width() const
	{
		return mWidth;
	}

	int Patch::height() const
	{
		return mHeight;
	}

	double Patch::fm() const
	{
		return mFm;
	}

}