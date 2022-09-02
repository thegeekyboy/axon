cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

execute_process(
    COMMAND bash "-c" "cat /etc/passwd | grep hyperion | cut -d ':' -f6"
    OUTPUT_VARIABLE SENTINEL_HOME_PATH
)
string(REGEX REPLACE "\n$" "" SENTINEL_HOME_PATH "${SENTINEL_HOME_PATH}")
if (hyperion_HOME_PATH)
    message(STATUS "Application home folder: ${SENTINEL_HOME_PATH}")
endif()

project (hyperion VERSION 0.0.0 LANGUAGES C CXX)

add_executable(hyperion "")
include_directories(include)
add_subdirectory(src)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# target_compile_features(hyperion PUBLIC cxx_std_11)

#####

include_directories(${INCLUDE_DIRS})
set_property(TARGET hyperion PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)

link_libraries(${LIBRARIES})

target_include_directories (hyperion PUBLIC ${AXON_INCLUDE_DIR} ${INCLUDE_DIRS})
target_link_libraries(hyperion PRIVATE axon)

install(DIRECTORY DESTINATION ${SENTINEL_HOME_PATH})
install(DIRECTORY DESTINATION ${SENTINEL_HOME_PATH}/logs)
install(DIRECTORY DESTINATION ${SENTINEL_HOME_PATH}/config)

install(TARGETS hyperion DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)