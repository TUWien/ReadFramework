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

#include "Evaluation.h"

#include "PixelLabel.h"

#pragma warning(push, 0)	// no warnings from includes
#include <QFileInfo>
#include <QDebug>
#pragma warning(pop)

namespace rdf {

// -------------------------------------------------------------------- SuperPixelEval 
EvalInfo::EvalInfo(const QString & name) : mName(name) {
}

void EvalInfo::operator+=(const EvalInfo & o) {
	mTp += o.mTp;
	mTn += o.mTn;
	mFp += o.mFp;
	mFn += o.mFn;
}

void EvalInfo::eval(int trueClassId, int predictedClassId, bool isBackground) {

	if (trueClassId == LabelInfo::label_unknown)
		return;

	if (isBackground) {
		(trueClassId == predictedClassId) ? mTn++ : mFn++;
	}
	else {
		(trueClassId == predictedClassId) ? mTp++ : mFp++;
	}
}

void EvalInfo::setName(const QString & name) {
	mName = name;
}

QString EvalInfo::name() const {
	return mName;
}

double EvalInfo::accuracy() const {
	return ((double)mTp + mTn) / count();
}

double EvalInfo::fscore() const {
	return 2.0*mTp / ((double)2.0*mTp + mFp + mFn);
}

double EvalInfo::precision() const {

	return mTp / ((double)mTp + mFp);
}

double EvalInfo::recall() const {

	return mTp / ((double)mTp + mFn);
}

int EvalInfo::count() const {
	return mTp + mTn + mFp + mFn;
}

QString EvalInfo::header() {
	return QString("# name, \tcount, \ttp, \ttn, \tfp, \tfn, \tp, \tr, \tf1-score, \tacc");
}

QString EvalInfo::toString() const {
	
	QString stats = name() + ",\t";
	stats += QString::number(count()) + ",\t";
	stats += QString::number(mTp) + ",\t";
	stats += QString::number(mTn) + ",\t";
	stats += QString::number(mFp) + ",\t";
	stats += QString::number(mFn) + ",\t";

	stats += QString::number(precision()) + ",\t";
	stats += QString::number(recall()) + ",\t";
	stats += QString::number(fscore()) + ",\t";
	stats += QString::number(accuracy()) + ",\t";
	
	return stats;
}

// -------------------------------------------------------------------- EvalInfoManager 
EvalInfoManager::EvalInfoManager(const QVector<EvalInfo>& evals) {
	mEvals = evals;
}

QString EvalInfoManager::toString() const {

	EvalInfo eia;

	for (auto e : mEvals) {
		eia += e;
	}

	return eia.toString();
}

bool EvalInfoManager::write(const QString & filePath) const {

	QFile f(filePath);

	if (!f.open(QIODevice::WriteOnly)) {
		qWarning() << "cannot open" << filePath;
		return false;
	}

	f.write(toBuffer());
	f.close();

	return true;
}

QByteArray EvalInfoManager::toBuffer() const {

	QByteArray ba;
	ba.append(EvalInfo::header() + "\n\n");

	for (const EvalInfo& e : mEvals) {
		ba.append(e.toString() + "\n");
	}

	return ba;
}

}