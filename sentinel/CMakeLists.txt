cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

execute_process(
    COMMAND bash "-c" "cat /etc/passwd | grep sentinel | cut -d ':' -f6"
    OUTPUT_VARIABLE sentinel_HOME_PATH
)
string(REGEX REPLACE "\n$" "" sentinel_HOME_PATH "${sentinel_HOME_PATH}")
if (sentinel_HOME_PATH)
    message(STATUS "Application home folder: ${sentinel_HOME_PATH}")
endif()

project (sentinel VERSION 0.0.0 LANGUAGES C CXX)

add_executable(sentinel "")
include_directories(include)
add_subdirectory(src)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

target_compile_features(sentinel PUBLIC cxx_std_11)

#####

include_directories(${INCLUDE_DIRS})
set_property(TARGET sentinel PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)

link_libraries(${LIBRARIES})

target_include_directories (sentinel PUBLIC ${AXON_INCLUDE_DIR})
target_link_libraries(sentinel PRIVATE ${LIBRARIES} axon)

install(DIRECTORY DESTINATION /home/sentinel)
install(DIRECTORY DESTINATION ${sentinel_HOME_PATH}/logs)
install(DIRECTORY DESTINATION ${sentinel_HOME_PATH}/config)

install(TARGETS sentinel DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)