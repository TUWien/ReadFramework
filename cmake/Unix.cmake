# ReadFramework cmake file for a Unix/Linux build


if (CMAKE_BUILD_TYPE STREQUAL "debug" OR CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "DEBUG")
	message(STATUS "A debug build. -DDEBUG is defined")
	add_definitions(-DDEBUG)
else()
	message(STATUS "A release build (non-debug). Debugging outputs are silently ignored.")
	add_definitions(-DQT_NO_DEBUG_OUTPUT)
	add_definitions(-DNDEBUG)
endif()
