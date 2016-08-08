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
#include "Pixel.h"

#pragma warning(push, 0)	// no warnings from includes
// Qt Includes
#pragma warning(pop)

#ifndef DllModuleExport
#ifdef DLL_MODULE_EXPORT
#define DllModuleExport Q_DECL_EXPORT
#else
#define DllModuleExport Q_DECL_IMPORT
#endif
#endif

// Qt defines

namespace rdf {

// read defines


class DllModuleExport TextBlockConfig : public ModuleConfig {

public:
	TextBlockConfig();

	virtual QString toString() const override;

protected:

	//void load(const QSettings& settings) override;
	//void save(QSettings& settings) const override;
};

class DllModuleExport TextBlockSegmentation : public Module {

public:
	TextBlockSegmentation(const cv::Mat& srcImg = cv::Mat(), 
		const QVector<QSharedPointer<Pixel> >& superPixels = QVector<QSharedPointer<Pixel> >());

	bool isEmpty() const override;
	bool compute() override;
	QSharedPointer<TextBlockConfig> config() const;

	QVector<QSharedPointer<PixelEdge> > filterEdges(const QVector<QSharedPointer<PixelEdge> >& pixelEdges, double factor = 1.2);
	QVector<QSharedPointer<PixelEdge> > connect(QVector<QSharedPointer<Pixel> >& superPixels) const;

	cv::Mat draw(const cv::Mat& img) const;
	QString toString() const override;

private:
	QVector<QSharedPointer<Pixel> > mSuperPixels;
	QVector<QSharedPointer<PixelEdge> > mEdges;
	cv::Mat mSrcImg;

	bool checkInput() const override;
};

};