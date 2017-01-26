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
#include "Utils.h"
#include "opencv2/ml.hpp"

#pragma warning(push, 0)	// no warnings from includes
// Qt Includes
#include <QSharedPointer>
#include <QImage>
#include <QDebug>
#include <QDirIterator>
#pragma warning(pop)

namespace rdf {
	TestWriterRetrieval::TestWriterRetrieval() {
	}
	void TestWriterRetrieval::run() {
		rdf::WriterDatabase wd = rdf::WriterDatabase();
		WriterVocabulary voc;
		voc.loadVocabulary("E:/Databases/writer-identification-competition/vocs/trigraph-binarized-gmm-pca96-60cluster-max90-min0.yml");
		wd.setVocabulary(voc);

		QString dirPath = "D:/Databases/icdar2011-cropped-invert/";
		QStringList list;
		//list << "1-1.png";
		//list << "1-2.png";
		//list << "1-3.png";
		//list << "4-1.png";
		//list << "4-2.png";
		//cv::Mat hists;
		//for(int i = 0; i < list.size(); i++) {
		//	WriterImage wi = WriterImage();
		//	QImage img = QImage(dirPath + list[i]);
		//	wi.setImage(Image::qImage2Mat(img));
		//	wd.addFile(wi);
		//}

		QDirIterator it(dirPath );
		while(it.hasNext()) {
			QString curEntry = it.next();
			if(!QFileInfo(curEntry).isDir()) {
				WriterImage wi = WriterImage();
				QImage img = QImage(curEntry);
				wi.setImage(Image::qImage2Mat(img));
				wd.addFile(wi);
				QFileInfo fi = QFileInfo(curEntry);
				list << fi.baseName();
			}
		}
		wd.writeCompetitionEvaluationFile(list, "c:/tmp/comp.csv");

		//QString outputString = "";
		//for(auto l : list) {
		//	outputString += l + "=" + l + "\n";
		//}
		//QFile file("C:/tmp/gtfile.csv");
		//if(file.open(QIODevice::WriteOnly)) {
		//	QTextStream stream(&file);
		//	stream << outputString;
		//	file.close();
		//}

	}
}