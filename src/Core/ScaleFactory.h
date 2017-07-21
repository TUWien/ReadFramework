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
#include "Image.h"
#include "Shapes.h"

#pragma warning(push, 0)	// no warnings from includes
// Qt Includes
#pragma warning(pop)

#ifndef DllCoreExport
#ifdef DLL_CORE_EXPORT
#define DllCoreExport Q_DECL_EXPORT
#else
#define DllCoreExport Q_DECL_IMPORT
#endif
#endif

// Qt defines

namespace rdf {

// read defines
class BaseElement;

/// <summary>
/// Class that configures the ScaleFactory
/// </summary>
/// <seealso cref="ModuleConfig" />
class DllCoreExport ScaleFactoryConfig : public ModuleConfig {

public:
	enum ScaleSideMode {
		scale_max_side = 0,		// scales w.r.t to the max side usefull if you have free images
		scale_height,			// [default] choose this if you now that you have pages & double pages

		scale_end
	};
	
	ScaleFactoryConfig();

	virtual QString toString() const override;

	void setMaxImageSide(int maxSide);
	int maxImageSide() const;

	void setScaleMode(const ScaleFactoryConfig::ScaleSideMode& mode);
	ScaleFactoryConfig::ScaleSideMode scaleMode() const;

	int dpi() const;

protected:

	void load(const QSettings& settings) override;
	void save(QSettings& settings) const override;

	// 1000px == 100dpi @ A4
	int mMaxImageSide = 1000;				// maximum image side in px (larger images are downscaled accordingly)
	int mDpi = 300;							// estimated dpi (even better if we truely know it : )
	ScaleFactoryConfig::ScaleSideMode mScaleMode = ScaleFactoryConfig::scale_height;	// scaling mode (see ScaleSideMode)
};

class ScaleFactory {

public:
	static ScaleFactory& instance();

	static double scaleFactor();
	static double scaleFactorDpi();
	static cv::Mat scaled(cv::Mat& img);
	static void scale(BaseElement& el);
	static void scaleInv(BaseElement& el);

	static Vector2D imgSize();

	QSharedPointer<ScaleFactoryConfig> config() const;

	void init(const Vector2D& imgSize);

private:
	ScaleFactory();
	ScaleFactory(const ScaleFactory&);

	Vector2D mImgSize;
	double mScaleFactor = 1.0;

	QSharedPointer<ScaleFactoryConfig> mConfig;

	double scaleFactor(const Vector2D& size, int maxImageSize, const ScaleFactoryConfig::ScaleSideMode& mode) const;
};


}