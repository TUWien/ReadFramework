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

#pragma warning(push, 0)	// no warnings from includes
#include <QSharedPointer>
#include <QSettings>

#include "opencv2/core/core.hpp"
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

// Qt defines
class QSettings;

namespace rdf {

// read defines
// read defines
class DllCoreExport Algorithms {

public:
	enum MorphShape { SQUARE = 0, DISK };
	enum MorphBorder { BORDER_ZERO = 0, BORDER_FLIP };

	static Algorithms& instance();
	cv::Mat dilateImage(const cv::Mat& bwImg, int seSize, MorphShape shape = Algorithms::SQUARE) const;
 	cv::Mat erodeImage(const cv::Mat& bwImg, int seSize, MorphShape shape = Algorithms::SQUARE) const;
	cv::Mat createStructuringElement(int seSize, int shape) const;
	cv::Mat convolveSymmetric(const cv::Mat& hist, const cv::Mat& kernel) const;
	cv::Mat get1DGauss(double sigma) const;
	cv::Mat threshOtsu(const cv::Mat& srcImg) const;
	cv::Mat convolveIntegralImage(const cv::Mat& src, const int kernelSizeX, const int kernelSizeY = 0, MorphBorder norm = BORDER_ZERO) const;

private:
	Algorithms();
	Algorithms(const Algorithms&);
};

};