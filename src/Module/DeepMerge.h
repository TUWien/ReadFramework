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
 [1] http://www.cvl.tuwien.ac.at/cvl/
 [2] https://transkribus.eu/Transkribus/
 [3] https://github.com/TUWien/
 [4] http://nomacs.org
 *******************************************************************************************************/

#pragma once

#include "BaseModule.h"
#include "BaseImageElement.h"
#include "PixelLabel.h"
#include "Shapes.h"

#pragma warning(push, 0)	// no warnings from includes
#include <QColor>
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

class DllCoreExport DeepMergeConfig : public ModuleConfig {

public:
	DeepMergeConfig();

	virtual QString toString() const override;

	QString labelConfigPath() const;

protected:

	QString mLabelConfigPath = "C:/nextcloud/READ/basilis/DeepMerge-config.json";

	void load(const QSettings& settings) override;
	void save(QSettings& settings) const override;
};

class DllCoreExport DMRegion : public BaseElement {

public:
	DMRegion(const QVector<Polygon>& poly = QVector<Polygon>(), const LabelInfo& l = LabelInfo());

	void operator<<(const Polygon& py) {
		mPoly << py;
	}

	void setRegions(const QVector<Polygon>& poly);
	QVector<Polygon> regions() const;

	LabelInfo label() const;

	void draw(QPainter& p);

private:
	QVector<Polygon> mPoly;
	LabelInfo mLabel;
};

class DllCoreExport DeepMerge : public Module {

public:
	DeepMerge(const cv::Mat& img);

	bool isEmpty() const override;
	bool compute() override;

	cv::Mat thresh(const cv::Mat& src, double thr) const;

	QSharedPointer<DeepMergeConfig> config() const;

	cv::Mat draw(const cv::Mat& img, const QColor& col = QColor()) const;
	QString toString() const override;

	cv::Mat image() const;

	void setScaleFactor(double sf);

private:
	bool checkInput() const override;

	// input
	cv::Mat mImg;
	double mScaleFactor = 1.0;

	// output
	cv::Mat mLabelImg;
	QVector<DMRegion> mRegions;
	LabelManager mManager;

	// helpers
	template <typename numFmt>
	cv::Mat maxChannels(const cv::Mat & src) const {

		// find the max channel
		cv::Mat maxImg(src.size(), src.depth(), cv::Scalar(0));

		for (int rIdx = 0; rIdx < maxImg.rows; rIdx++) {

			const numFmt* rPtr = src.ptr<numFmt>(rIdx);
			numFmt* mPtr = maxImg.ptr<numFmt>(rIdx);

			for (int cIdx = 0; cIdx < maxImg.cols; cIdx++) {

				numFmt m = *rPtr;	rPtr++;
				m = qMax(m, *rPtr); rPtr++;
				m = qMax(m, *rPtr); rPtr++;

				// max of all channels
				mPtr[cIdx] = m;
			}
		}

		return maxImg;
	}


};


}
