# prepare build directory
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/libs)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/Debug)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/Release)

# these variables need to be set before adding subdirectory with projects
set(RDF_BUILD_DIRECTORY ${CMAKE_BINARY_DIR})

# Platforms
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/Release/platforms)
file(COPY ${QT_QMAKE_PATH}}/../plugins/platforms/qwindows.dll DESTINATION ${CMAKE_BINARY_DIR}/Release/platforms/)
file(COPY ${QT_QMAKE_PATH}}/../plugins/platforms/qwindows.dll DESTINATION ${CMAKE_BINARY_DIR}/RelWithDebInfo/platforms/)
file(COPY ${QT_QMAKE_PATH}}/../plugins/platforms/qwindowsd.dll DESTINATION ${CMAKE_BINARY_DIR}/Debug/platforms/)

# add image plugins
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/Release/imageformats)
file(GLOB QT_IMAGE_FORMATS "${QT_QMAKE_PATH}}/../plugins/imageformats/*.dll")
file(COPY ${QT_IMAGE_FORMATS} DESTINATION ${CMAKE_BINARY_DIR}/Release/imageformats PATTERN *d.dll EXCLUDE)
file(COPY ${QT_IMAGE_FORMATS} DESTINATION ${CMAKE_BINARY_DIR}/RelWithDebInfo/imageformats PATTERN *d.dll EXCLUDE)
file(COPY ${QT_IMAGE_FORMATS} DESTINATION ${CMAKE_BINARY_DIR}/Debug/imageformats)

file(GLOB QT_EXTRA_IMAGE_FORMATS "${QT_QMAKE_PATH}}/../../qtimageformats/plugins/imageformats/*.dll")
file(COPY ${QT_EXTRA_IMAGE_FORMATS} DESTINATION ${CMAKE_BINARY_DIR}/Release/imageformats PATTERN *d.dll EXCLUDE)
file(COPY ${QT_EXTRA_IMAGE_FORMATS} DESTINATION ${CMAKE_BINARY_DIR}/RelWithDebInfo/imageformats PATTERN *d.dll EXCLUDE)
file(COPY ${QT_EXTRA_IMAGE_FORMATS} DESTINATION ${CMAKE_BINARY_DIR}/Debug/imageformats)
