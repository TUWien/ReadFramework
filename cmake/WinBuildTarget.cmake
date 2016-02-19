set(RDF_RC src/rdf.rc) #add resource file when compiling with MSVC 
set(VERSION_LIB Version.lib)
set(LIBRARY_DIR ${CMAKE_BINARY_DIR}/libs) #add libs directory to build dir

# create the targets
set(RDF_BINARY_NAME ${PROJECT_NAME})
set(RDF_DLL_CORE_NAME ${PROJECT_NAME}Core)
set(RDF_DLL_MODULE_NAME ${PROJECT_NAME}Module)

set(RDF_LIB_CORE_NAME optimized ${RDF_DLL_CORE_NAME}.lib debug ${RDF_DLL_CORE_NAME}d.lib)
set(RDF_LIB_MODULE_NAME optimized ${RDF_DLL_MODULE_NAME}.lib debug ${RDF_DLL_MODULE_NAME}d.lib)
set(RDF_LIB_NAME optimized ${DLL_GUI_NAME}.lib debug ${DLL_GUI_NAME}d.lib)

#binary
link_directories(${OpenCV_LIBRARY_DIRS} ${LIBRARY_DIR})
set(CHANGLOG_FILE ${CMAKE_CURRENT_SOURCE_DIR}/src/changelog.txt)
add_executable(${RDF_BINARY_NAME} WIN32  MACOSX_BUNDLE ${MAIN_SOURCES} ${MAIN_HEADERS} ${RDF_TRANSLATIONS} ${RDF_RC} ${CHANGLOG_FILE}) #changelog is added here, so that i appears in visual studio
set_source_files_properties(${CHANGLOG_FILE} PROPERTIES HEADER_FILE_ONLY TRUE) # define that changelog should not be compiled

target_link_libraries(${RDF_BINARY_NAME} ${RDF_LIB_CORE_NAME} ${RDF_LIB_MODULE_NAME} ${OpenCV_LIBS} ${VERSION_LIB}) 
set_target_properties(${RDF_BINARY_NAME} PROPERTIES COMPILE_FLAGS "-DNOMINMAX")

# add core
add_library(${RDF_DLL_CORE_NAME} SHARED ${CORE_SOURCES} ${CORE_HEADERS} ${RDF_RC})
target_link_libraries(${RDF_DLL_CORE_NAME} ${VERSION_LIB} ${OpenCV_LIBS}) 

# add module
add_library(${RDF_DLL_MODULE_NAME} SHARED ${MODULE_SOURCES} ${MODULE_HEADERS} ${RDF_RC})
target_link_libraries(${RDF_DLL_MODULE_NAME} ${RDF_LIB_CORE_NAME} ${OpenCV_LIBS} ${VERSION_LIB}) 

add_dependencies(${RDF_DLL_MODULE_NAME} ${RDF_DLL_CORE_NAME})
add_dependencies(${RDF_BINARY_NAME} ${RDF_DLL_MODULE_NAME} ${RDF_DLL_CORE_NAME}) 

target_include_directories(${RDF_BINARY_NAME} 		PRIVATE ${OpenCV_INCLUDE_DIRS})
target_include_directories(${RDF_DLL_MODULE_NAME} 	PRIVATE ${OpenCV_INCLUDE_DIRS})
target_include_directories(${RDF_DLL_CORE_NAME} 	PRIVATE ${OpenCV_INCLUDE_DIRS})

qt5_use_modules(${RDF_BINARY_NAME} 		Core Network Widgets)
qt5_use_modules(${RDF_DLL_MODULE_NAME} 	Core Network Widgets)
qt5_use_modules(${RDF_DLL_CORE_NAME} 	Core Network Widgets)

# core flags
set_target_properties(${RDF_DLL_CORE_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/libs)
set_target_properties(${RDF_DLL_CORE_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/libs)

set_target_properties(${RDF_DLL_CORE_NAME} PROPERTIES COMPILE_FLAGS "-DDLL_CORE_EXPORT -DNOMINMAX")
set_target_properties(${RDF_DLL_CORE_NAME} PROPERTIES DEBUG_OUTPUT_NAME ${RDF_DLL_CORE_NAME}d)
set_target_properties(${RDF_DLL_CORE_NAME} PROPERTIES RELEASE_OUTPUT_NAME ${RDF_DLL_CORE_NAME})

# loader flags
set_target_properties(${RDF_DLL_MODULE_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/libs)
set_target_properties(${RDF_DLL_MODULE_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/libs)

set_target_properties(${RDF_DLL_MODULE_NAME} PROPERTIES COMPILE_FLAGS "-DDLL_MODULE_EXPORT -DNOMINMAX")
set_target_properties(${RDF_DLL_MODULE_NAME} PROPERTIES DEBUG_OUTPUT_NAME ${RDF_DLL_MODULE_NAME}d)
set_target_properties(${RDF_DLL_MODULE_NAME} PROPERTIES RELEASE_OUTPUT_NAME ${RDF_DLL_MODULE_NAME})

# copy required dlls to the directories
set(OpenCV_MODULE_NAMES core imgproc stitching)
foreach(opencvlib ${OpenCV_MODULE_NAMES})
	file(GLOB dllpath ${OpenCV_DIR}/bin/Release/opencv_${opencvlib}*.dll)
	file(COPY ${dllpath} DESTINATION ${CMAKE_BINARY_DIR}/Release)
	
	set(RDF_OPENCV_BINARIES ${RDF_OPENCV_BINARIES} ${dllpath})
	
	file(GLOB dllpath ${OpenCV_DIR}/bin/Debug/opencv_${opencvlib}*d.dll)
	file(COPY ${dllpath} DESTINATION ${CMAKE_BINARY_DIR}/Debug)
	file(COPY ${dllpath} DESTINATION ${CMAKE_BINARY_DIR}/RelWithDebInfo)
	
	set(RDF_OPENCV_BINARIES ${RDF_OPENCV_BINARIES} ${dllpath})
	
	message(STATUS "copying ${dllpath} to ${CMAKE_BINARY_DIR}/Debug")
endforeach(opencvlib)

set(QTLIBLIST Qt5Core Qt5Gui Qt5Network Qt5Widgets Qt5PrintSupport Qt5Concurrent Qt5Svg)
foreach(qtlib ${QTLIBLIST})
	get_filename_component(QT_DLL_PATH_tmp ${QT_QMAKE_EXECUTABLE} PATH)
	file(COPY ${QT_DLL_PATH_tmp}/${qtlib}.dll DESTINATION ${CMAKE_BINARY_DIR}/Release)
	file(COPY ${QT_DLL_PATH_tmp}/${qtlib}.dll DESTINATION ${CMAKE_BINARY_DIR}/RelWithDebInfo)
	file(COPY ${QT_DLL_PATH_tmp}/${qtlib}d.dll DESTINATION ${CMAKE_BINARY_DIR}/Debug)
endforeach(qtlib)

# create settings file for portable version while working
if(NOT EXISTS ${CMAKE_BINARY_DIR}/Release/rdf-settings.nfo)
	file(WRITE ${CMAKE_BINARY_DIR}/Release/rdf-settings.nfo "")
endif()
if(NOT EXISTS ${CMAKE_BINARY_DIR}/RelWithDebInfo/rdf-settings.nfo)
	file(WRITE ${CMAKE_BINARY_DIR}/RelWithDebInfo/rdf-settings.nfo "")
endif()
if(NOT EXISTS ${CMAKE_BINARY_DIR}/Debug/rdf-settings.nfo)
	file(WRITE ${CMAKE_BINARY_DIR}/Debug/rdf-settings.nfo "")
endif()

# # copy translation files after each build
# add_custom_command(TARGET ${RDF_BINARY_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory \"${CMAKE_BINARY_DIR}/$<CONFIGURATION>/translations/\")
# foreach(QM ${NOMACS_QM})
	# add_custom_command(TARGET ${RDF_BINARY_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy \"${QM}\" \"${CMAKE_BINARY_DIR}/$<CONFIGURATION>/translations/\")
# endforeach(QM)

# set properties for Visual Studio Projects
add_definitions(/Zc:wchar_t-)
set(CMAKE_CXX_FLAGS_DEBUG "/W4 ${CMAKE_CXX_FLAGS_DEBUG}")
set(CMAKE_CXX_FLAGS_RELEASE "/W4 /O2 ${CMAKE_CXX_FLAGS_RELEASE}")

file(GLOB RDF_AUTOMOC "${CMAKE_BINARY_DIR}/*_automoc.cpp")
source_group("Generated Files" FILES ${RDF_RC} ${RDF_QM} ${RDF_AUTOMOC})
# set_source_files_properties(${NOMACS_TRANSLATIONS} PROPERTIES HEADER_FILE_ONLY TRUE)
# source_group("Translations" FILES ${NOMACS_TRANSLATIONS})
source_group("Changelog" FILES ${CHANGLOG_FILE})

# generate configuration file
if (RDF_DLL_CORE_NAME)
	get_property(CORE_DEBUG_NAME TARGET ${RDF_DLL_CORE_NAME} PROPERTY DEBUG_OUTPUT_NAME)
	get_property(CORE_RELEASE_NAME TARGET ${RDF_DLL_CORE_NAME} PROPERTY RELEASE_OUTPUT_NAME)
	set(RDF_CORE_LIB optimized ${CMAKE_BINARY_DIR}/libs/${CORE_RELEASE_NAME}.lib debug  ${CMAKE_BINARY_DIR}/libs/${CORE_DEBUG_NAME}.lib)
endif()
if(RDF_DLL_MODULE_NAME)
	get_property(MODULE_DEBUG_NAME TARGET ${RDF_DLL_MODULE_NAME} PROPERTY DEBUG_OUTPUT_NAME)
	get_property(MODULE_RELEASE_NAME TARGET ${RDF_DLL_MODULE_NAME} PROPERTY RELEASE_OUTPUT_NAME)
	set(RDF_MODULE_LIB optimized ${CMAKE_BINARY_DIR}/libs/${MODULE_RELEASE_NAME}.lib debug  ${CMAKE_BINARY_DIR}/libs/${MODULE_DEBUG_NAME}.lib)
endif()

set(RDF_OPENCV_BINARIES ${RDF_OPENCV_BINARIES})	# just to show it goes into cmake.in
set(RDF_LIBS ${RDF_CORE_LIB} ${RDF_MODULE_LIB})
set(RDF_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(RDF_INCLUDE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/src/Core ${CMAKE_CURRENT_SOURCE_DIR}/src/Module ${CMAKE_BINARY_DIR})
set(RDF_BUILD_DIRECTORY ${CMAKE_BINARY_DIR})
set(RDF_BINARIES ${CMAKE_BINARY_DIR}/Debug/${MODULE_DEBUG_NAME}.dll ${CMAKE_BINARY_DIR}/Release/${MODULE_RELEASE_NAME}.dll ${CMAKE_BINARY_DIR}/Debug/${CORE_DEBUG_NAME}.dll ${CMAKE_BINARY_DIR}/Release/${CORE_RELEASE_NAME}.dll )
configure_file(${RDF_SOURCE_DIR}/ReadFramework.cmake.in ${CMAKE_BINARY_DIR}/ReadFrameworkConfig.cmake)