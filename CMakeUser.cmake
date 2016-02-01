# If you want to use prefix paths with cmake, copy and rename this file to CMakeUser.cmake
# Do not add this file to GIT!

if (MSVC11)
	# set your preferred Qt Library path
	IF (CMAKE_CL_64)
		SET(CMAKE_PREFIX_PATH "C:/Qt/qt5.4/5.4/msvc2012_opengl/bin/")
	ELSE ()
		SET(CMAKE_PREFIX_PATH "C:/Qt/qt5.4/5.4/msvc2012_opengl/bin/")
	ENDIF ()
ELSE ()
	# set your preferred Qt Library path
	IF (CMAKE_CL_64)
		SET(CMAKE_PREFIX_PATH "D:/Qt/qt-everywhere-opensource-src-5.5.1-x64/qtbase/bin/")
	ELSE ()
		SET(CMAKE_PREFIX_PATH "D:/Qt/qt-everywhere-opensource-src-5.5.1-x86/qtbase/bin/")
	ENDIF ()
ENDIF ()

# set your preferred OpenCV Library path
IF (CMAKE_CL_64)
	SET(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "C:/VSProjects/OpenCV3/build2015-x64")
ELSE ()
	SET(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "C:/VSProjects/OpenCV3/build2015-x86")
ENDIF ()
# set your preferred HUpnp path
IF (CMAKE_CL_64)
	SET(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "C:/VSProjects/nomacs-developer/herqq/build2015x64")
ELSE ()
	SET(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "C:/VSProjects/nomacs-developer/herqq/build2015x86")
ENDIF ()

# quazip path
IF (CMAKE_CL_64)
	SET(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "../quazip-0.7/build2015x64")
ELSE ()
	SET(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "../quazip-0.7/bild2015x86")
ENDIF ()