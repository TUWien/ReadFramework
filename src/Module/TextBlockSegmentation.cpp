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

#include "TextBlockSegmentation.h"

#include "Image.h"
#include "Drawer.h"
#include "Utils.h"

#pragma warning(push, 0)	// no warnings from includes
#include <QDebug>
#include <QPainter>

#pragma warning(pop)

namespace rdf {

// TextBlockConfig --------------------------------------------------------------------
TextBlockConfig::TextBlockConfig() {
	mModuleName = "Text Block";
}

QString TextBlockConfig::toString() const {
	return ModuleConfig::toString();
}

// TextBlockSegmentation --------------------------------------------------------------------
TextBlockSegmentation::TextBlockSegmentation(const cv::Mat& srcImg, 
	const QVector<QSharedPointer<Pixel> >& superPixels) {
	
	mSrcImg = srcImg;
	mSuperPixels = superPixels;
	mConfig = QSharedPointer<TextBlockConfig>::create();
}

bool TextBlockSegmentation::isEmpty() const {
	return mSrcImg.empty();
}

bool TextBlockSegmentation::compute() {

	Timer dt;

	if (!checkInput())
		return false;

	Rect r(0, 0, mSrcImg.cols, mSrcImg.rows);
	mEdges = PixelSet::connect(mSuperPixels, r);
	mEdges = filterEdges(mEdges);
	
	mTextBlocks = createTextBlocks(mEdges);

	mDebug << "computed in" << dt;

	return true;
}

QSharedPointer<TextBlockConfig> TextBlockSegmentation::config() const {
	return qSharedPointerDynamicCast<TextBlockConfig>(mConfig);
}

QVector<QSharedPointer<PixelEdge> > TextBlockSegmentation::filterEdges(const QVector<QSharedPointer<PixelEdge>>& pixelEdges, double factor) {
	
	double meanEdgeLength = 0;

	for (auto pe : pixelEdges)
		meanEdgeLength += pe->edgeWeight();
	
	meanEdgeLength /= pixelEdges.size();
	double maxEdgeLength = meanEdgeLength * factor;

	QVector<QSharedPointer<PixelEdge> > pixelEdgesClean;

	for (auto pe : pixelEdges) {

		if (pe->edgeWeight() < maxEdgeLength)
			pixelEdgesClean << pe;
	}

	return pixelEdgesClean;
}

cv::Mat TextBlockSegmentation::draw(const cv::Mat& img) const {
	
	// draw mser blobs
	Timer dtf;
	QPixmap pm = Image::instance().mat2QPixmap(img);
	
	QPainter p(&pm);
	QColor col = QColor(60, 60, 60);
	Drawer::instance().setColor(col);
	p.setPen(Drawer::instance().pen());

	for (auto b : mEdges) {
		b->draw(p);
	}

	for (auto tb : mTextBlocks) {
		Drawer::instance().setColor(ColorManager::instance().getRandomColor());
		p.setPen(Drawer::instance().pen());

		tb->draw(p);
	}

	mDebug << mEdges.size() << "edges drawn in" << dtf;

	return Image::instance().qPixmap2Mat(pm);
}

QString TextBlockSegmentation::toString() const {
	return Module::toString();
}

bool TextBlockSegmentation::checkInput() const {
	
	return !mSuperPixels.isEmpty();
}

QVector<QSharedPointer<PixelSet> > TextBlockSegmentation::createTextBlocks(const QVector<QSharedPointer<PixelEdge> >& edges) const {
	
	QVector<QSharedPointer<PixelSet> > sets;

	for (const QSharedPointer<PixelEdge> e : edges) {

		int fIdx = -1;
		int sIdx = -1;

		for (int idx = 0; idx < sets.size(); idx++) {

			if (sets[idx]->contains(e->first()))
				fIdx = idx;
			if (sets[idx]->contains(e->second()))
				sIdx = idx;

			if (fIdx != -1 && sIdx != -1)
				break;
		}

		// none is contained in a set
		if (fIdx == -1 && sIdx == -1) {
			QSharedPointer<PixelSet> ps(new PixelSet());
			ps->add(e->first());
			ps->add(e->second());
			sets << ps;
		}
		// add first to the set of second
		else if (fIdx == -1) {
			sets[sIdx]->add(e->first());
		}
		// add second to the set of first
		else if (sIdx == -1) {
			sets[fIdx]->add(e->second());
		}
		// two different idx? - merge the sets
		else if (fIdx != sIdx) {
			sets[fIdx]->merge(*sets[sIdx]);
			sets.remove(sIdx);
		}
		// else : nothing to do - they are both already added

	}

	qDebug() << "I found" << sets.size() << "text blocks";
	
	return sets;
}

}