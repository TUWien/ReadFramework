set(RDF_RC src/rdf.rc) 
set(LIBRARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/libs) #add libs directory to build dir

# create the targets
set(BINARY_NAME ${CMAKE_PROJECT_NAME})
set(DLL_CORE_NAME ${CMAKE_PROJECT_NAME}Core)
set(DLL_MODULE_NAME ${CMAKE_PROJECT_NAME}Module)

#binary
link_directories(${OpenCV_LIBRARY_DIRS} ${LIBRARY_DIR})
set(CHANGLOG_FILE ${CMAKE_CURRENT_SOURCE_DIR}/src/changelog.txt)
add_executable(${BINARY_NAME} WIN32  MACOSX_BUNDLE ${MAIN_SOURCES} ${MAIN_HEADERS} ${RDF_TRANSLATIONS} ${RDF_RC} ${CHANGLOG_FILE}) #changelog is added here, so that i appears in visual studio
set_source_files_properties(${CHANGLOG_FILE} PROPERTIES HEADER_FILE_ONLY TRUE) # define that changelog should not be compiled
target_link_libraries(${BINARY_NAME} ${DLL_CORE_NAME} ${DLL_MODULE_NAME} ${OpenCV_LIBS}) 
		
set_target_properties(${BINARY_NAME} PROPERTIES COMPILE_FLAGS "-DNOMINMAX")
set_target_properties(${BINARY_NAME} PROPERTIES LINK_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE}")
set_target_properties(${BINARY_NAME} PROPERTIES LINK_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG}")

# add core
add_library(${DLL_CORE_NAME} SHARED ${CORE_SOURCES} ${CORE_HEADERS} ${RDF_RC})
target_link_libraries(${DLL_CORE_NAME} ${OpenCV_LIBS}) 

# add module
add_library(${DLL_MODULE_NAME} SHARED ${MODULE_SOURCES} ${MODULE_HEADERS} ${RDF_RC})
target_link_libraries(${DLL_MODULE_NAME} ${DLL_CORE_NAME} ${OpenCV_LIBS} ${VERSION_LIB}) 

add_dependencies(${DLL_MODULE_NAME} ${DLL_CORE_NAME})
add_dependencies(${BINARY_NAME} ${DLL_MODULE_NAME} ${DLL_CORE_NAME}) 

target_include_directories(${BINARY_NAME}       PRIVATE ${OpenCV_INCLUDE_DIRS})
target_include_directories(${DLL_MODULE_NAME} 	PRIVATE ${OpenCV_INCLUDE_DIRS})
target_include_directories(${DLL_CORE_NAME} 	PRIVATE ${OpenCV_INCLUDE_DIRS})

qt5_use_modules(${BINARY_NAME} 		Core Network Widgets)
qt5_use_modules(${DLL_MODULE_NAME} 	Core Network Widgets)
qt5_use_modules(${DLL_CORE_NAME} 	Core Network Widgets)

# core flags
set_target_properties(${DLL_CORE_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR}/libs)
set_target_properties(${DLL_CORE_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_BINARY_DIR}/libs)

set_target_properties(${DLL_CORE_NAME} PROPERTIES COMPILE_FLAGS "-DDLL_CORE_EXPORT -DNOMINMAX")
set_target_properties(${DLL_CORE_NAME} PROPERTIES LINK_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE}")
set_target_properties(${DLL_CORE_NAME} PROPERTIES LINK_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG}")
set_target_properties(${DLL_CORE_NAME} PROPERTIES DEBUG_OUTPUT_NAME ${DLL_CORE_NAME}d)
set_target_properties(${DLL_CORE_NAME} PROPERTIES RELEASE_OUTPUT_NAME ${DLL_CORE_NAME})

# loader flags
set_target_properties(${DLL_MODULE_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR}/libs)
set_target_properties(${DLL_MODULE_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_BINARY_DIR}/libs)

set_target_properties(${DLL_MODULE_NAME} PROPERTIES COMPILE_FLAGS "-DDLL_MODULE_EXPORT -DNOMINMAX")
set_target_properties(${DLL_MODULE_NAME} PROPERTIES LINK_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE}")
set_target_properties(${DLL_MODULE_NAME} PROPERTIES LINK_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG}")
set_target_properties(${DLL_MODULE_NAME} PROPERTIES DEBUG_OUTPUT_NAME ${DLL_MODULE_NAME}d)
set_target_properties(${DLL_MODULE_NAME} PROPERTIES RELEASE_OUTPUT_NAME ${DLL_MODULE_NAME})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unknown-pragmas")

# mac's bundle install
set_target_properties(${BINARY_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/macosx/Info.plist.in")
set(MACOSX_BUNDLE_ICON_FILE nomacs.icns)
set(MACOSX_BUNDLE_INFO_STRING "${BINARY_NAME} ${RDF_FRAMEWORK_VERSION}")
set(MACOSX_BUNDLE_GUI_IDENTIFIER "org.nomacs")
set(MACOSX_BUNDLE_LONG_VERSION_STRING "${RDF_FRAMEWORK_VERSION}")
set(MACOSX_BUNDLE_BUNDLE_NAME "${BINARY_NAME}")
set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${RDF_FRAMEWORK_VERSION}")
set(MACOSX_BUNDLE_BUNDLE_VERSION "${RDF_FRAMEWORK_VERSION}")
set(MACOSX_BUNDLE_COPYRIGHT "(c) CVL")
set_source_files_properties(${CMAKE_SOURCE_DIR}/macosx/nomacs.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

install(TARGETS ${BINARY_NAME} ${DLL_NAME} ${DLL_LOADER_NAME} ${DLL_CORE_NAME} BUNDLE DESTINATION ${CMAKE_INSTALL_PREFIX} LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX})

# create a "transportable" bundle - all libs into the bundle: "make bundle" after make install
configure_file(${CMAKE_SOURCE_DIR}/macosx/bundle.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/bundle.cmake @ONLY)
add_custom_target(bundle ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/bundle.cmake)

# generate configuration file
set(RDF_LIBS ${RDF_CORE_LIB} ${RDF_MODULE_LIB})
set(RDF_SOURCE_DIR ${CMAKE_SOURCE_DIR})
set(RDF_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/Core ${CMAKE_SOURCE_DIR}/src/Module ${CMAKE_BINARY_DIR})
set(RDF_BUILD_DIRECTORY ${CMAKE_BINARY_DIR})

configure_file(${RDF_SOURCE_DIR}/ReadFramework.cmake.in ${CMAKE_BINARY_DIR}/ReadFrameworkConfig.cmake)
