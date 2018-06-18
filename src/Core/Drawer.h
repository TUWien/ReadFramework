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
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program. If not, see <http://www.gnu.org/licenses/>.

 The READ project has received funding from the European Union’s Horizon 2020 
 research and innovation programme under grant agreement No 674943
 
 related links:
 [1] http://www.cvl.tuwien.ac.at/cvl/
 [2] https://transkribus.eu/Transkribus/
 [3] https://github.com/TUWien/
 [4] http://nomacs.org
 *******************************************************************************************************/

#pragma once

#include <vector>

#pragma warning(push, 0)	// no warnings from includes
#include <QColor>
#include <QPen>

#include <opencv2/core.hpp>
#pragma warning(pop)

#ifndef DllCoreExport
#ifdef DLL_CORE_EXPORT
#define DllCoreExport Q_DECL_EXPORT
#else
#define DllCoreExport Q_DECL_IMPORT
#endif
#endif

// Qt/CV defines
namespace cv {
	class Mat;
}

class QPainter;
class QPointF;
class QRectF;
class QPen;

namespace rdf {

// read defines

namespace Drawer {

	DllCoreExport void drawPoints(QPainter& p, const std::vector<cv::Point>& pts);
	DllCoreExport void drawPoints(QPainter& p, const QVector<QPointF>& pts);

	DllCoreExport void drawRects(QPainter& p, const QVector<QRectF>& rects);

	// add general drawing functions here

}

namespace ColorManager {

	DllCoreExport QColor randColor(double alpha = 1.0);
	DllCoreExport QColor getColor(int idx, double alpha = 1.0);
	DllCoreExport QVector<QColor> colors();

	DllCoreExport QColor lightGray(double alpha = 1.0);
	DllCoreExport QColor darkGray(double alpha = 1.0);
	DllCoreExport QColor red(double alpha = 1.0);
	DllCoreExport QColor green(double alpha = 1.0);
	DllCoreExport QColor blue(double alpha = 1.0);
	DllCoreExport QColor pink(double alpha = 1.0);
	DllCoreExport QColor white(double alpha = 1.0);

	DllCoreExport QColor alpha(const QColor& col, double a);

	// add your favorite colors here
}

}
