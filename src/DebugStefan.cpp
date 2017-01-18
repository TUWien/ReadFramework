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

#include "DebugStefan.h"

#include "WriterDatabase.h"
#include "WriterRetrieval.h"
#include "Image.h"
#include "opencv2/ml.hpp"

#pragma warning(push, 0)	// no warnings from includes
// Qt Includes
#include <QSharedPointer>
#include <QImage>
#include <QDebug>
#pragma warning(pop)

namespace rdf {
	TestWriterRetrieval::TestWriterRetrieval() {
	}
	void TestWriterRetrieval::run() {
		//QSharedPointer<rdf::WriterRetrievalConfig> wrc = QSharedPointer<rdf::WriterRetrievalConfig>(new rdf::WriterRetrievalConfig());
		//wrc->loadSettings();

		//QString imgPath = "D:/ABP_FirstTestCollection/M_Aigen_am_Inn_007_0021.jpg";
		//std::string xmlFile = "D:/ABP_FirstTestCollection/page/M_Aigen_am_Inn_007_0021.xml";

		//QImage i = QImage(imgPath);
		//rdf::WriterRetrieval wr = rdf::WriterRetrieval(Image::qImage2Mat(i));
		//wr.setConfig(wrc);
		//wr.setXmlPath(xmlFile);
		//wr.compute();

		std::string gmmPath = "C:/tmp/transkribus-settings/trigraph-gmm-pca64-40cluster-max90-min0-woNormbeforePCA-gmm.yml";
		cv::Ptr<cv::ml::EM>  mEM = cv::ml::EM::load<cv::ml::EM>(gmmPath);
		qDebug() << "loading done";
		std::string out;

		std::cout << mEM->getMeans();
	}
}