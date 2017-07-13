
# create the targets
set(RDF_BINARY_NAME ${PROJECT_NAME})		# binary
set(RDF_TEST_NAME ${PROJECT_NAME}Test)		# test binary
set(RDF_DLL_CORE_NAME ${PROJECT_NAME}Core)	# library

if (MSVC)
	set(RDF_RC src/rdf.rc) #add resource file when compiling with MSVC 
	set(VERSION_LIB Version.lib)
	set(LIBRARY_DIR ${CMAKE_BINARY_DIR}/libs/) #add libs directory to build dir
	set(RDF_LIB_CORE_NAME optimized ${RDF_DLL_CORE_NAME}.lib debug ${RDF_DLL_CORE_NAME}d.lib)
else ()
	set(RDF_LIB_CORE_NAME ${RDF_DLL_CORE_NAME})	# forward declarem
endif()

#binary
link_directories(${OpenCV_LIBRARY_DIRS} ${LIBRARY_DIR})
set(CHANGLOG_FILE ${CMAKE_CURRENT_SOURCE_DIR}/src/changelog.txt)
add_executable(${RDF_BINARY_NAME} WIN32  MACOSX_BUNDLE ${MAIN_SOURCES} ${MAIN_HEADERS} ${RDF_TRANSLATIONS} ${RDF_RC} ${CHANGLOG_FILE}) #changelog is added here, so that i appears in visual studio
set_source_files_properties(${CHANGLOG_FILE} PROPERTIES HEADER_FILE_ONLY TRUE) # define that changelog should not be compiled

target_link_libraries(${RDF_BINARY_NAME} ${RDF_LIB_CORE_NAME} ${OpenCV_LIBS} ${VERSION_LIB}) 
set_target_properties(${RDF_BINARY_NAME} PROPERTIES COMPILE_FLAGS "-DNOMINMAX")

# add test target
add_executable(${RDF_TEST_NAME} WIN32  MACOSX_BUNDLE ${TEST_SOURCES} ${TEST_HEADERS} ${RDF_RC})
target_link_libraries(${RDF_TEST_NAME} ${RDF_DLL_CORE_NAME} ${RDF_DLL_MODULE_NAME} ${OpenCV_LIBS}) 
		
set_target_properties(${RDF_TEST_NAME} PROPERTIES COMPILE_FLAGS "-DNOMINMAX")
set_target_properties(${RDF_TEST_NAME} PROPERTIES LINK_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE}")
set_target_properties(${RDF_TEST_NAME} PROPERTIES LINK_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG}")

add_dependencies(${RDF_TEST_NAME} ${RDF_DLL_MODULE_NAME} ${RDF_DLL_CORE_NAME}) 

# add core
add_library(
	${RDF_DLL_CORE_NAME} SHARED 
	${CORE_SOURCES} ${CORE_HEADERS} 
	${MODULE_SOURCES} ${MODULE_HEADERS} 	# modules
	${GC_HEADERS} ${GC_SOURCES} 			# graph-cut
	${LSD_HEADERS} ${LSD_SOURCES} 			# LSD
	${RDF_RC}
	)
target_link_libraries(${RDF_DLL_CORE_NAME} ${VERSION_LIB} ${OpenCV_LIBS}) 

add_dependencies(${RDF_BINARY_NAME} ${RDF_DLL_CORE_NAME}) 

target_include_directories(${RDF_BINARY_NAME} 		PRIVATE ${OpenCV_INCLUDE_DIRS})
target_include_directories(${RDF_TEST_NAME} 	    PRIVATE ${OpenCV_INCLUDE_DIRS})
target_include_directories(${RDF_DLL_CORE_NAME} 	PRIVATE ${OpenCV_INCLUDE_DIRS})

qt5_use_modules(${RDF_BINARY_NAME} 		Core Network Widgets)
qt5_use_modules(${RDF_TEST_NAME} 		Core Network Widgets)
qt5_use_modules(${RDF_DLL_CORE_NAME} 	Core Network Widgets)

# core flags
set_target_properties(${RDF_DLL_CORE_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/libs/$<CONFIGURATION>)
set_target_properties(${RDF_DLL_CORE_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/libs/$<CONFIGURATION>)
set_target_properties(${RDF_DLL_CORE_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/libs/$<CONFIGURATION>)
set_target_properties(${RDF_DLL_CORE_NAME} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/libs/$<CONFIGURATION>)

set_target_properties(${RDF_DLL_CORE_NAME} PROPERTIES COMPILE_FLAGS "-DDLL_CORE_EXPORT -DNOMINMAX")
set_target_properties(${RDF_DLL_CORE_NAME} PROPERTIES DEBUG_OUTPUT_NAME ${RDF_DLL_CORE_NAME}d)
set_target_properties(${RDF_DLL_CORE_NAME} PROPERTIES RELEASE_OUTPUT_NAME ${RDF_DLL_CORE_NAME})

# make RelWithDebInfo link against release instead of debug opencv dlls
set_target_properties(${OpenCV_LIBS} PROPERTIES MAP_IMPORTED_CONFIG_RELWITHDEBINFO RELEASE)
set_target_properties(${OpenCV_LIBS} PROPERTIES MAP_IMPORTED_CONFIG_MINSIZEREL RELEASE)

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

if (MSVC)
	# set properties for Visual Studio Projects
	add_definitions(/Zc:wchar_t-)
	set(CMAKE_CXX_FLAGS_DEBUG "/W4 ${CMAKE_CXX_FLAGS_DEBUG}")
	set(CMAKE_CXX_FLAGS_RELEASE "/W4 /O2 ${CMAKE_CXX_FLAGS_RELEASE}")
	
	source_group("Generated Files" FILES ${RDF_RC} ${RDF_QM} ${RDF_AUTOMOC})
	source_group("graphcut" FILES ${GC_HEADERS} ${GC_SOURCES})
	source_group("LSD" FILES ${LSD_HEADERS} ${LSD_SOURCES})
	source_group("Changelog" FILES ${CHANGLOG_FILE})

	# set as console project 
	set_target_properties(${RDF_BINARY_NAME} PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE")
	set_target_properties(${RDF_BINARY_NAME} PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:CONSOLE")
	set_target_properties(${RDF_BINARY_NAME} PROPERTIES LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:CONSOLE")
	
	# set as console project 
	set_target_properties(${RDF_TEST_NAME} PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE")
	set_target_properties(${RDF_TEST_NAME} PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:CONSOLE")
	set_target_properties(${RDF_TEST_NAME} PROPERTIES LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:CONSOLE")

endif ()

file(GLOB RDF_AUTOMOC "${CMAKE_BINARY_DIR}/*_automoc.cpp")

# generate configuration file
if (RDF_DLL_CORE_NAME)
	get_property(CORE_DEBUG_NAME TARGET ${RDF_DLL_CORE_NAME} PROPERTY DEBUG_OUTPUT_NAME)
	get_property(CORE_RELEASE_NAME TARGET ${RDF_DLL_CORE_NAME} PROPERTY RELEASE_OUTPUT_NAME)
endif()

set(RDF_OPENCV_BINARIES ${RDF_OPENCV_BINARIES})	# just to show it goes into cmake.in
set(RDF_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(RDF_BUILD_DIRECTORY ${CMAKE_BINARY_DIR})
set(
	RDF_INCLUDE_DIRECTORY 
	${CMAKE_CURRENT_SOURCE_DIR}/src 
	${CMAKE_CURRENT_SOURCE_DIR}/src/Core 
	${CMAKE_CURRENT_SOURCE_DIR}/src/Module 
	${CMAKE_BINARY_DIR})

if (MSVC)
	set(
		RDF_BINARIES 
		${CMAKE_BINARY_DIR}/Debug/${CORE_DEBUG_NAME}.dll 
		${CMAKE_BINARY_DIR}/Release/${CORE_RELEASE_NAME}.dll
		)
	set(RDF_CORE_LIB optimized ${CMAKE_BINARY_DIR}/libs/Release/${CORE_RELEASE_NAME}.lib debug  ${CMAKE_BINARY_DIR}/libs/Debug/${CORE_DEBUG_NAME}.lib)

else ()
	set(RDF_BINARIES 
		${CMAKE_CURRENT_BINARY_DIR}/lib${RDF_DLL_CORE_NAME}.so 
		)
endif()

set(RDF_LIBS ${RDF_CORE_LIB})

configure_file(${RDF_SOURCE_DIR}/ReadFramework.cmake.in ${CMAKE_BINARY_DIR}/ReadFrameworkConfig.cmake)

# tests
add_test(NAME SuperPixel COMMAND ${RDF_TEST_NAME} "--super-pixel")
add_test(NAME BaselineTest COMMAND ${RDF_TEST_NAME} "--baseline")
add_test(NAME TableTest COMMAND ${RDF_TEST_NAME} "--table")

#package 
if (UNIX)

	# install
	install(TARGETS ${RDF_BINARY_NAME} ${RDF_DLL_MODULE_NAME} ${RDF_DLL_CORE_NAME} DESTINATION bin LIBRARY DESTINATION lib${LIB_SUFFIX})

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
elseif (MSVC)
	### DependencyCollector
	set(DC_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/cmake/DependencyCollector.py)
	set(DC_CONFIG ${CMAKE_BINARY_DIR}/DependencyCollector.ini)

	# CMAKE_MAKE_PROGRAM works for VS 2017 too
	get_filename_component(VS_PATH ${CMAKE_MAKE_PROGRAM} PATH)
	if(CMAKE_CL_64)
		set(VS_PATH "${VS_PATH}/../../../Common7/IDE/Remote Debugger/x64")
	else()
		set(VS_PATH "${VS_PATH}/../../Common7/IDE/Remote Debugger/x86")
	endif()
	set(DC_PATHS_RELEASE ${OpenCV_DIR}/bin/Release ${QT_QMAKE_PATH} ${VS_PATH})
	set(DC_PATHS_DEBUG ${OpenCV_DIR}/bin/Debug ${QT_QMAKE_PATH} ${VS_PATH})

	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/DependencyCollector.config.cmake.in ${DC_CONFIG})

	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND python ${DC_SCRIPT} --infile $<TARGET_FILE:${PROJECT_NAME}> --configfile ${DC_CONFIG} --configuration $<CONFIGURATION>)
endif()

