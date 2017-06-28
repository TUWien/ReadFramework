set(RDF_RC src/rdf.rc) 

# create the targets
set(RDF_BINARY_NAME ${PROJECT_NAME})
set(RDF_TEST_NAME ${PROJECT_NAME}Test)
set(RDF_DLL_CORE_NAME ${PROJECT_NAME}Core)
set(RDF_DLL_MODULE_NAME ${PROJECT_NAME}Module)

#binary
link_directories(${OpenCV_LIBRARY_DIRS})
set(CHANGLOG_FILE ${CMAKE_CURRENT_SOURCE_DIR}/src/changelog.txt)
add_executable(${RDF_BINARY_NAME} WIN32  MACOSX_BUNDLE ${MAIN_SOURCES} ${MAIN_HEADERS} ${RDF_TRANSLATIONS} ${RDF_RC} ${CHANGLOG_FILE}) #changelog is added here, so that it appears in visual studio
set_source_files_properties(${CHANGLOG_FILE} PROPERTIES HEADER_FILE_ONLY TRUE) # define that changelog should not be compiled
target_link_libraries(${RDF_BINARY_NAME} ${RDF_DLL_CORE_NAME} ${RDF_DLL_MODULE_NAME} ${OpenCV_LIBS}) 
		
set_target_properties(${RDF_BINARY_NAME} PROPERTIES COMPILE_FLAGS "-DNOMINMAX")
set_target_properties(${RDF_BINARY_NAME} PROPERTIES LINK_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE}")
set_target_properties(${RDF_BINARY_NAME} PROPERTIES LINK_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG}")

# add core
add_library(${RDF_DLL_CORE_NAME} SHARED ${CORE_SOURCES} ${CORE_HEADERS} ${RDF_RC})
target_link_libraries(${RDF_DLL_CORE_NAME} ${OpenCV_LIBS}) 

# add module
add_library(
	${RDF_DLL_MODULE_NAME} SHARED 
	${MODULE_SOURCES} ${MODULE_HEADERS} 
	${GC_HEADERS} ${GC_SOURCES} 			# graphcut
	${LSD_HEADERS} ${LSD_SOURCES} 			# LSD 
	${RDF_RC})
target_link_libraries(${RDF_DLL_MODULE_NAME} ${RDF_DLL_CORE_NAME} ${OpenCV_LIBS} ${VERSION_LIB}) 

add_dependencies(${RDF_DLL_MODULE_NAME} ${RDF_DLL_CORE_NAME})
add_dependencies(${RDF_BINARY_NAME} ${RDF_DLL_MODULE_NAME} ${RDF_DLL_CORE_NAME}) 

target_include_directories(${RDF_BINARY_NAME}       PRIVATE ${OpenCV_INCLUDE_DIRS})
target_include_directories(${RDF_DLL_MODULE_NAME} 	PRIVATE ${OpenCV_INCLUDE_DIRS})
target_include_directories(${RDF_DLL_CORE_NAME} 	PRIVATE ${OpenCV_INCLUDE_DIRS})

qt5_use_modules(${RDF_BINARY_NAME} 		Core Network Widgets)
qt5_use_modules(${RDF_DLL_MODULE_NAME} 	Core Network Widgets)
qt5_use_modules(${RDF_DLL_CORE_NAME} 	Core Network Widgets)

# core flags
set_target_properties(${RDF_DLL_CORE_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR}/libs)
set_target_properties(${RDF_DLL_CORE_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_BINARY_DIR}/libs)

set_target_properties(${RDF_DLL_CORE_NAME} PROPERTIES COMPILE_FLAGS "-DDLL_CORE_EXPORT -DNOMINMAX")
set_target_properties(${RDF_DLL_CORE_NAME} PROPERTIES LINK_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE}")
set_target_properties(${RDF_DLL_CORE_NAME} PROPERTIES LINK_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG}")
set_target_properties(${RDF_DLL_CORE_NAME} PROPERTIES DEBUG_OUTPUT_NAME ${RDF_DLL_CORE_NAME}d)
set_target_properties(${RDF_DLL_CORE_NAME} PROPERTIES RELEASE_OUTPUT_NAME ${RDF_DLL_CORE_NAME})

# loader flags
set_target_properties(${RDF_DLL_MODULE_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR}/libs)
set_target_properties(${RDF_DLL_MODULE_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_BINARY_DIR}/libs)

set_target_properties(${RDF_DLL_MODULE_NAME} PROPERTIES COMPILE_FLAGS "-DDLL_MODULE_EXPORT -DNOMINMAX")
set_target_properties(${RDF_DLL_MODULE_NAME} PROPERTIES LINK_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE}")
set_target_properties(${RDF_DLL_MODULE_NAME} PROPERTIES LINK_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG}")
set_target_properties(${RDF_DLL_MODULE_NAME} PROPERTIES DEBUG_OUTPUT_NAME ${RDF_DLL_MODULE_NAME}d)
set_target_properties(${RDF_DLL_MODULE_NAME} PROPERTIES RELEASE_OUTPUT_NAME ${RDF_DLL_MODULE_NAME})

# installation
#  binary
install(TARGETS ${RDF_BINARY_NAME} ${RDF_DLL_MODULE_NAME} ${RDF_DLL_CORE_NAME} DESTINATION bin LIBRARY DESTINATION lib${LIB_SUFFIX})
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

# tests
add_executable(${RDF_TEST_NAME} WIN32  MACOSX_BUNDLE ${TEST_SOURCES} ${TEST_HEADERS} ${RDF_RC})
target_link_libraries(${RDF_TEST_NAME} ${RDF_DLL_CORE_NAME} ${RDF_DLL_MODULE_NAME} ${OpenCV_LIBS}) 
		
set_target_properties(${RDF_TEST_NAME} PROPERTIES COMPILE_FLAGS "-DNOMINMAX")
set_target_properties(${RDF_TEST_NAME} PROPERTIES LINK_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE}")
set_target_properties(${RDF_TEST_NAME} PROPERTIES LINK_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG}")

add_dependencies(${RDF_TEST_NAME} ${RDF_DLL_MODULE_NAME} ${RDF_DLL_CORE_NAME}) 

target_include_directories(${RDF_TEST_NAME}       PRIVATE ${OpenCV_INCLUDE_DIRS})
qt5_use_modules(${RDF_TEST_NAME} 		Core Network Widgets)

add_test(NAME BaselineTest COMMAND ${RDF_TEST_NAME} "--baseline")

# "make dist" target
string(TOLOWER ${PROJECT_NAME} CPACK_PACKAGE_NAME)
set(CPACK_PACKAGE_VERSION "${RDF_FRAMEWORK_VERSION}")
set(CPACK_SOURCE_GENERATOR "TBZ2")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}")
set(CPACK_IGNORE_FILES "/CVS/;/\\\\.svn/;/\\\\.git/;\\\\.swp$;\\\\.#;/#;\\\\.tar.gz$;/CMakeFiles/;CMakeCache.txt;refresh-copyright-and-license.pl;build;release;")
set(CPACK_SOURCE_IGNORE_FILES ${CPACK_IGNORE_FILES})
include(CPack)
# simulate autotools' "make dist"

if(DEFINED GLOBAL_READ_BUILD)
	if(NOT ${GLOBAL_READ_BUILD}) # cannot be incooperated into if one line above
		add_custom_target(dist COMMAND ${CMAKE_MAKE_PROGRAM} package_source)
	endif()
endif()

# generate configuration file
set(RDF_LIBS ${RDF_DLL_CORE_NAME} ${RDF_DLL_MODULE_NAME})
set(RDF_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(RDF_INCLUDE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/src/Core ${CMAKE_CURRENT_SOURCE_DIR}/src/Module ${CMAKE_BINARY_DIR})
set(RDF_BUILD_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
set(RDF_BINARIES ${CMAKE_CURRENT_BINARY_DIR}/lib${RDF_DLL_CORE_NAME}.so ${CMAKE_CURRENT_BINARY_DIR}/lib${RDF_DLL_MODULE_NAME}.so)

configure_file(${RDF_SOURCE_DIR}/ReadFramework.cmake.in ${CMAKE_BINARY_DIR}/ReadFrameworkConfig.cmake)
