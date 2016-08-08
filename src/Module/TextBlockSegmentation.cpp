#include "TextBlockSegmentation.h"
#include "TextBlockSegmentation.h"
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

#include <opencv2/imgproc/imgproc.hpp>
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

	mEdges = connect(mSuperPixels);
	mEdges = filterEdges(mEdges);

	mDebug << "computed in" << dt;

	return true;
}

QSharedPointer<TextBlockConfig> TextBlockSegmentation::config() const {
	return qSharedPointerDynamicCast<TextBlockConfig>(mConfig);
}

QVector<QSharedPointer<PixelEdge> > TextBlockSegmentation::filterEdges(const QVector<QSharedPointer<PixelEdge>>& pixelEdges, double factor) {
	
	double meanEdgeLength = 0;

	for (auto pe : pixelEdges)
		meanEdgeLength += pe->edge().length();
	
	meanEdgeLength /= pixelEdges.size();
	double maxEdgeLength = meanEdgeLength * factor;

	QVector<QSharedPointer<PixelEdge>> pixelEdgesClean;

	for (auto pe : pixelEdges) {

		if (pe->edge().length() < maxEdgeLength)
			pixelEdgesClean << pe;
	}

	return pixelEdgesClean;
}

QVector<QSharedPointer<PixelEdge> > TextBlockSegmentation::connect(QVector<QSharedPointer<Pixel> >& superPixels) const {


	// Create an instance of Subdiv2D
	cv::Rect rect(cv::Point(), mSrcImg.size());
	cv::Subdiv2D subdiv(rect);

	QVector<int> ids;
	for (const QSharedPointer<Pixel>& b : superPixels)
		ids << subdiv.insert(b->center().toCvPointF());

	// that took me long... but this is how get can map the edges to our objects without an (expensive) lookup
	QVector<QSharedPointer<PixelEdge> > edges;
	for (int idx = 0; idx < superPixels.size()*3; idx++) {

		int orgVertex = ids.indexOf(subdiv.edgeOrg((idx << 2)));
		int dstVertex = ids.indexOf(subdiv.edgeDst((idx << 2)));

		// there are a few edges that lead to nowhere
		if (orgVertex == -1 || dstVertex == -1) {
			continue;
		}
		
		assert(orgVertex >= 0 && orgVertex < superPixels.size());
		assert(dstVertex >= 0 && dstVertex < superPixels.size());

		QSharedPointer<PixelEdge> pe(new PixelEdge(superPixels[orgVertex], superPixels[dstVertex]));
		edges << pe;
	}

	return edges;
}

cv::Mat TextBlockSegmentation::draw(const cv::Mat& img) const {
	
	// draw mser blobs
	Timer dtf;
	QPixmap pm = Image::instance().mat2QPixmap(img);
	
	QPainter p(&pm);
	Drawer::instance().setColor(ColorManager::instance().getRandomColor());
	p.setPen(Drawer::instance().pen());

	for (auto b : mEdges) {
		b->draw(p);
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

}