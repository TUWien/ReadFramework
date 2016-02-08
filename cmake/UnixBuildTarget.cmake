set(RDF_RC src/rdf.rc) 

# create the targets
set(BINARY_NAME ${PROJECT_NAME})
set(DLL_CORE_NAME ${PROJECT_NAME}Core)
set(DLL_MODULE_NAME ${PROJECT_NAME}Module)

#binary
link_directories(${OpenCV_LIBRARY_DIRS})
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


# installation
#  binary
install(TARGETS ${BINARY_NAME} ${DLL_MODULE_NAME} ${DLL_CORE_NAME} DESTINATION bin LIBRARY DESTINATION lib${LIB_SUFFIX})
#  desktop file
#install(FILES nomacs.desktop DESTINATION share/applications)
#  icon
#install(FILES src/img/nomacs.svg DESTINATION share/pixmaps)
#  translations
#install(FILES ${NOMACS_QM} DESTINATION share/nomacs/translations)
#  manpage
#install(FILES Readme/nomacs.1 DESTINATION share/man/man1)
#  appdata
#install(FILES nomacs.appdata.xml DESTINATION /usr/share/appdata/)


# "make dist" target
string(TOLOWER ${PROJECT_NAME} CPACK_PACKAGE_NAME)
set(CPACK_PACKAGE_VERSION "${RDF_FRAMEWORK_VERSION}")
set(CPACK_SOURCE_GENERATOR "TBZ2")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}")
set(CPACK_IGNORE_FILES "/CVS/;/\\\\.svn/;/\\\\.git/;\\\\.swp$;\\\\.#;/#;\\\\.tar.gz$;/CMakeFiles/;CMakeCache.txt;refresh-copyright-and-license.pl;build;release;")
set(CPACK_SOURCE_IGNORE_FILES ${CPACK_IGNORE_FILES})
include(CPack)
# simulate autotools' "make dist"
add_custom_target(dist COMMAND ${CMAKE_MAKE_PROGRAM} package_source)


# generate configuration file
set(RDF_LIBS ${DLL_CORE_NAME} ${DLL_MODULE_NAME})
set(RDF_SOURCE_DIR ${CMAKE_SOURCE_DIR})
set(RDF_INCLUDE_DIRECTORY ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/Core ${CMAKE_SOURCE_DIR}/src/Module ${CMAKE_BINARY_DIR})
set(RDF_BUILD_DIRECTORY ${CMAKE_BINARY_DIR})
set(RDF_BINARIES ${CMAKE_BINARY_DIR}/lib${DLL_CORE_NAME}.so ${CMAKE_BINARY_DIR}/lib${DLL_MODULE_NAME}.so)

configure_file(${RDF_SOURCE_DIR}/ReadFramework.cmake.in ${CMAKE_BINARY_DIR}/ReadFrameworkConfig.cmake)
