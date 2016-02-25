# Searches for Qt with the required components
macro(RDF_FIND_QT)
	
	set(CMAKE_AUTOMOC ON)
	set(CMAKE_AUTORCC OFF)
	
	set(CMAKE_INCLUDE_CURRENT_DIR ON)
	if(NOT QT_QMAKE_EXECUTABLE)
		find_program(QT_QMAKE_EXECUTABLE NAMES "qmake" "qmake-qt5" "qmake.exe")
	endif()
	if(NOT QT_QMAKE_EXECUTABLE)
		message(FATAL_ERROR "you have to set the path to the Qt5 qmake executable")
	endif()
	message(STATUS "QMake found: path: ${QT_QMAKE_EXECUTABLE}")
	GET_FILENAME_COMPONENT(QT_QMAKE_PATH ${QT_QMAKE_EXECUTABLE} PATH)
	set(QT_ROOT ${QT_QMAKE_PATH}/)
	SET(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${QT_QMAKE_PATH}\\..\\lib\\cmake\\Qt5)
	find_package(Qt5 REQUIRED Core Network LinguistTools)
	if (NOT Qt5_FOUND)
		message(FATAL_ERROR "Qt5 not found. Check your QT_QMAKE_EXECUTABLE path and set it to the correct location")
	endif()
	add_definitions(-DQT5)
endmacro(RDF_FIND_QT)

# add OpenCV dependency
macro(RDF_FIND_OPENCV)
	
	# search for opencv
	unset(OpenCV_LIB_DIR_DBG CACHE)
	unset(OpenCV_3RDPARTY_LIB_DIR_DBG CACHE)
	unset(OpenCV_3RDPARTY_LIB_DIR_OPT CACHE)
	unset(OpenCV_CONFIG_PATH CACHE)
	unset(OpenCV_LIB_DIR_DBG CACHE)
	unset(OpenCV_LIB_DIR_OPT CACHE)
	unset(OpenCV_LIBRARY_DIRS CACHE)
	unset(OpenCV_DIR)
 
	find_package(OpenCV REQUIRED core imgproc stitching imgcodecs flann features2d calib3d ximgproc)
 
	if(NOT OpenCV_FOUND)
	 message(FATAL_ERROR "OpenCV not found.") 
	else()
	 add_definitions(-DWITH_OPENCV)
	endif()
 
	# unset include directories since OpenCV sets them global
	get_property(the_include_dirs  DIRECTORY . PROPERTY INCLUDE_DIRECTORIES)
	list(REMOVE_ITEM the_include_dirs ${OpenCV_INCLUDE_DIRS})
	set_property(DIRECTORY . PROPERTY INCLUDE_DIRECTORIES ${the_include_dirs})
 
endmacro(RDF_FIND_OPENCV)

# check if the c++ compiler supports c++11 (our code uses parts of this standard)
macro(RDF_CHECK_COMPILER)
	
	include(CheckCXXCompilerFlag)
	
	CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
	CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
	
	# check if c++11 is supported
	if(COMPILER_SUPPORTS_CXX11)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
	elseif(COMPILER_SUPPORTS_CXX0X)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
	elseif(!MSVC)
		message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
	endif()
 
	if(DEFINED GLOBAL_READ_BUILD)
		#  this is a complete build of the READ framework, thus we need to set compiler/linker settings for ReallyRelease
		set(CMAKE_CXX_FLAGS_REALLYRELEASE "${CMAKE_CXX_FLAGS_RELEASE}  /DQT_NO_DEBUG_OUTPUT") 
		set(CMAKE_EXE_LINKER_FLAGS_REALLYRELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /SUBSYSTEM:WINDOWS /LARGEADDRESSAWARE") # /subsystem:windows does not work due to a bug in cmake (see http://public.kitware.com/Bug/view.php?id=12566)
	endif()
endmacro(RDF_CHECK_COMPILER)