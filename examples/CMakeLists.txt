cmake_minimum_required(VERSION 3.7 FATAL_ERROR)
project(examples)

#
list(APPEND EXAMPLES_LIST sftp)
add_executable (sftp EXCLUDE_FROM_ALL sftp.cpp)
target_link_libraries (sftp ${PROJECT_NAME_MASTER} ${LIBRARIES})

#
list(APPEND EXAMPLES_LIST license)
add_executable (license EXCLUDE_FROM_ALL license.cpp)
target_link_libraries (license ${PROJECT_NAME_MASTER} ${LIBRARIES})

#
list(APPEND EXAMPLES_LIST ftp)
add_executable (ftp EXCLUDE_FROM_ALL sftp.cpp)
target_link_libraries (ftp ${PROJECT_NAME_MASTER} ${LIBRARIES})

#
list(APPEND EXAMPLES_LIST cfg)
add_executable (cfg EXCLUDE_FROM_ALL cfg.cpp)
target_link_libraries (cfg ${PROJECT_NAME_MASTER} ${LIBRARIES})

#
#list(APPEND EXAMPLES_LIST cfg2)
#add_executable (cfg2 cfg2.cpp)
#target_link_libraries (cfg2 ${PROJECT_NAME_MASTER})

#
list(APPEND EXAMPLES_LIST read_topic)
add_executable (read_topic EXCLUDE_FROM_ALL read_topic.cpp)
target_link_libraries (read_topic ${PROJECT_NAME_MASTER} ${LIBRARIES})

#
list(APPEND EXAMPLES_LIST smb)
add_executable (smb EXCLUDE_FROM_ALL smb.cpp)
target_link_libraries (smb ${PROJECT_NAME_MASTER} ${LIBRARIES})

#
list(APPEND EXAMPLES_LIST krb5)
add_executable (krb5 EXCLUDE_FROM_ALL krb5.cpp)
target_link_libraries (krb5 ${PROJECT_NAME_MASTER} ${LIBRARIES})

#
list(APPEND EXAMPLES_LIST connector)
add_executable (connector EXCLUDE_FROM_ALL connector.cpp)
target_link_libraries (connector ${PROJECT_NAME_MASTER} ${LIBRARIES})

#
list(APPEND EXAMPLES_LIST mail)
add_executable (mail EXCLUDE_FROM_ALL mail.cpp)
target_link_libraries (mail ${PROJECT_NAME_MASTER} ${LIBRARIES})

#
list(APPEND EXAMPLES_LIST logger)
add_executable (logger EXCLUDE_FROM_ALL logger.cpp)
target_link_libraries (logger ${PROJECT_NAME_MASTER} ${LIBRARIES})

## make example
add_custom_target(examples)
add_dependencies(examples ${EXAMPLES_LIST})