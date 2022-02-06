cmake_minimum_required(VERSION 3.17 FATAL_ERROR)

project(axon VERSION 1.0.0 DESCRIPTION "axon helper library")

string(REGEX MATCH "([0-9]*)" _ ${PROJECT_VERSION})
set(PROJECT_VERSION_MAJOR ${CMAKE_MATCH_1})

add_library(axon STATIC "")

include_directories(include)
add_subdirectory(src)
add_subdirectory(test)

target_include_directories(axon PUBLIC ${INCLUDE_DIRS})
set_target_properties(axon PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR})
set(AXON_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include CACHE INTERNAL "axon")

target_compile_features(axon PUBLIC cxx_std_17)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

##############################################################################
configure_file(extras/axon.pc.in axon.pc @ONLY)
##############################################################################

# message(STATUS "INCLUDE_DIRS is set to ${INCLUDE_DIRS}")
# message(STATUS "LIBRARIES is set to ${LIBRARIES}")
# message(STATUS "CMAKE_INSTALL_LIBDIR is set to ${CMAKE_INSTALL_LIBDIR}")
# message(STATUS "CMAKE_INSTALL_INCLUDEDIR is set to ${CMAKE_INSTALL_INCLUDEDIR}")
# message(STATUS "CMAKE_BINARY_DIR is set to ${CMAKE_BINARY_DIR}")
# message(STATUS "CMAKE_INSTALL_DATAROOTDIR is set to ${CMAKE_INSTALL_DATAROOTDIR}")

# message(STATUS "axon = CMAKE_SYSTEM_PREFIX_PATH is set to ${CMAKE_SYSTEM_PREFIX_PATH}")
# message(STATUS "axon = INCLUDE_DIRS is set to ${INCLUDE_DIRS}")

include_directories(${INCLUDE_DIRS})
link_libraries(${LIBRARIES})
target_link_libraries(axon PRIVATE ${LIBRARIES})

install(TARGETS axon LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/axon)
install(FILES ${CMAKE_BINARY_DIR}/axon.pc DESTINATION ${CMAKE_INSTALL_DATADIR}/pkgconfig)