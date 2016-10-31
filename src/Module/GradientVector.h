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

#ifndef DllModuleExport
#ifdef DLL_MODULE_EXPORT
#define DllModuleExport Q_DECL_EXPORT
#else
#define DllModuleExport Q_DECL_IMPORT
#endif
#endif

// Qt defines

namespace rdf {


class DllModuleExport GradientVectorConfig : public ModuleConfig {

	public:
		GradientVectorConfig();

		double sigma() const;
		void setSigma(double s);

		QString toString() const override;

	private:
		void load(const QSettings& settings) override;
		void save(QSettings& settings) const override;

		double mSigma = 1.75;	//filter parameter: maximal difference of line orientation compared to the result of the Rotation module (default: 5 deg)
};

class DllModuleExport GradientVector : public Module {


public:
	GradientVector(const cv::Mat& img, const cv::Mat& mask = cv::Mat());

	cv::Mat getDebugGaussImg();
	cv::Mat getMagImg(bool normGrad = true);
	cv::Mat getRadImg();

	bool isEmpty() const override;
	virtual bool compute() override;

	QSharedPointer<GradientVectorConfig> config() const;

	virtual QString toString() const override;

protected:
	bool checkInput() const override;


private:

	double mMinVal;
	double mMaxVal;

	cv::Mat mSrcImg;			// input image
	cv::Mat mMask;			// input mask
	cv::Mat mGaussImg;		// gaussian image 
	cv::Mat mDxImg;			// x derivative image
	cv::Mat mDyImg;			// y derivative image
	cv::Mat mMagImg;			// gradient magnitude image
	cv::Mat mRadImg;			// orientation image (in radians)

	/**
	 * Returns the smoothed image.
	 * @return a blurred 2D image 32F (the Gaussian derivative
	 * corresponds to sigma)
	 **/
	 /**
	  * Computes and returns the gradient magnitude image.
	  * The gradient magnitude is - in other words - the gradient
	  * vector's scale.
	  * @return the gradient magnitude of the given input image img
	  */
	  /**
	   * Computes and returns the gradient orientation image.
	   * Each pixel corresponds to the gradient angle in radians.
	   * @return the gradient orientation of the given input image img
	   */
};



};
