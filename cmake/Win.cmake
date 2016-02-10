# ReadFramework cmake file for a windows build

# load pathes from the user file if exists 
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/CMakeUser.txt)
	include(${CMAKE_CURRENT_SOURCE_DIR}/CMakeUser.txt)
endif()

# prepare build directory
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/libs)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/Debug)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/Release)

# these variables need to be set before adding subdirectory with projects
set(RDF_BUILD_DIRECTORY ${CMAKE_BINARY_DIR})
