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

#include "Utils.h"

#pragma warning(push, 0)	// no warnings from includes
#include <QApplication>
#include <QDebug>
#include <QPolygon>
#include <QFileInfo>
#include <QDir>
#include <QTime>
#include <QColor>

#include <opencv2/core/core.hpp>
#pragma warning(pop)

// needed for registering the file version
#ifdef WIN32
#include "shlwapi.h"
#pragma comment (lib, "shlwapi.lib")
#endif


namespace rdf {


// Utils --------------------------------------------------------------------
Utils::Utils() {

	// initialize random
	qsrand(QTime::currentTime().msec());
}

Utils& Utils::instance() { 

	static QSharedPointer<Utils> inst;
	if (!inst)
		inst = QSharedPointer<Utils>(new Utils());
	return *inst; 
}

void Utils::initFramework() const {

	// format console
	QString p = "%{if-info}[INFO] %{endif}%{if-warning}[WARNING] %{endif}%{if-critical}[CRITICAL] %{endif}%{if-fatal}[ERROR] %{endif}%{message}";
	qSetMessagePattern(p);

	registerVersion();
}

void Utils::registerVersion() const {

#ifdef WIN32
	// this function is based on code from:
	// http://stackoverflow.com/questions/316626/how-do-i-read-from-a-version-resource-in-visual-c

	QString version(RDF_FRAMEWORK_VERSION);	// default version (we do not know the build)
	
	// get the filename of the executable containing the version resource
	TCHAR szFilename[MAX_PATH + 1] = {0};
	if (GetModuleFileName(NULL, szFilename, MAX_PATH) == 0) {
		qWarning() << "Sorry, I can't read the module fileInfo name";
		return;
	}

	// allocate a block of memory for the version info
	DWORD dummy;
	DWORD dwSize = GetFileVersionInfoSize(szFilename, &dummy);
	if (dwSize == 0) {
		qWarning() << "The version info size is zero\n";
		return;
	}
	QVector<BYTE> bytes(dwSize);

	if (bytes.empty()) {
		qWarning() << "The version info is empty\n";
		return;
	}

	// load the version info
	if (!bytes.empty() && !GetFileVersionInfo(szFilename, NULL, dwSize, &bytes[0])) {
		qWarning() << "Sorry, I can't read the version info\n";
		return;
	}

	// get the name and version strings
	UINT                uiVerLen = 0;
	VS_FIXEDFILEINFO*   pFixedInfo = 0;     // pointer to fixed file info structure

	if (!bytes.empty() && !VerQueryValue(&bytes[0], TEXT("\\"), (void**)&pFixedInfo, (UINT *)&uiVerLen)) {
		qWarning() << "Sorry, I can't get the version values...\n";
		return;
	}

	// pFixedInfo contains a lot more information...
	version = QString::number(HIWORD(pFixedInfo->dwFileVersionMS)) + "."
		+ QString::number(LOWORD(pFixedInfo->dwFileVersionMS)) + "."
		+ QString::number(HIWORD(pFixedInfo->dwFileVersionLS)) + "."
		+ QString::number(LOWORD(pFixedInfo->dwFileVersionLS));

#else
	QString version(RDF_FRAMEWORK_VERSION);	// default version (we do not know the build)
#endif
	QApplication::setApplicationVersion(version);

}

/// <summary>
/// Returns a random number within [0 1].
/// </summary>
/// <returns></returns>
double Utils::rand() {

	return (double)qrand() / RAND_MAX;
}

/// <summary>
/// Creates a new file path from filePath.
/// Hence, C:\temp\josef.png can be turned into C:\temp\josef-something.xml
/// </summary>
/// <param name="filePath">The old file path.</param>
/// <param name="attribute">An attribute string which is appended to the filename.</param>
/// <param name="newSuffix">A new suffix, the old suffix is used if empty.</param>
/// <returns>The new file path.</returns>
QString Utils::createFilePath(const QString & filePath, const QString & attribute, const QString & newSuffix) const {
	
	QFileInfo info(filePath);
	QString suffix = (newSuffix.isEmpty()) ? info.suffix() : newSuffix;
	QString newFilePath = baseName(filePath) + attribute + "." + suffix;

	return newFilePath;
}

/// <summary>
/// Returns the filePath without suffix.
/// C:/temp/something.png -> C:/temp/something
/// This fixes an issue of Qt QFileInfo::baseName which 
/// returns wrong basenames if the filename contains dots
/// Qt baseName:
/// Best. 901 Nr. 112 00147.jpg -> Best.
/// This method:
/// Best. 901 Nr. 112 00147.jpg -> Best. 901 Nr. 112 00147
/// </summary>
/// <param name="filePath">The file path.</param>
/// <returns>The file path without suffix.</returns>
QString Utils::baseName(const QString & filePath) const {

	QString suffix = QFileInfo(filePath).suffix();

	int sI = filePath.lastIndexOf(suffix);

	if (sI < 1) {
		qWarning() << "Cannot extract basename:" << filePath << "does not have a suffix";
		return filePath;
	}

	return filePath.left(sI-1);	// -1 to remove the point
}

// ColorManager --------------------------------------------------------------------
/// <summary>
/// Returns a pleasent random color.
/// </summary>
/// <param name="idx">If idx != -1 a specific color is chosen from the palette.</param>
/// <returns></returns>
QColor ColorManager::getColor(int idx, double alpha) {

	QVector<QColor> cols = colors();
	int maxCols = cols.size();
	if (idx == -1)
		idx = qRound(Utils::rand()*maxCols*3);

	assert(idx >= 0 && cols.size() > 0);

	QColor col = cols[idx % cols.size()];

	// currently not hit
	if (idx > 2 * cols.size())
		col = col.darker();
	else if (idx > cols.size())
		col = col.lighter();

	col.setAlpha(qRound(alpha * 255));

	return col;
}

/// <summary>
/// Returns our color palette.
/// </summary>
/// <returns></returns>
QVector<QColor> ColorManager::colors() {

	static QVector<QColor> cols;

	if (cols.empty()) {
		cols << QColor(238, 120, 34);
		cols << QColor(240, 168, 47);
		cols << QColor(120, 192, 167);
		cols << QColor(251, 234, 181);
	}

	return cols;
}

/// <summary>
/// Returns a light gray.
/// </summary>
/// <param name="alpha">Optional alpha [0 1].</param>
/// <returns></returns>
QColor ColorManager::lightGray(double alpha) {
	return QColor(200, 200, 200, qRound(alpha*255));
}

/// <summary>
/// Returns a dark gray.
/// </summary>
/// <param name="alpha">Optional alpha [0 1].</param>
/// <returns></returns>
QColor ColorManager::darkGray(double alpha) {
	return QColor(66, 66, 66, qRound(alpha*255));
}

/// <summary>
/// Returns a dark red.
/// </summary>
/// <param name="alpha">Optional alpha [0 1].</param>
/// <returns></returns>
QColor ColorManager::red(double alpha) {
	return QColor(200, 50, 50, qRound(alpha*255));
}

// Converter --------------------------------------------------------------------

/// <summary>
/// Converts a cv::Rect to QRectF.
/// </summary>
/// <param name="r">The OpenCV rectangle.</param>
/// <returns></returns>
QRectF Converter::cvRectToQt(const cv::Rect & r) {
	return QRectF(r.x, r.y, r.width, r.height);
}

/// <summary>
/// Converts a QRectF to a cv::Rect.
/// </summary>
/// <param name="r">The Qt rectangle.</param>
/// <returns></returns>
cv::Rect2d Converter::qRectToCv(const QRectF & r) {
	return cv::Rect2d(r.x(), r.y(), r.width(), r.height());
}

/// <summary>
/// Converts a PAGE points attribute to a polygon.
/// the format is: p1x,p1y p2x,p2y (for two points p1, p2)
/// </summary>
/// <param name="pointList">A string containing the point list.</param>
/// <returns>A QPolygon parsed from the point list.</returns>
QPolygon Converter::stringToPoly(const QString& pointList) {

	// we expect point pairs like that: <Coords points="1077,482 1167,482 1167,547 1077,547"/>
	QStringList pairs = pointList.split(" ");
	QPolygon poly;

	for (const QString pair : pairs) {

		QStringList points = pair.split(",");

		if (points.size() != 2) {
			qWarning() << "illegal point string: " << pair;
			continue;
		}

		bool xok = false, yok = false;
		int x = points[0].toInt(&xok);
		int y = points[1].toInt(&yok);

		if (xok && yok)
			poly.append(QPoint(x, y));
		else
			qWarning() << "illegal point string: " << pair;
	}

	return poly;
}

/// <summary>
/// Converts a QPolygon to QString according to the PAGE XML format.
/// A line would look like this: p1x,p1y p2x,p2y
/// </summary>
/// <param name="polygon">The polygon to convert.</param>
/// <returns>A string representing the polygon.</returns>
QString Converter::polyToString(const QPolygon& polygon) {

	QString polyStr;

	for (const QPoint& p : polygon) {
		polyStr += QString::number(p.x()) + "," + QString::number(p.y()) + " ";
	}

	// NOTE: we have one space at the end
	//FK040716: remove last space - otherwise we get a warning when reading
	polyStr.remove(polyStr.length() - 1, 1);

	return polyStr;
}

QPointF Converter::cvPointToQt(const cv::Point & pt) {
	return QPointF(pt.x, pt.y);
}

cv::Point2d Converter::qPointToCv(const QPointF & pt) {
	return cv::Point2d(pt.x(), pt.y());
}

// Timer --------------------------------------------------------------------
/**
* Initializes the class and stops the clock.
**/
Timer::Timer() {
	mTimer.start();
}

QDataStream& operator<<(QDataStream& s, const Timer& t) {

	// this makes the operator<< virtual (stroustrup)
	return t.put(s);
}

QDebug operator<<(QDebug d, const Timer& t) {

	d << qPrintable(t.stringifyTime(t.elapsed()));
	return d;
}

/**
* Returns a string with the total time interval.
* The time interval is measured from the time,
* the object was initialized.
* @return the time in seconds or milliseconds.
**/
QString Timer::getTotal() const {

	return qPrintable(stringifyTime(mTimer.elapsed()));
}

QDataStream& Timer::put(QDataStream& s) const {

	s << stringifyTime(mTimer.elapsed());

	return s;
}

/**
* Converts time to QString.
* @param ct current time interval
* @return QString the time interval as string
**/ 
QString Timer::stringifyTime(int ct) const {

	if (ct < 1000)
		return QString::number(ct) + " ms";

	int v = qRound(ct / 1000.0);
	int ms = ct % 1000;
	int sec = v % 60;	v = qRound(v / 60.0);
	int min = v % 60;	v = qRound(v / 60.0);
	int h = v % 24;		v = qRound(v / 24.0);
	int d = v;
	
	QString mss = QString("%1").arg(ms, 3, 10, QChar('0')); // zero padding
	QString secs = QString::number(sec);

	if (ct < 10000)
		return secs + "." + mss + " sec";

	if (ct < 60000)
		return secs + " sec";

	QString ds = QString("%1").arg(d, 2, 10, QChar('0'));		// zero padding e.g. 01
	QString hs = QString("%1").arg(h, 2, 10, QChar('0'));		// zero padding e.g. 01;
	QString mins = QString("%1").arg(min, 2, 10, QChar('0'));	// zero padding e.g. 01;
	secs = QString("%1").arg(sec, 2, 10, QChar('0'));

	if (ct < 3600000)
		return mins + ":" + secs;
	if (d == 0)
		return hs + ":" + mins + ":" + secs;

	return ds + "days " + hs + ":" + mins + ":" + secs;
}

void Timer::start() {
	mTimer.restart();
}

int Timer::elapsed() const {
	return mTimer.elapsed();
}

}