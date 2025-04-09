cmake_minimum_required(VERSION 3.26 FATAL_ERROR)

IF(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
	MESSAGE(FATAL_ERROR "cannot build the project in the source directory! Out-of-source build is enforced!")
ENDIF()

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

set(PROJECT_NAME "axon" CACHE INTERNAL "axon")
set(PROJECT_NAME_MASTER ${PROJECT_NAME})

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

message(STATUS "Starting build process for ${PROJECT_NAME} ${VERSION}")

project(${PROJECT_NAME} VERSION ${VERSION} DESCRIPTION "helper library for hyperion" LANGUAGES C CXX)

# add_library(${PROJECT_NAME} OBJECT)
add_library(${PROJECT_NAME} SHARED "")
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR})

## cmake helpers
include (GNUInstallDirs)

if (UNIX)
	include (CheckFunctionExists)
endif (UNIX)

## cmake sub configs
include (macros)
include (compiler)
include (definitions)
include (dependencies)
include (targets)
include (packaging)

## project files
add_subdirectory(src)
add_subdirectory(examples)
add_subdirectory(experiment)

target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_link_libraries(${PROJECT_NAME} PRIVATE ${LINK_LIBRARIES})
target_link_options(${PROJECT_NAME} PUBLIC ${LDFLAGS})

## installs
configure_file(extras/${PROJECT_NAME}.pc.in ${PROJECT_NAME}.pc @ONLY)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/)
install(FILES ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.pc DESTINATION ${CMAKE_INSTALL_DATADIR}/pkgconfig)
