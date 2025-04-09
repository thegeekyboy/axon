cmake_minimum_required(VERSION 3.7 FATAL_ERROR)
project(experiment)

#
list(APPEND EXPERIMENT_LIST e_ldap)
add_executable (e_ldap EXCLUDE_FROM_ALL ldap.cpp)
target_link_libraries (e_ldap ${PROJECT_NAME_MASTER} ${LIBRARIES})

#
list(APPEND EXPERIMENT_LIST inherit)
add_executable (inherit EXCLUDE_FROM_ALL inherit.cpp)
target_link_libraries (inherit ${PROJECT_NAME_MASTER} ${LIBRARIES})

## make example
add_custom_target(experiment)
add_dependencies(experiment ${EXPERIMENT_LIST})
