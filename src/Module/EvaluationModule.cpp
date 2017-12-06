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

#include "EvaluationModule.h"
#include "Image.h"

#pragma warning(push, 0)	// no warnings from includes
#include <QPainter>
#pragma warning(pop)

namespace rdf {

// -------------------------------------------------------------------- SuperPixelEval 
SuperPixelEval::SuperPixelEval(const PixelSet & set) : mSet(set) {
}

bool SuperPixelEval::isEmpty() const {
	return mSet.isEmpty();
}

bool SuperPixelEval::compute() {

	if (!checkInput())
		return false;

	// create statistics
	for (const QSharedPointer<Pixel>& px : mSet.pixels()) {

		auto pl = px->label();
		if (pl)
			mEvalInfo.eval(pl->trueLabel().id(), pl->predicted().id(), pl->trueLabel().isBackground());
	}

	return true;
}

EvalInfo SuperPixelEval::evalInfo() const {
	return mEvalInfo;
}

QString SuperPixelEval::toString() const {
	return config()->toString();
}

cv::Mat SuperPixelEval::draw(const cv::Mat & img) const {
	
	QImage qImg = Image::mat2QImage(img, true);
	
	QPainter p(&qImg);

	for (auto px : mSet.pixels()) {
		px->draw(p, 0.3, rdf::Pixel::DrawFlags() | Pixel::draw_ellipse | Pixel::draw_label_colors);
	}
	
	return Image::qImage2Mat(qImg);
}

bool SuperPixelEval::checkInput() const {
	return !mSet.isEmpty();
}

}