# prepare build directory
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/libs)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/Debug)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/Release)

# these variables need to be set before adding subdirectory with projects
set(RDF_BUILD_DIRECTORY ${CMAKE_BINARY_DIR})
