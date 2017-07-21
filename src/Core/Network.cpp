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

#include "Network.h"

#pragma warning(push, 0)	// no warnings from includes
#include <QNetworkProxyQuery>
#include <QNetworkReply>
#include <QEventLoop>
#include <QTcpSocket>
#pragma warning(pop)

namespace rdf {

/// <summary>
/// Downloads the contents of the path specified.
/// NOTE: this is not intended for UI puroposes
/// for it blocks while downloading
/// </summary>
/// <param name="path">The path.</param>
/// <returns></returns>
QByteArray net::download(const QString & url, bool* ok) {

	if (ok)
		*ok = false;

	if (!QUrl(url).isValid()) {
		return QByteArray();
	}
	//if (!urlExists(url)) {
	//	qWarning() << url << "is not available";
	//	return QByteArray();
	//}

	QNetworkAccessManager manager;

	// do we need to support global proxies?
	QNetworkProxyQuery npq(QUrl("http://www.nomacs.org"));
	QList<QNetworkProxy> listOfProxies = QNetworkProxyFactory::systemProxyForQuery(npq);

	if (!listOfProxies.empty() && listOfProxies[0].hostName() != "") {
		manager.setProxy(listOfProxies[0]);
	}

	QNetworkRequest request(url);
	QNetworkReply* nr = manager.get(request);

	if (!nr) {
		qCritical() << "QNetworkReply is unexpectedly NULL";
		return QByteArray();
	}

	// wait for finished - yes this is a blocking downloader
	QEventLoop loop;
	QObject::connect(nr, SIGNAL(finished()), &loop, SLOT(quit()));
	loop.exec();

	QByteArray ba = nr->readAll();
	nr->deleteLater();
	
	if (ok)
		*ok = !ba.isEmpty();

	// TODO: check for non-existing files 
	//if (ba.isEmpty())
	//	qWarning() << nr->errorString();

	return ba;
}

}