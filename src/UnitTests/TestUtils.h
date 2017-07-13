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

#pragma warning(push, 0)	// no warnings from includes
#include <QString>
#pragma warning(pop)

// TODO: add DllExport magic

// Qt defines

namespace rdf {

// read defines
class TestConfig{

public:
	TestConfig();

	void setImagePath(const QString& path);
	QString imagePath() const;

	void setXmlPath(const QString& path);
	QString xmlPath() const;

	void setClassifierPath(const QString& path);
	QString classifierPath() const;

	void setLabelConfigPath(const QString& path);
	QString labelConfigPath() const;

	void setFeatureCachePath(const QString& path);
	QString featureCachePath() const;

protected:
	QString mImagePath = "ftp://scruffy.caa.tuwien.ac.at/staff/read/test-resources/00000001-6.jpg";
	QString mXMLPath = "ftp://scruffy.caa.tuwien.ac.at/staff/read/test-resources/page/00000001-6.xml";
	QString mClassifierPath = "./classifier.yml";
	QString mFeatureCachePath = "./features.json";
	QString mLabelConfigPath = "ftp://scruffy.caa.tuwien.ac.at/staff/read/test-resources/configs/config-baseline.json";
};

}
