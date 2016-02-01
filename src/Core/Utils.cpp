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
#pragma warning(pop)

// needed for registering the file version
#ifdef WIN32
#include "shlwapi.h"
#pragma comment (lib, "shlwapi.lib")
#endif


namespace rdf {

// Utils --------------------------------------------------------------------
Utils::Utils() {
}

Utils& Utils::instance() { 

	static QSharedPointer<Utils> inst;
	if (!inst)
		inst = QSharedPointer<Utils>(new Utils());
	return *inst; 
}

void Utils::registerVersion() {

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
	std::vector<BYTE> bytes(dwSize);

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


}