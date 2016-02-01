# check cmake requirements
cmake_minimum_required(VERSION 3.0)
# if(COMMAND cmake_policy)
	# cmake_policy(SET CMP0003 NEW)
	  
	# IF(CMAKE_VERSION VERSION_GREATER 2.8.10)
		# cmake_policy(SET CMP0020 NEW)	# qt warnings
	# ENDIF()
  # IF(CMAKE_VERSION VERSION_GREATER 2.8.12.9)
    # cmake_policy(SET CMP0043 NEW)	# COMPILE_DEFINITIONS_<CONFIG> warnings
  # ENDIF()
# endif(COMMAND cmake_policy)

######################## begin of project
project(ReadFramework)

set(RDF_FRAMEWORK_VERSION 0.1)
add_definitions(-DRDF_FRAMEWORK_VERSION="${RDF_FRAMEWORK_VERSION}")

# include macros needed
include("cmake/Utils.cmake")

# different compile options
option(DISABLE_QT_DEBUG "Disable Qt Debug Messages" OFF)

# load paths from the user file if exists 
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/CMakeUser.cmake)
	include(${CMAKE_CURRENT_SOURCE_DIR}/CMakeUser.cmake)
endif()

RDF_CHECK_COMPILER()

# find Qt
RDF_FIND_QT()

# find OpenCV
RDF_FIND_OPENCV()

if (DISABLE_QT_DEBUG)
	message (STATUS "disabling qt debug messages")
	add_definitions(-DQT_NO_DEBUG_OUTPUT)
endif()

if(MSVC)
	include(${CMAKE_SOURCE_DIR}/cmake/Win.cmake)
elseif(APPLE)
	include(${CMAKE_SOURCE_DIR}/cmake/Mac.cmake)
else(UNIX)
	include(${CMAKE_SOURCE_DIR}/cmake/Unix.cmake)
else()
	message(STATUS "build system unkown ... fallback to unix")
	include(${CMAKE_SOURCE_DIR}/cmake/Unix.cmake)
endif()

file(GLOB MAIN_SOURCES "src/*.cpp")
file(GLOB MAIN_HEADERS "src/*.h")

# loader
file(GLOB MODULE_SOURCES "src/Module/*.cpp")
file(GLOB MODULE_HEADERS "src/Module/*.h")

# core
file(GLOB CORE_SOURCES "src/Core/*.cpp")
file(GLOB CORE_HEADERS "src/Core/*.h")

# gather information for building
include_directories (
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_SOURCE_DIR}/src
	${CMAKE_SOURCE_DIR}/src/Core
	${CMAKE_SOURCE_DIR}/src/Module
)

set(RDF_RESOURCES
  src/RDF.qrc
)

QT5_ADD_RESOURCES(NOMACS_RCC ${RDF_RESOURCES})

# TODO: uncomment if we ever need translations
# file(GLOB RDF_TRANSLATIONS "translations/*.ts")
# QT5_ADD_TRANSLATION(RDF_QM ${RDF_TRANSLATIONS})

if(MSVC)
	include(${CMAKE_SOURCE_DIR}/cmake/WinBuildTarget.cmake)
else(UNIX)
	include(${CMAKE_SOURCE_DIR}/cmake/UnixBuildTarget.cmake)
else()
	message(STATUS "build system unkown ... fallback to unix")
	include(${CMAKE_SOURCE_DIR}/cmake/UnixBuildTarget.cmake)
endif()


#debug for printing out all variables 
# get_cmake_property(_variableNames VARIABLES)
# foreach (_variableName ${_variableNames})
#     message(STATUS "${_variableName}=${${_variableName}}")
# endforeach()
