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

#include "WriterIdentification.h"


//opencv
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/xfeatures2d.hpp"

#pragma warning(push, 0)	// no warnings from includes
// Qt Includes
#include <QSharedPointer>
#include <QVector>
#include <QFileInfo>
#include <QDebug>
#include <QDir>
#pragma warning(pop)

namespace rdf {
	/// <summary>
	/// Initializes a new instance of the <see cref="WriterIdentification"/> class.
	/// </summary>
	WriterIdentification::WriterIdentification() {
		// do nothing
	}
	/// <summary>
	/// Sets the image.
	/// </summary>
	/// <param name="img">The img.</param>
	void WriterIdentification::setImage(const cv::Mat img) {
		this->mImg = img;
	}
	/// <summary>
	/// Calculates the SIFT features of the image.
	/// </summary>
	void WriterIdentification::calculateFeatures() {
		if(mImg.empty())
			return;
#ifdef WITH_XFEATURES2D
		cv::Ptr<cv::Feature2D> sift = cv::xfeatures2d::SIFT::create();
		std::vector<cv::KeyPoint> kp;
		sift->detect(mImg, kp, cv::Mat());


		qWarning() << "using modified SIFT";

		for(int i = 0; i < kp.size(); i++) {
			if(kp[i].angle > 180)
				kp[i].angle -= 180;
			//kp[i].angle = 0;
			//kp[i].size = 10;
		}

		sift->compute(mImg, kp, mDescriptors);

		mKeyPoints = QVector<cv::KeyPoint>::fromStdVector(kp);
#else
		mKeyPoints = QVector<cv::KeyPoint>();
		qWarning() << "ReadFramework is compilied without xfeatures2d. Thus, WriterRetrieval is not able to generate features";
		#pragma message("[WARNING] Writer Retrieval is not working when compiling without xfeatures2d! Please enable it in cmake")
#endif

		
	}
	/// <summary>
	/// Saves the SIFT features to the given file path.
	/// </summary>
	/// <param name="filePath">The file path.</param>
	void WriterIdentification::saveFeatures(QString filePath) {
		if(mKeyPoints.empty() || mDescriptors.empty()) {
			qWarning() << debugName() << " keypoints or descriptors empty ... unable to save to file";
			return;
		}
		QFileInfo fi(filePath);
		if(!fi.absoluteDir().exists()) {
			qWarning() << debugName() << " unable to save features, directory " << fi.absoluteDir().path() << "does not exist";
		}
		cv::FileStorage fs(filePath.toStdString(), cv::FileStorage::WRITE);
		fs << "keypoints" << mKeyPoints.toStdVector();
		fs << "descriptors" << mDescriptors;
		fs.release();
	}
	/// <summary>
	/// Loads the features from the given file path.
	/// </summary>
	/// <param name="filePath">The file path.</param>
	void WriterIdentification::loadFeatures(QString filePath) {
		cv::FileStorage fs(filePath.toStdString(), cv::FileStorage::READ);
		if(!fs.isOpened()) {
			qWarning() << debugName() << " unable to read file " << filePath;
			return;
		}
		std::vector<cv::KeyPoint> kp;
		fs["keypoints"] >> kp;
		mKeyPoints = QVector<cv::KeyPoint>::fromStdVector(kp);
		fs["descriptors"] >> mDescriptors;
		fs.release();
	}
	/// <summary>
	/// Sets the key points for this Writer Identification task.
	/// </summary>
	/// <param name="kp">The keypoints.</param>
	void WriterIdentification::setKeyPoints(QVector<cv::KeyPoint> kp) {
		mKeyPoints = kp;
	}
	/// <summary>
	/// Returns the keypoints of the SIFT features.
	/// </summary>
	/// <returns>keypoints</returns>
	QVector<cv::KeyPoint> WriterIdentification::keyPoints() const {
		return mKeyPoints;
	}
	/// <summary>
	/// Sets the descriptors for this Writer Identification task.
	/// </summary>
	/// <param name="desc">The descriptors.</param>
	void WriterIdentification::setDescriptors(cv::Mat desc) {
		mDescriptors = desc;
	}
	/// <summary>
	/// Returns the descriptors of the SIFT features
	/// </summary>
	/// <returns></returns>
	cv::Mat WriterIdentification::descriptors() const {
		return mDescriptors;
	}
	/// <summary>
	/// Debug name
	/// </summary>
	/// <returns></returns>
	QString WriterIdentification::debugName() {
		return "WriterIdentification";
	}
}