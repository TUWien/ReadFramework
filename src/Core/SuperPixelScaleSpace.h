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

#include "SuperPixel.h"

#include "Image.h"
#include "ImageProcessor.h"
#include "Settings.h"

#pragma warning(push, 0)	// no warnings from includes
#include <opencv2/core/core.hpp>
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

class DllCoreExport ScaleSpaceSPConfig : public ModuleConfig {

public:
	ScaleSpaceSPConfig();

	virtual QString toString() const override;

	int numLayers() const;
	int minLayer() const;

protected:
	int mNumLayers = 3;
	int mMinLayer = 0;

	void load(const QSettings& settings) override;
	void save(QSettings& settings) const override;
};

/// <summary>
/// Creates a scale space and
/// runs the SuperPixelModule on each scale.
/// Use this class to extract SuperPixels
/// that are robust w.r.t. scale changes.
/// </summary>
template <class SuperPixelModule>
class DllCoreExport ScaleSpaceSuperPixel : public SuperPixelBase {
	static_assert(std::is_base_of<SuperPixelBase, SuperPixelModule>::value, "T must derive from SuperPixelBase");	// requires C++11

public:
	ScaleSpaceSuperPixel(const cv::Mat & img) : SuperPixelBase(img) {
		mConfig = QSharedPointer<ScaleSpaceSPConfig>::create();
	}

	bool compute() override {

		if (!checkInput())
			return false;

		Timer dt;

		cv::Mat img = mSrcImg.clone();
		img = IP::grayscale(img);
		cv::normalize(img, img, 255, 0, cv::NORM_MINMAX);

		Config::instance().global().setNumScales(config()->numLayers());

		int idCnt = 0;

		for (int idx = 0; idx < config()->numLayers(); idx++) {

			// compute super pixel
			if (config()->minLayer() <= idx) {

				SuperPixelModule spm(img);
				spm.setPyramidLevel(idx);
				qDebug() << "computing new layer...";

				// get super pixels of the current scale
				if (!spm.compute())
					mWarning << "could not compute super pixels for layer #" << idx;

				PixelSet set = spm.pixelSet();

				// assign the pyramid level
				for (auto p : set.pixels()) {
					p->setPyramidLevel(idx);
					// make ID unique for scale space
					p->setId(QString::number(idCnt));
					idCnt++;
				}

				if (idx > 0) {

					// re-scale
					double sf = std::pow(2, idx);
					set.scale(sf);
				}

				mSet += set;
			}

			cv::resize(img, img, cv::Size(), 0.5, 0.5, CV_INTER_AREA);
		}

		// filter from all scales
		mSet.filterDuplicates();
		mInfo << mSet.size() << "pixels extracted in" << dt;

		return true;
	}

	cv::Mat draw(const cv::Mat & img) const {

		QImage qImg = Image::mat2QImage(img, true);
		QPainter p(&qImg);

		p.setPen(ColorManager::blue());

		for (auto px : mSet.pixels()) {
			px->draw(p, 0.3, Pixel::DrawFlags() | /*Pixel::draw_id |*/ Pixel::draw_center | Pixel::draw_stats);
		}

		return Image::qImage2Mat(qImg);
	}

	QString toString() const override {
		return config()->toString();
	}

	QSharedPointer<ScaleSpaceSPConfig> config() const {
		return qSharedPointerDynamicCast<ScaleSpaceSPConfig>(mConfig);
	}

};

// NOTE: we need this instantiation for the DllExport... 
// -> you have to add each SuperPixel module that you want to export 
// or you get an unresolved external
template class ScaleSpaceSuperPixel<SuperPixel>;
template class ScaleSpaceSuperPixel<GridSuperPixel>;


}