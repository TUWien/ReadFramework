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

 You should have received a copy of the GNU General Public License
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

#include "BaseModule.h"

#include "PixelSet.h"

#pragma warning(push, 0)	// no warnings from includes
// Qt Includes
#include <QObject>
#include <QVector>
#include <QVector4D>

#include <opencv2/core.hpp>
#pragma warning(pop)

#pragma warning (disable: 4251)	// inlined Qt functions in dll interface

#ifndef DllCoreExport
#ifdef DLL_CORE_EXPORT
#define DllCoreExport Q_DECL_EXPORT
#else
#define DllCoreExport Q_DECL_IMPORT
#endif
#endif

// Qt defines

namespace rdf {


	class DllCoreExport BaseSkewEstimationConfig : public ModuleConfig {

	public:
		BaseSkewEstimationConfig();

		int width() const;
		void setWidth(int w);

		int height() const;
		void setHeight(int h);

		int delta() const;
		void setDelta(int d);

		int epsilon() const;
		void setEpsilon(int e);

		int minLineLength() const;
		void setMinLineLength(int l);

		//int minLineProjLength() const;
		//void setMinLineProjLength(int l);

		double sigma() const;
		void setSigma(double s);

		double thr() const;
		void setThr(double t);

		int kMax() const;
		void setKMax(int k);

		int nIter() const;
		void setNIter(int n);

		QString toString() const override;

	private:
		void load(const QSettings& settings) override;
		void save(QSettings& settings) const override;

		int mW = 168; //49 according to the paper, best for document: 60
		int mH = 56; //12 according to the paper, best for document: 28
		int mDelta = 20; //according to the paper
		int mEpsilon = 2; //according to the paper

		int mMinLineLength = 50;
		//int mMinLineProjLength = 50 / 4;
		double mSigma = 0.3; //according to the paper, best for document: 0.5
		//double mThr = 0.1; //according to the paper
		double mThr = 4.0; //according to the paper

		int mKMax = 7; //according to the paper
		int mNIter = 200; //according to the paper
	};


/// <summary>
/// The class estimates the skew of a document page. The methodology is based on
/// "Skew estimation of natural images based on a salient line detector", Hyung Il Koo and Nam Ik Cho
/// </summary>
/// <seealso cref="Module" />
	class DllCoreExport BaseSkewEstimation : public Module {

	public:
		enum EdgeDirection { HORIZONTAL = 0, VERTICAL };

		BaseSkewEstimation(const cv::Mat& img = cv::Mat(), const cv::Mat& mask = cv::Mat());
		void setImages(const cv::Mat& img, const cv::Mat& mask = cv::Mat());

		bool compute();
		double getAngle();
		QVector<QVector4D> getSelectedLines() const;

		bool isEmpty() const override;
		virtual QString toString() const override;

		void setFixedThr(bool f);

		QSharedPointer<BaseSkewEstimationConfig> config() const;

	protected:
		cv::Mat mSrcImg;									//the input image  either 3 channel or 1 channel [0 255]
		cv::Mat mMask;										//the mask image [0 255]

		cv::Mat separability(const cv::Mat& srcImg, int w, int h, const cv::Mat& mask = cv::Mat());
		cv::Mat edgeMap(const cv::Mat& separability, double thr, EdgeDirection direction = HORIZONTAL, const cv::Mat& mask = cv::Mat()) const;
		QVector<QVector3D> computeWeights(cv::Mat edgeMap, int delta, int epsilon, EdgeDirection direction = HORIZONTAL);
		//according to paper eta should be 0.5
		double skewEst(const QVector<QVector3D>& weights, double imgDiagonal, bool& ok, double eta=0.35);
		bool checkInput() const override;

	private:

		double mSkewAngle = 0.0;
		//double mWeightEps = 0.5;
		int mRotationFactor = 1; //needed if we want to transpose the image in the beginning...
		bool mFixedThr = true;

		cv::Mat mIntegralImg;
		cv::Mat mIntegralSqdImg;
		
		QVector<QVector4D> mSelectedLines;
		QVector<int> mSelectedLineTypes;

	};

	class DllCoreExport TextLineSkewConfig : public ModuleConfig {

	public:
		TextLineSkewConfig();

		virtual QString toString() const override;

		double minAngle() const;
		double maxAngle() const;

	protected:

		void load(const QSettings& settings) override;
		void save(QSettings& settings) const override;

		double mMinAngle = -CV_PI;			// minimum angle expected in radians
		double mMaxAngle = CV_PI;		// maximum angle expected in radians
	};


	class DllCoreExport TextLineSkew : public Module {

	public:
		TextLineSkew(const cv::Mat& img = cv::Mat());

		bool compute();
		
		double getAngle();
		
		bool isEmpty() const override;
		virtual QString toString() const override;

		QSharedPointer<TextLineSkewConfig> config() const;

		cv::Mat draw(const cv::Mat& img) const;
		cv::Mat rotated(const cv::Mat& img) const;

	protected:
		cv::Mat mImg;			//the input image  either 3 channel or 1 channel [0 255]

		PixelSet mSet;
		QSharedPointer<ScaleFactory> mScaleFactory;

		double mAngle = 0.0;

		bool checkInput() const override;
	};




}
