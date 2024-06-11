cmake_minimum_required(VERSION 3.7 FATAL_ERROR)
project(examples)

#
list(APPEND EXAMPLES_LIST sftp)
add_executable (sftp EXCLUDE_FROM_ALL sftp.cpp)
target_link_libraries (sftp ${PROJECT_NAME_MASTER} ${LIBRARIES})

#
# list(APPEND EXAMPLES_LIST license)
# add_executable (license EXCLUDE_FROM_ALL license.cpp)
# target_link_libraries (license ${PROJECT_NAME_MASTER} ${LIBRARIES})

#
list(APPEND EXAMPLES_LIST counter)
add_executable (counter EXCLUDE_FROM_ALL counter.cpp)
target_link_libraries (counter ${PROJECT_NAME_MASTER} ${LIBRARIES})

#
# list(APPEND EXAMPLES_LIST cfg)
# add_executable (cfg EXCLUDE_FROM_ALL cfg.cpp)
# target_link_libraries (cfg ${PROJECT_NAME_MASTER} ${LIBRARIES})

#
list(APPEND EXAMPLES_LIST database)
add_executable (database EXCLUDE_FROM_ALL database.cpp)
target_include_directories(database PRIVATE ${ORACLE_INCLUDE_DIRS})
target_link_libraries (database ${PROJECT_NAME_MASTER})

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
# list(APPEND EXAMPLES_LIST logger)
# add_executable (logger EXCLUDE_FROM_ALL logger.cpp)
# target_link_libraries (logger ${PROJECT_NAME_MASTER} ${LIBRARIES})


# streambuf
# list(APPEND EXAMPLES_LIST stream)
# add_executable (stream EXCLUDE_FROM_ALL stream.cpp)
# target_link_libraries (stream ${PROJECT_NAME_MASTER} ${LIBRARIES})

# scylladb
list(APPEND EXAMPLES_LIST scylladb)
add_executable (scylladb EXCLUDE_FROM_ALL scylladb.cpp)
target_link_libraries (scylladb ${PROJECT_NAME_MASTER} ${LIBRARIES})

# kafka
list(APPEND EXAMPLES_LIST kafka)
add_executable (kafka EXCLUDE_FROM_ALL kafka.cpp)
target_link_libraries (kafka ${PROJECT_NAME_MASTER} ${LIBRARIES} boost_json)

# stream
list(APPEND EXAMPLES_LIST stream)
add_executable (stream EXCLUDE_FROM_ALL stream.cpp)
target_include_directories(stream PRIVATE ${ORACLE_INCLUDE_DIRS})
target_link_libraries (stream ${PROJECT_NAME_MASTER} ${LIBRARIES} ${ORACLE_LIBRARIES})

# rabbitmq
list(APPEND EXAMPLES_LIST rabbitmq)
add_executable (rabbitmq EXCLUDE_FROM_ALL rabbitmq.cpp)
target_link_libraries (rabbitmq ${PROJECT_NAME_MASTER} ${LIBRARIES})

## make example
add_custom_target(examples)
add_dependencies(examples ${EXAMPLES_LIST})
