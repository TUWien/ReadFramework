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

#include "DebugFlo.h"
#include "Image.h"
#include "Utils.h"
#include "Binarization.h"
#include "Algorithms.h"

#pragma warning(push, 0)	// no warnings from includes
// Qt Includes
#include <QDebug>
#include <QDebug>
#include <QImage>
#include <QFileInfo>
#pragma warning(pop)

namespace rdf {

	BinarizationTest::BinarizationTest(const DebugConfig& config) {

		mConfig = config;
	}

	void BinarizationTest::binarizeTest() {

		qDebug() << mConfig.imagePath() << "loaded";

		rdf::Timer dt;
		QImage img;
		img.load(mConfig.imagePath());
		cv::Mat inputImg = rdf::Image::instance().qImage2Mat(img);

		
		//test Otsu
		//cv::Mat binImg = rdf::Algorithms::instance().threshOtsu(inputImg);
		
		rdf::BaseBinarizationSu testBin(inputImg);
		testBin.compute();
		cv::Mat binImg = testBin.binaryImage();

		rdf::Image::instance().imageInfo(binImg, "binImg");
		qDebug() << testBin << " in " << dt;
		
		QImage binImgQt = rdf::Image::instance().mat2QImage(binImg);

		if (!mConfig.outputPath().isEmpty()) {
			qDebug() << "saving to" << mConfig.outputPath();

			//binImgQt = binImgQt.convertToFormat(QImage::Format_RGB888);
			//binImgQt.save(mConfig.outputPath());
			rdf::Image::instance().save(binImgQt, mConfig.outputPath());
			
		} else {
			qDebug() << "no save path";
		}
	}

}