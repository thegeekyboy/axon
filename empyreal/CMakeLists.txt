cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

execute_process(
    COMMAND bash "-c" "cat /etc/passwd | grep empyreal | cut -d ':' -f6"
    OUTPUT_VARIABLE empyreal_HOME_PATH
)
string(REGEX REPLACE "\n$" "" empyreal_HOME_PATH "${empyreal_HOME_PATH}")
if (empyreal_HOME_PATH)
    message(STATUS "Application home folder: ${empyreal_HOME_PATH}")
endif()

project (empyreal VERSION 0.0.0 LANGUAGES C CXX)

add_executable(empyreal "")
include_directories (include)
add_subdirectory(src)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

target_compile_features(empyreal PUBLIC cxx_std_11)

#####

include_directories(${INCLUDE_DIRS})
set_property(TARGET empyreal PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)

link_libraries(${LIBRARIES})
target_link_libraries(empyreal PRIVATE ${LIBRARIES} axon)

install(DIRECTORY DESTINATION /home/empyreal)
install(DIRECTORY DESTINATION ${empyreal_HOME_PATH}/logs)
install(DIRECTORY DESTINATION ${empyreal_HOME_PATH}/config)
install(TARGETS empyreal DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)