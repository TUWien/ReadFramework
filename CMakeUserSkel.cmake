# If you want to use prefix paths with cmake, copy and rename this file to CMakeUser.cmake
# Do not add this file to GIT!

# set your preferred library paths
IF (CMAKE_CL_64)
	SET(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "D:/Qt/qt-everywhere-opensource-src-5.8.0-x64/qtbase/bin/")
	SET(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "D:/coding/3rd-party/OpenCV/build2017-x64")

ELSE ()
	SET(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "D:/Qt/qt-everywhere-opensource-src-5.8.0-x86/qtbase/bin/")
	SET(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "D:/coding/3rd-party/OpenCV/build2017-x86")
ENDIF ()


