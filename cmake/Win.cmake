# ReadFramework cmake file for a windows build

# load pathes from the user file if exists 
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/CMakeUser.txt)
	include(${CMAKE_CURRENT_SOURCE_DIR}/CMakeUser.txt)
endif()

# prepare build directory
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/libs)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Debug)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Release)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/ReallyRelease)

# these variables need to be set before adding subdirectory with projects
SET(CMAKE_SHARED_LINKER_FLAGS_REALLYRELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LARGEADDRESSAWARE")
set(CMAKE_CXX_FLAGS_REALLYRELEASE "-W3 -O2 -DQT_NO_DEBUG_OUTPUT")
set(CMAKE_C_FLAGS_REALLYRELEASE "-W3 -O2 -DQT_NO_DEBUG_OUTPUT")
set(RDF_BUILD_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
