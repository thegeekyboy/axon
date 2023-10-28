cmake_minimum_required(VERSION 3.7 FATAL_ERROR)

IF(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
	MESSAGE(FATAL_ERROR "cannot build the project in the source directory! Out-of-source build is enforced!")
ENDIF()

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

set(PROJECT_NAME "axon" CACHE INTERNAL "axon")

# version extraction
execute_process(
	COMMAND bash "-c" "awk -F'[.-]' '{printf(\"%d.%d.%d\",$1,$2,$3)}' ${CMAKE_CURRENT_SOURCE_DIR}/version"
	OUTPUT_VARIABLE VERSION
)
execute_process(
	COMMAND bash "-c" "awk -F'[.-]' '{printf(\"%d.%d.%d-%d\",$1,$2,$3,$4)}' ${CMAKE_CURRENT_SOURCE_DIR}/version"
	OUTPUT_VARIABLE FULL_VERSION
)
string(REGEX MATCH "([0-9]*)\.([0-9]*)\.([0-9]*)\-([0-9]*)" _ ${FULL_VERSION})
set(PROJECT_VERSION_MAJOR ${CMAKE_MATCH_1})
set(PROJECT_VERSION_MINOR ${CMAKE_MATCH_2})
set(PROJECT_VERSION_PATCH ${CMAKE_MATCH_3})
set(PROJECT_VERSION_BUILD ${CMAKE_MATCH_4})
message(STATUS "Starting build process for ${APP_PROJECT_NAME} ${VERSION}")

project(${PROJECT_NAME} VERSION ${VERSION} DESCRIPTION "helper library for hyperion" LANGUAGES C CXX)

include(GNUInstallDirs)

include(macros)
include(compiler)
include(definitions)

if (UNIX)
  include(CheckFunctionExists)
endif (UNIX)

include (dependencies)
include (packaging)

add_library(${PROJECT_NAME} SHARED "")
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR})

add_subdirectory(src)
add_subdirectory(test)

target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_link_libraries(${PROJECT_NAME} PRIVATE ${LINK_LIBRARIES})
target_link_options(${PROJECT_NAME} PUBLIC ${LDFLAGS})

configure_file(extras/${PROJECT_NAME}.pc.in ${PROJECT_NAME}.pc @ONLY)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/)
install(FILES ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.pc DESTINATION ${CMAKE_INSTALL_DATADIR}/pkgconfig)

## make uninstall
add_custom_target("uninstall" COMMENT "Uninstall installed files")
add_custom_command(
	TARGET "uninstall"
	POST_BUILD
	COMMENT "Uninstall files with install_manifest.txt"
	COMMAND xargs rm -vf < install_manifest.txt || echo Nothing in install_manifest.txt to be uninstalled!
)

## version management
add_custom_target(
	"version"
	COMMAND awk -F'[.-]' '{ printf \"%d.%d.%d-%d\" , $$1, $$2, $$3, $$4+1 > \"${CMAKE_CURRENT_SOURCE_DIR}/version\"}' ${CMAKE_CURRENT_SOURCE_DIR}/version
	# COMMAND awk -F'[.-]' '{ printf \"\#define VERSION \\"%d.%d.%d-%d\\"\\n\" , $$1, $$2, $$3, $$4 > \"${CMAKE_CURRENT_SOURCE_DIR}/include/version.h\"}' ${CMAKE_CURRENT_SOURCE_DIR}/version
)
add_dependencies(${PROJECT_NAME} "version")

## make bump
add_custom_target("bump" COMMENT "bumping minor version")
add_custom_command(
	TARGET "bump"
	POST_BUILD
	COMMENT "bumping minor version"
	COMMAND awk -F'[.-]' '{ printf \"%d.%d.%d-%d\" , $$1, $$2, $$3+1, $$4 > \"${CMAKE_CURRENT_SOURCE_DIR}/version\"}' ${CMAKE_CURRENT_SOURCE_DIR}/version
)
##