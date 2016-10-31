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

#include "PageSegmentation.h"

#include "Utils.h"
#include "Image.h"
#include "Drawer.h"
#include "ImageProcessor.h"

#include "SuperPixel.h"

#pragma warning(push, 0)	// no warnings from includes
#include <QDebug>
#include <QPainter>

#include <opencv2\imgproc\imgproc.hpp>
#include <opencv2/features2d/features2d.hpp>

#pragma warning(pop)

namespace rdf {

// PageSegmentationConfig --------------------------------------------------------------------
PageSegmentationConfig::PageSegmentationConfig() : ModuleConfig("Page Segmentation") {
}

QString PageSegmentationConfig::toString() const {
	return ModuleConfig::toString();
}

int PageSegmentationConfig::maxSide() const {
	return mMaxSide;
}

// PageSegmentation --------------------------------------------------------------------
PageSegmentation::PageSegmentation(const cv::Mat& img) {

	mImg = img;
	mConfig = QSharedPointer<PageSegmentationConfig>::create();
}

bool PageSegmentation::isEmpty() const {
	return mImg.empty();
}

bool PageSegmentation::compute() {

	// TODO: add page segmentation

	qWarning() << "page segmentation not implemented yet...";
	return true;
}

QSharedPointer<PageSegmentationConfig> PageSegmentation::config() const {
	return qSharedPointerDynamicCast<PageSegmentationConfig>(mConfig);
}

cv::Mat PageSegmentation::draw(const cv::Mat& img) const {

	// draw mser blobs
	Timer dtf;
	QPixmap pm = Image::mat2QPixmap(img);

	QPainter p(&pm);
	
	return Image::qPixmap2Mat(pm);
}

QString PageSegmentation::toString() const {
	return Module::toString();
}

bool PageSegmentation::checkInput() const {

	return !mImg.empty();
}

}