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

namespace rdf {

// read defines

	class DllModuleExport BasicFM {

	public:
		BasicFM();
		BasicFM(const cv::Mat& img);

		virtual double compute() = 0;

		void setImg(const cv::Mat& img);
		double val() const;
		void setWindowSize(int s);
		int windowSize() const;

	protected:
		cv::Mat mSrcImg;

		// parameters
		double mVal = -1.0;
		int mWindowSize = 15;

	};

	class DllModuleExport BrennerFM : public BasicFM {

	public:
		BrennerFM();
		BrennerFM(const cv::Mat& img);

		double compute() override;

	};

	class DllModuleExport GlVaFM : public BasicFM {

	public:
		GlVaFM();
		GlVaFM(const cv::Mat& img);

		double compute() override;
	};

	class DllModuleExport GlLvFM : public BasicFM {

	public:
		GlLvFM();
		GlLvFM(const cv::Mat& img);

		double compute() override;
	};


	class DllModuleExport GlVnFM : public BasicFM {

	public:
		GlVnFM();
		GlVnFM(const cv::Mat& img);

		double compute() override;
	};

	class DllModuleExport GraTFM : public BasicFM {

	public:
		GraTFM();
		GraTFM(const cv::Mat& img);

		double compute() override;
	};

	class DllModuleExport GraSFM : public BasicFM {

	public:
		GraSFM();
		GraSFM(const cv::Mat& img);

		double compute() override;
	};


};