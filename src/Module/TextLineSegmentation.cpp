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

#include "TextLineSegmentation.h"

#include "Utils.h"
#include "Image.h"
#include "Drawer.h"

#pragma warning(push, 0)	// no warnings from includes
#include <QDebug>
#include <QPainter>
#pragma warning(pop)

namespace rdf {

// TextLineConfig --------------------------------------------------------------------
TextLineConfig::TextLineConfig() : ModuleConfig("Text Line Module") {
}

QString TextLineConfig::toString() const {
	return ModuleConfig::toString();
}

double TextLineConfig::minDistFactor() const {
	return mMinDistFactor;
}

void TextLineConfig::setMinDistFactor(double val) {
	mMinDistFactor = val;
}

void TextLineConfig::load(const QSettings & settings) {
	mMinDistFactor = settings.value("minDistFactor", minDistFactor()).toDouble();
}

void TextLineConfig::save(QSettings & settings) const {

	settings.setValue("minDistFactor", minDistFactor());
}

// TextLineSegmentation --------------------------------------------------------------------
TextLineSegmentation::TextLineSegmentation(const QVector<QSharedPointer<Pixel> >& superPixels) {

	mSuperPixels = superPixels;
	mConfig = QSharedPointer<TextLineConfig>::create();
	mConfig->loadSettings();
	mConfig->saveDefaultSettings();
}

bool TextLineSegmentation::isEmpty() const {
	return mSuperPixels.empty();
}

bool TextLineSegmentation::compute() {

	Timer dt;

	if (!checkInput())
		return false;

	//RegionPixelConnector rpc;
	//rpc.setLineSpacingMultiplier(2.0);
	DelauneyPixelConnector dpc;
	QVector<QSharedPointer<PixelEdge> > pEdges = dpc.connect(mSuperPixels);

	qDebug() << "# edges: " << pEdges.size();

	QVector<QSharedPointer<LineEdge> > lEdges;
	for (const QSharedPointer<PixelEdge>& pe : pEdges)
		lEdges << QSharedPointer<LineEdge>(new LineEdge(*pe));

	lEdges = filterEdges(lEdges, config()->minDistFactor());

	if (!mStopLines.empty())
		lEdges = filterEdges(lEdges, mStopLines);

	mSets = toSets(lEdges);

	//mDebug << mSets.size() << "text lines found";
	//mSets = merge(mSets, 160);
	//mDebug << mSets.size() << "text lines (after merging)";

	//mSets = filter(mSets, 1.0);
	//mDebug << mSets.size() << "text lines (after filtering)";

	mDebug << "computed in" << dt;

	return true;
}

QSharedPointer<TextLineConfig> TextLineSegmentation::config() const {
	return qSharedPointerDynamicCast<TextLineConfig>(mConfig);
}

QVector<QSharedPointer<LineEdge> > TextLineSegmentation::filterEdges(const QVector<QSharedPointer<LineEdge>>& edges, double factor) const {

	QVector<QSharedPointer<LineEdge> > pixelEdgesClean;

	for (auto pe : edges) {

		if (pe->edgeWeight() < factor)
			pixelEdgesClean << pe;
	}

	return pixelEdgesClean;
}

QVector<QSharedPointer<LineEdge>> TextLineSegmentation::filterEdges(const QVector<QSharedPointer<LineEdge>>& edges, const QVector<Line>& lines) const {

	QVector<QSharedPointer<LineEdge> > pixelEdgesClean;

	for (const QSharedPointer<LineEdge>& pe : edges) {

		bool filter = false;
		
		// does the edge intersect with any 'stop' line?
		for (const Line& line : lines) {
			if (pe->edge().intersects(line)) {
				filter = true;
				break;
			}
		}

		if (!filter)
			pixelEdgesClean << pe;
	}

	return pixelEdgesClean;
}

QVector<PixelSet > TextLineSegmentation::toSets(const QVector<QSharedPointer<LineEdge> > & edges) const {

	// why do I have to do this?
	QVector<QSharedPointer<PixelEdge> > pe;
	for (auto e : edges)
		pe << e;

	QVector<QSharedPointer<PixelSet> > allSets = PixelSet::fromEdges(pe);
	QVector<PixelSet> filteredSets;

	for (auto s : allSets) {

		if (s->pixels().size() > 3)
			filteredSets << *s;
	}

	return filteredSets;
}

QVector<PixelSet> TextLineSegmentation::merge(const QVector<PixelSet>& sets, double ) const {



	QVector<QString> mergedIds;
	QVector<PixelSet> mergedSets;


	for (auto oSet : sets) {

		if (mergedIds.contains(oSet.id()))
			continue;

		Line baseLine = oSet.fitLine();
		Vector2D center = oSet.meanCenter();
		
		Vector2D orVec(0, 1);
		double angle = oSet.orientation();
		orVec.rotate(angle);

		for (auto cSet : sets) {

			if (cSet == oSet)
				continue;

			if (mergedIds.contains(cSet.id()))
				continue;


			PixelSet clSet = oSet;	// clone
			clSet.merge(cSet);
			
			Vector2D cd = clSet.meanCenter() - center;
			double w = cd.theta(orVec);
			
			if (baseLine.diffAngle(clSet.fitLine()) < CV_PI*0.001 && cd.length()*w < 10) {
				oSet = clSet;
				mergedIds << oSet.id();
				mergedIds << cSet.id();
			}
		}

		mergedSets << oSet;
	}

	return mergedSets;

	//QMap<QString, QString> mergeMap;
	//QVector<QString> mergedIds;

	//QVector<PixelSet > mergedSets;


	//for (auto oSet : sets) {

	//	if (mergedIds.contains(oSet->id()))
	//		continue;

	//	for (auto cSet : sets) {

	//		if (cSet == oSet)
	//			continue;

	//		if (mergedIds.contains(cSet->id()))
	//			continue;

	//		assert(cSet);

	//		double o = oSet->overlapRatio(*cSet);

	//		if (o < overlap) {
	//			oSet->merge(*cSet);
	//			//mergeMap.insert(oSet->id(), cSet->id());
	//			mergedIds << oSet->id();
	//			mergedIds << cSet->id();
	//		}
	//	}

	//	mergedSets << oSet;
	//}

	////for (const QString& fId : mergeMap) {
	////	
	////	PixelSet set = findSet(sets, fId);
	////	PixelSet oSet = findSet(sets, mergeMap.value(fId));

	////	assert(set && oSet);

	////	set->merge(*oSet);
	////}


	//return mergedSets;
}

QVector<PixelSet> TextLineSegmentation::filter(const QVector<PixelSet>& sets, double ) const {

	double nPx = 0;

	for (auto set : sets) {
		nPx += (double)sets.size();
	}

	nPx /= sets.size();
	
	QVector<PixelSet> fSets;

	for (auto set : sets) {

		if (set.size() > 32)
			fSets << set;

	}

	return fSets;
}

PixelSet TextLineSegmentation::findSet(const QVector<PixelSet>& sets, const QString & id) const {

	for (auto set : sets) {
		//assert(set);
		if (set.id() == id)
			return set;
	}

	return PixelSet();
}

void TextLineSegmentation::slac(const QVector<QSharedPointer<LineEdge> >& ) const {

	QVector<PixelSet > clusters;

	//for (const QSharedPointer<PixelEdge>& pe : edges) {
	//	PixelSet ps(new PixelSet());
	//	ps->add(pe->first());
	//	ps->add(pe->second());
	//}

	// TODO: go on here...

}

cv::Mat TextLineSegmentation::draw(const cv::Mat& img) const {

	// draw mser blobs
	Timer dtf;
	QPixmap pm = Image::mat2QPixmap(img);

	QPainter p(&pm);
	
	// this block draws the edges
	Drawer::instance().setColor(ColorManager::darkGray(0.4));
	p.setPen(Drawer::instance().pen());

	//for (auto b : mEdges) {
	//	b->draw(p);
	//}

	// show the stop lines
	Drawer::instance().setColor(ColorManager::red(0.4));
	p.setPen(Drawer::instance().pen());

	for (auto l : mStopLines)
		l.draw(p);

	//auto sets = toSets();
	
	for (auto set : mSets) {
		Drawer::instance().setColor(ColorManager::getColor());
		p.setPen(Drawer::instance().pen());
		set.draw(p, (int)PixelSet::draw_poly | (int)PixelSet::draw_pixels);

		Line baseLine = set.fitLine();
		baseLine.setThickness(3.0);
		baseLine.draw(p);

		//for (auto pixel : set->pixels()) {
		//	pixel->draw(p, .3, Pixel::draw_ellipse_only);
		//}
	}

	//mDebug << mEdges.size() << "edges drawn in" << dtf;

	return Image::qPixmap2Mat(pm);
}

QString TextLineSegmentation::toString() const {
	return Module::toString();
}

void TextLineSegmentation::addLines(const QVector<Line>& lines) {
	mStopLines << lines;
}

QVector<QSharedPointer<TextLine>> TextLineSegmentation::textLines() const {
	
	QVector<QSharedPointer<TextLine>> tls;
	for (auto set : mSets)
		tls << set.toTextLine();

	return tls;
}

bool TextLineSegmentation::checkInput() const {

	return !mSuperPixels.isEmpty();
}

}