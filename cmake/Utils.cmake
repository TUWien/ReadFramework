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
 
	set(RDF_REQUIRED_OPENCV_PACKAGES core imgproc stitching imgcodecs flann features2d calib3d)
	set(RDF_OPTIONAL_OPENCV_PACKAGES xfeatures2d)
	find_package(OpenCV REQUIRED ${RDF_REQUIRED_OPENCV_PACKAGES})# OPTIONAL_COMPONENTS ${RDF_OPTIONAL_OPENCV_PACKAGES}) 
	find_package(OpenCV QUIET OPTIONAL_COMPONENTS ${RDF_OPTIONAL_OPENCV_PACKAGES}) 
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

macro(RDF_ADD_INSTALL)

	if (MSVC)
		RDF_GENERATE_PACKAGE_XML()
	
		SET(RDF_INSTALL_DIRECTORY ${CMAKE_SOURCE_DIR}/install CACHE PATH "Path to the nomacs install directory for deploying")
	
		message(STATUS "${PROJECT_NAME} \t will be installed to: ${RDF_INSTALL_DIRECTORY}")
		
		set(PACKAGE_DIR ${RDF_INSTALL_DIRECTORY}/packages/plugins.${RDF_ARCHITECTURE}.${PROJECT_NAME})
		set(PACKAGE_DATA_DIR ${PACKAGE_DIR}/data/nomacs-${RDF_ARCHITECTURE}/plugins/)
		install(TARGETS ${PROJECT_NAME} ${RDF_DLL_MODULE_NAME} ${RDF_DLL_CORE_NAME} RUNTIME DESTINATION ${PACKAGE_DATA_DIR} CONFIGURATIONS Release)
		install(FILES ${CMAKE_CURRENT_BINARY_DIR}/package.xml DESTINATION ${PACKAGE_DIR}/meta CONFIGURATIONS Release)
	endif()
endmacro(RDF_ADD_INSTALL)

macro(RDF_GENERATE_PACKAGE_XML)

	string(TIMESTAMP CURRENT_DATE "%Y-%m-%d")	
	
	set(XML_CONTENT "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n")
	set(XML_CONTENT "${XML_CONTENT}<Package>\n")
	set(XML_CONTENT "${XML_CONTENT}\t<DisplayName>${PROJECT_NAME} [${RDF_ARCHITECTURE}]</DisplayName>\n")
	set(XML_CONTENT "${XML_CONTENT}\t<Description>READ Framework for ${RDF_ARCHITECTURE} systems (v${RDF_FRAMEWORK_VERSION}).</Description>\n")
	set(XML_CONTENT "${XML_CONTENT}\t<Version>${RDF_FRAMEWORK_VERSION}</Version>\n")
	set(XML_CONTENT "${XML_CONTENT}\t<ReleaseDate>${CURRENT_DATE}</ReleaseDate>\n")
	set(XML_CONTENT "${XML_CONTENT}\t<Default>true</Default>\n")
	set(XML_CONTENT "${XML_CONTENT}</Package>\n")
	
	file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/package.xml ${XML_CONTENT})
	
endmacro(RDF_GENERATE_PACKAGE_XML)
