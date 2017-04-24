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
#include "Shapes.h"

#pragma warning(push, 0)	// no warnings from includes
// Qt Includes
#include <QVector>
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



class DllCoreExport LineFilterConfig : public ModuleConfig {

public:
	LineFilterConfig();

	double maxSlopeRotat() const;
	void setMaxSlopeRotat(double s);

	int minLength() const;
	void setMinLength(int l);

	double maxGap() const;
	void setMaxGap(double g);

	double maxAngleDiff() const;
	void setMaxAngleDiff(double a);

	QString toString() const override;

private:
	void load(const QSettings& settings) override;
	void save(QSettings& settings) const override;

	double mMaxSlopeRotat = 10.0;	//filter parameter: maximal difference of line orientation compared to the result of the Rotation module (default: 10°)
	int mMinLength = 100;			//filter parameter: remove lines which are smaller (default: 100)
	double mMaxGap = 100;			//filter parameter: maximal gap between two lines in pixel (default: 100)
	double mMaxAngleDiff = 2.0;		//filter parameter: maximal angle difference between two compared and the inserted line (default: 2.0)
};

class DllCoreExport LineFilter {

public:
	LineFilter();

	QVector<rdf::Line> filterLineAngle(const QVector<rdf::Line>& lines, double angle, double angleDiff = DBL_MAX) const;
	QVector<rdf::Line> mergeLines(const QVector<rdf::Line>& lines, QVector<rdf::Line>* gaps = 0, double maxGap = DBL_MAX, double maxAngleDiff = DBL_MAX) const;
	QVector<rdf::Line> removeSmall(const QVector<rdf::Line>& lines, int minLineLength = 0) const;

	QSharedPointer<LineFilterConfig> config() const;

protected:

	QSharedPointer<LineFilterConfig> mConfig = 0;
};


class DllCoreExport LineTraceConfig : public ModuleConfig {

public:
	LineTraceConfig();

	int minWidth() const;
	void setMinWidth(int w);

	double maxLenDiff() const;
	void setMaxLenDiff(double l);

	int maxLen() const;
	void setMaxLen(int l);

	int minLenSecondRun() const;
	void setMinLenSecondRun(int r);

	float maxDistExtern() const;
	void setMaxDistExtern(float d);

	float maxAngleDiffExtern() const;
	void setMaxAngleDiffExtern(float a);

	double maxAspectRatio() const;
	void setMaxAspectRatio(double a);

	QString toString() const override;

private:
	void load(const QSettings& settings) override;
	void save(QSettings& settings) const override;

	int mMinLenSecondRun = 60;    //min len to filter after merge lines; was 60
	double mMaxLenDiff = 1.5;		//filter parameter: maximal difference in length between two successive runlengths (default: 1.5)
	double mMaxAspectRatio = 0.3;	//filter parameter: maximal aspect ratio of a line (default: 0.3f)
	int mMinWidth = 20;				//filter parameter: minimal width a line in pixel (default: 30)

	//filter Lines parameter (compared to given line vector, see std::vector<DkLineExt> filterLines(std::vector<DkLineExt> &externLines))
	int mMaxLen = 20;				//filter parameter: maximal length of a line in pixel (default: 20)
	float mMaxDistExtern = 10.0f;		//maximal Distance of the external line end points compared to a given line (default: 5 pixel)
	float mMaxAngleDiffExtern = 20.0f / 180.0f * (float)CV_PI;	//maximal Angle Difference of the external line compared to a given line (default: 20 deg)
};

/// <summary>
/// Detects Lines in a binary image. The result is a binary image containing all line elements (pixel accurate) as well as all lines as vectors.
///	It is also possible to filter lines according a specified angle.
/// </summary>
/// <seealso cref="Module" />
class DllCoreExport LineTrace : public Module {

public:
	LineTrace(const cv::Mat& img, const cv::Mat& mask = cv::Mat());

	bool isEmpty() const override;
	virtual bool compute() override;
	QVector<rdf::Line> getHLines() const;
	QVector<rdf::Line> getVLines() const;
	QVector<rdf::Line> getLines() const;
	void setAngle(double angle = std::numeric_limits<double>::infinity());
	void resetAngle();

	QSharedPointer<LineTraceConfig> config() const;

	cv::Mat lineImage() const;
	cv::Mat generatedLineImage() const;
	static void generateLineImage(const QVector<rdf::Line>& hline, const QVector<rdf::Line>& vline, cv::Mat& img, cv::Scalar hCol = cv::Scalar(255), cv::Scalar vCol = cv::Scalar(255), cv::Point2d offset = cv::Point(0,0));
	virtual QString toString() const override;

protected:
	bool checkInput() const override;

	cv::Mat mSrcImg;									//the input image  either 3 channel or 1 channel [0 255]
	cv::Mat mLineImg;									//the line image [0 255]
	cv::Mat mMask;										//the mask image [0 255]

	QVector<rdf::Line> hLines;
	QVector<rdf::Line> vLines;

	LineFilter mLineFilter;

private:

	double mAngle = std::numeric_limits<double>::infinity();		//filter parameter: angle of the snippet determined by the skew estimation (default: 0.0f)

	float mLineProb;
	float mLineDistProb;

	cv::Mat hDSCC(const cv::Mat& bwImg) const;
	void filter(cv::Mat& hDSCCImg, cv::Mat& vDSCCImg);
	void filterLines();
	void drawGapLines(cv::Mat& img, QVector<rdf::Line> lines);
};

class DllCoreExport ReadLSDConfig : public ModuleConfig {

public:
	ReadLSDConfig();

	double scale() const;
	void setScale(double s);

	double sigmaScale() const;
	void setSigmaScale(double s);

	double angleThr() const;
	void setAngleThr(double a);

	double logEps() const;
	void setLogeps(double l);

	double density() const;
	void setDensity(double d);

	int bins() const;
	void setBins(int b);

	double quant() const;
	void setQuant(double q);


	QString toString() const override;

private:
	void load(const QSettings& settings) override;
	void save(QSettings& settings) const override;

	double mScale = 1.0f;		// When different from 1.0, LSD will scale the input image
								//by 'scale' factor by Gaussian filtering, before detecting	line segments.
								//Example: if scale = 0.8, the input image will be subsampled
								//to 80 % of its size, before the line segment detector
								//is applied.	Suggested value : 0.8
	double mSigmaScale = 0.6f;	//When scale != 1.0, the sigma of the Gaussian filter is :
								//sigma = sigma_scale / scale, if scale <  1.0
								//sigma = sigma_scale, if scale >= 1.0
								//Suggested value : 0.6
	double mAngleThr = 22.5f;	// Gradient angle tolerance in the region growing  algorithm, in degrees.
								//Suggested value : 22.5
	double mLogEps = 0.0f; 		//Detection threshold, accept if - log10(NFA) > log_eps.
								//	The larger the value, the more strict the detector is,
								//	and will result in less detections.
								//	(Note that the 'minus sign' makes that this
								//		behavior is opposite to the one of NFA.)
								//	The value - log10(NFA) is equivalent but more
								//	intuitive than NFA :
								//--1.0 gives an average of 10 false detections on noise
								//	- 0.0 gives an average of 1 false detections on noise
								//	- 1.0 gives an average of 0.1 false detections on nose
								//	- 2.0 gives an average of 0.01 false detections on noise
								//	.
								//	Suggested value : 0.0
	double mDensityThr = 0.7f;	//Minimal proportion of 'supporting' points in a rectangle.	Suggested value : 0.7
	int mNBins = 1024;			//Number of bins used in the pseudo-ordering of gradient modulus. Suggested value : 1024
	double mQuant = 2.0f / 255.0f; //Bound to the quantization error on the gradient norm.
									//Example: if gray levels are quantized to integer steps,
									//the gradient(computed by finite differences) error
									//due to quantization will be bounded by 2.0, as the
									//worst case is when the error are 1 and -1, that
									//gives an error of 2.0.
									//Suggested value : 2.0
									//normalize -> 2.0/255.0


};


class DllCoreExport ReadLSD : public Module {

public:
	ReadLSD(const cv::Mat& img, const cv::Mat& mask = cv::Mat());

	bool isEmpty() const override;
	virtual bool compute() override;
	cv::Mat magImg() const;
	cv::Mat radImg() const;
	cv::Mat regImg() const;
	QVector<rdf::LineSegment> lines() const;
	QSharedPointer<ReadLSDConfig> config() const;

	//void setMaxAspectRatio(float ratio);
	virtual QString toString() const override;

protected:
	bool checkInput() const override;

	cv::Mat mSrcImg;									//the input image  either 3 channel or 1 channel [0 255]
	cv::Mat mLineImg;									//the line image [0 255]
	cv::Mat mRegionImg;
	cv::Mat mMagImg;
	cv::Mat mRadImg;
	cv::Mat mMask;										//the mask image [0 255]
	
	int mTabSize = 100000;
	double mInv[100000];   /* table to keep computed inverse values */
	QVector<rdf::Line> hLines;
	QVector<rdf::Line> vLines;
	QVector<rdf::LineSegment> mLineSegments;

private:
	double nfa(int n, int k, double p, double logNT);
	double mAngle = std::numeric_limits<double>::infinity();		//filter parameter: angle of the snippet determined by the skew estimation (default: 0.0f)
	bool isAligned(double thetaTest, double theta, double prec);
	bool isAligned(int x, int y, const cv::Mat& img, double theta, double prec);
	double regionGrow(int x, int y, QVector<cv::Point> &region, int regionIdx, double thr, double prec); //thr is magnitude threshold
	rdf::LineSegment region2Rect(QVector<cv::Point> &region, const cv::Mat& magImg, double angle, double prec, double p);
	double getTheta(QVector<cv::Point> &region, const cv::Mat& magImg, double angle, double prec, double x, double y);
	bool doubleEqual(double a, double b);
	bool refine(LineSegment& l, QVector<cv::Point> &region, const cv::Mat& magImg, const cv::Mat& radImg, double densityThr, double thr, double prec, double p); //thr is magnitude threshold
	bool reduceRegionRadius(LineSegment& l, QVector<cv::Point> &region, const cv::Mat& magImg, double prec, double p, double densityThr);
	double rectImprove(rdf::LineSegment& l, cv::Mat& radImg, double logNT, double logEps);
	double rectNfa(rdf::LineSegment& l, cv::Mat& radImg, double logNT);

};

class DllCoreExport LineTraceLSDConfig : public ModuleConfig {

public:
	LineTraceLSDConfig();

	void setScale(double scale);
	double scale() const;

	QString toString() const override;

private:
	void load(const QSettings& settings) override;
	void save(QSettings& settings) const override;

	double mScale = 0.5;	// initial downscaling of the image
};

/// <summary>
/// Detect lines using the LSD algorithm.
/// This line finder implements the LSD
/// method from OpenCV contrib. 
/// </summary>
/// <seealso cref="Module" />
class DllCoreExport LineTraceLSD : public Module {

public:
	LineTraceLSD(const cv::Mat& img);

	bool isEmpty() const override;
	virtual bool compute() override;
	
	QVector<Line> lines() const;
	QVector<Line> separatorLines() const;
	
	QSharedPointer<LineTraceLSDConfig> config() const;
	LineFilter lineFilter() const;

	virtual QString toString() const override;

	cv::Mat draw(const cv::Mat& img) const;

protected:
	cv::Mat mImg;
	QVector<Line> mLines;
	LineFilter mLineFilter;

	bool checkInput() const override;
};

};