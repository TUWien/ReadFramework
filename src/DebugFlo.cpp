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
#include "Blobs.h"
#include "shapes.h"

#pragma warning(push, 0)	// no warnings from includes
// Qt Includes
#include <QDebug>
#include <QDebug>
#include <QImage>
#include <QFileInfo>
#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv/highgui.h"
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

		qDebug() << "image converted...";

		
		//test Otsu
		//cv::Mat binImg = rdf::Algorithms::instance().threshOtsu(inputImg);
		
		//if (inputImg.channels() != 1) cv::cvtColor(inputImg, inputImg, CV_RGB2GRAY);
		//if (inputImg.depth() != CV_8U) inputImg.convertTo(inputImg, CV_8U, 255);
		//findContours test
		//std::vector<std::vector<cv::Point> > contours;
		//std::vector<cv::Vec4i> hierarchy;
		//cv::findContours(inputImg, contours, hierarchy, cv::RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE);

		//rdf::BaseBinarizationSu testBin(inputImg);
		//testBin.compute();
		//cv::Mat binImg = testBin.binaryImage();

		rdf::BinarizationSuAdapted testBin(inputImg);
		testBin.compute();
		cv::Mat binImg = testBin.binaryImage();

		rdf::Blobs binBlobs;
		binBlobs.setImage(binImg);
		binBlobs.compute();

		//QVector<rdf::Blob> blobs = binBlobs.blobs();

		//binBlobs.setBlobs(rdf::BlobManager::instance().filterArea(20, binBlobs));
		//binBlobs.setBlobs(rdf::BlobManager::instance().filterMar(0.3f,200, binBlobs));
		//binBlobs.setBlobs(rdf::BlobManager::instance().filterAngle(0,));

		qDebug() << "blobs #: " << binBlobs.blobs().size();

		cv::Mat testImg;
		testImg = rdf::BlobManager::instance().drawBlobs(binBlobs);



		//rdf::BinarizationSuFgdWeight testBin(inputImg);
		//testBin.compute();
		//cv::Mat binImg = testBin.binaryImage();

		//rdf::Image::instance().imageInfo(binImg, "binImg");
		qDebug() << testBin << " in " << dt;
		
		QImage binImgQt = rdf::Image::instance().mat2QImage(testImg);

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