cmake_minimum_required(VERSION 3.17 FATAL_ERROR)

set(PROJECT_NAME "axon" CACHE INTERNAL "axon")
set(VERSION "2.1.7")
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

project(${PROJECT_NAME} VERSION ${VERSION} DESCRIPTION "helper library for hyperion" LANGUAGES C CXX)

IF(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    MESSAGE(FATAL_ERROR "cannot build the project in the source directory! Out-of-source build is enforced!")
ENDIF()

include(GNUInstallDirs)
include(CheckCXXCompilerFlag)

function(enable_cxx_compiler_flag_if_supported flag)
    string(FIND "${CMAKE_CXX_FLAGS}" "${flag}" flag_already_set)
    if(flag_already_set EQUAL -1)
        check_cxx_compiler_flag("${flag}" flag_supported)
        if(flag_supported)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}" PARENT_SCOPE)
        endif()
        unset(flag_supported CACHE)
    endif()
endfunction()

enable_cxx_compiler_flag_if_supported("-Wall")
enable_cxx_compiler_flag_if_supported("-Wextra")
enable_cxx_compiler_flag_if_supported("-pedantic")

string(REGEX MATCH "([0-9]*)" _ ${PROJECT_VERSION})
set(PROJECT_VERSION_MAJOR ${CMAKE_MATCH_1})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(GCC_COVERAGE_COMPILE_FLAGS "-D_GLIBCXX_USE_NANOSLEEP -Wall")

SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Debug)
endif()

IF(CMAKE_BUILD_TYPE MATCHES Debug)
	message(STATUS "Configuring cmake for Debug build")
	add_definitions(-DDEBUG)
ELSEIF(CMAKE_BUILD_TYPE MATCHES Release)
message(STATUS "Configuring cmake for Release build")
ELSE()
	message(STATUS "Unknown build type")
ENDIF()

if (UNIX)
  include(CheckFunctionExists)
endif (UNIX)

get_property(LIB64 GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS)
if (${LIB64} STREQUAL "TRUE")
	set(LIB_SUFFIX 64)
	message(STATUS "64bit system detected")
else()
	set(LIB_SUFFIX "")
endif()

##############################################################################
## feature testing															 #
##############################################################################
#try_compile(HAS_STD_VARIANT ${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/test/try_compile_variant.cpp)

##############################################################################
## Finding the libraries and packages										 #
##############################################################################
# pkgconfig
find_package(PkgConfig REQUIRED)

# crypt
find_package(Crypt REQUIRED)

if (CRYPT_FOUND)
	list(APPEND INCLUDE_DIRS ${CRYPT_INCLUDE_DIR})
	list(APPEND LIBRARIES ${CRYPT_LIBRARIES})
else (NOT CRYPT_FOUND)
	message(FATAL_ERROR "crypt library not found!")
endif ()

# file-magic
find_package(LibMagic REQUIRED)

if (LIBMAGIC_FOUND)
	list(APPEND INCLUDE_DIRS ${LIBMAGIC_INCLUDE_DIR})
	list(APPEND LIBRARIES ${LIBMAGIC_LIBRARY})
else (NOT LIBMAGIC_FOUND)
	message(FATAL_ERROR "libmagic library not found!")
endif ()

# OpenSSL
find_package (OpenSSL REQUIRED)

if (OPENSSL_FOUND)
	list(APPEND INCLUDE_DIRS ${OPENSSL_INCLUDE_DIRS})
	list(APPEND LIBRARIES ${OPENSSL_LIBRARIES})
else (NOT OPENSSL_FOUND)
	message(FATAL_ERROR "OpenSSL Library missing!")
endif ()

# GSSAPI
find_package (GSSAPI REQUIRED)

if (GSSAPI_FOUND)
	list(APPEND INCLUDE_DIRS ${GSSAPI_INCLUDE_DIR})
	list(APPEND LIBRARIES ${GSSAPI_LIBRARIES})
else (NOT GSSAPI_FOUND)
	message(FATAL_ERROR "GSSAPI Library missing!")
endif ()

# CURL
find_package (CURL REQUIRED)

if (CURL_FOUND)
	list(APPEND INCLUDE_DIRS ${CURL_INCLUDE_DIRS})
	list(APPEND LIBRARIES ${CURL_LIBRARIES})
	MESSAGE(STATUS "Found CURL at ${CURL_LIBRARIES}")
else (NOT CURL_FOUND)
	message(FATAL_ERROR "CURL library missing!")
endif ()

# BZip2
find_package (BZip2 REQUIRED)

if (BZIP2_FOUND)
	list(APPEND INCLUDE_DIRS ${BZIP2_INCLUDE_DIRS})
	list(APPEND LIBRARIES ${BZIP2_LIBRARIES})
	MESSAGE(STATUS "Found BZip2 at ${BZIP2_LIBRARIES}")
else (NOT BZIP2_FOUND)
	message(FATAL_ERROR "BZip2 library missing!")
endif ()

# Zstd
find_package (ZSTD REQUIRED)

if (ZSTD_FOUND)
	list(APPEND INCLUDE_DIRS ${ZSTD_INCLUDE_DIRS})
	list(APPEND LIBRARIES ${ZSTD_LIBRARIES})
	MESSAGE(STATUS "Found ZStandard at ${ZSTD_LIBRARIES}")
else (NOT ZSTD_FOUND)
	message(FATAL_ERROR "ZStandard Library missing!")
endif ()

# Boost (regex, iostreams, system)
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

find_package(Boost REQUIRED COMPONENTS regex iostreams system thread filesystem) 

if (Boost_FOUND)
	list(APPEND INCLUDE_DIRS ${Boost_INCLUDE_DIRS})
	list(APPEND LIBRARIES ${Boost_LIBRARIES})
else (NOT Boost_FOUND)
	message(FATAL_ERROR "Boost RegEx library not found!")
endif ()

# Oracle
find_package(ORACLE REQUIRED)
if (ORACLE_FOUND)
	list(APPEND INCLUDE_DIRS ${ORACLE_INCLUDE_DIR})
	list(APPEND LIBRARIES ${ORACLE_LIBRARY})
else (NOT ORACLE_FOUND)
		message(FATAL_ERROR "Oracle library not found!")
endif ()

# SQLite3
find_package(SQLite3 REQUIRED)

if (SQLite3_FOUND)
	list(APPEND INCLUDE_DIRS ${SQLite3_INCLUDE_DIRS})
	list(APPEND LIBRARIES ${SQLite3_LIBRARIES})
else (NOT SQLite3_FOUND)
	message(FATAL_ERROR "SQLite3 library not found!")
endif ()

# SSH2
find_package(LIBSSH2 REQUIRED)
if (LIBSSH2_FOUND)
	list(APPEND INCLUDE_DIRS ${LIBSSH2_INCLUDE_DIR})
	list(APPEND LIBRARIES ${LIBSSH2_LIBRARY})
else (NOT LIBSSH2_FOUND)
	message(FATAL_ERROR "SSH2 library not found!")
endif ()

# Config++
find_package(Config++ REQUIRED)

if (CONFIG++_FOUND)
	list(APPEND INCLUDE_DIRS ${CONFIG++_INCLUDE_DIR})
	list(APPEND LIBRARIES ${CONFIG++_LIBRARY})
else (NOT CONFIG++_FOUND)
	message(FATAL_ERROR "Config++ library not found!")
endif ()

# BLKID
find_package(blkid REQUIRED)

if (BLKID_FOUND)
	list(APPEND INCLUDE_DIRS ${BLKID_INCLUDE_DIR})
	list(APPEND LIBRARIES ${BLKID_LIBRARIES})
else (NOT BLKID_FOUND)
	message(FATAL_ERROR "BLKID library not found!")
endif ()

# Samba share
find_package(SMB2 REQUIRED)

if (SMB2_FOUND)
	list(APPEND INCLUDE_DIRS ${SMB2_INCLUDE_DIRS})
	list(APPEND LIBRARIES ${SMB2_LIBRARIES})
else (NOT SMB2_FOUND)
	message(FATAL_ERROR "AWS CPP library not found!")
endif ()

# rdkafka
find_package(RdKafka REQUIRED)

if (RDKAFKA_FOUND)
	list(APPEND INCLUDE_DIRS ${RDKAFKA_INCLUDE_DIR})
	list(APPEND LIBRARIES ${RDKAFKA_LIBRARIES})
	MESSAGE(STATUS "Found RDKAFKA at ${RDKAFKA_LIBRARIES}")
else (NOT RDKAFKA_FOUND)
	message(FATAL_ERROR "RDKAFKA library not found!")
endif ()

# avro
find_package(AVRO REQUIRED)

if (AVRO_FOUND)
	list(APPEND INCLUDE_DIRS ${AVRO_INCLUDE_DIR})
	list(APPEND LIBRARIES ${AVRO_LIBRARIES})
	MESSAGE(STATUS "Found AVRO at ${AVRO_LIBRARIES}")
else (NOT AVRO_FOUND)
	message(FATAL_ERROR "AVRO library not found!")
endif ()

# SERDES
find_package(SerDes REQUIRED)

if (SERDES_FOUND)
	list(APPEND INCLUDE_DIRS ${SERDES_INCLUDE_DIR})
	list(APPEND LIBRARIES ${SERDES_LIBRARIES})
	message(STATUS "Found SERDES at ${SERDES_LIBRARIES}")
else (NOT SERDES_FOUND)
	message(FATAL_ERROR "SERDES library not found!")
endif ()

# Kerberos 5
find_package(KRB5 REQUIRED gssapi)
if (KRB5_FOUND)
    list(APPEND INCLUDE_DIRS ${KRB5_INCLUDE_DIRS})
    list(APPEND LIBRARIES ${KRB5_LIBRARIES})
else (NOT KRB5_FOUND)
	message(FATAL_ERROR "Kerberos 5 library not found!")
endif()

# Java Native Interface
#find_package(JNI REQUIRED)
#if (JNI_FOUND)
#    list(APPEND INCLUDE_DIRS ${JAVA_INCLUDE_PATH})
#    list(APPEND LIBRARIES ${JAVA_JVM_LIBRARY})
#endif()

# HDFS
find_package(HDFS3 REQUIRED)
if (HDFS3_FOUND)
    list(APPEND INCLUDE_DIRS ${HDFS3_INCLUDE_DIR})
    list(APPEND LIBRARIES ${HDFS3_LIBRARY})
else (NOT HDFS3_FOUND)
	message(FATAL_ERROR "LibHDFS Not Found!")
endif()

# AWS CPP SDK
find_package(AWSSDK REQUIRED COMPONENTS s3)

if (AWSSDK_FOUND)
	list(APPEND INCLUDE_DIRS ${AWSSDK_INCLUDE_DIRS})
	list(APPEND LIBRARIES ${AWSSDK_LINK_LIBRARIES})
else (NOT AWSSDK_FOUND)
	message(FATAL_ERROR "AWS CPP library not found!")
endif ()

##############################################################################
##############################################################################
set(CPACK_PACKAGE_VERSION ${VERSION})
set(CPACK_GENERATOR "RPM")
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_RELEASE 1)
set(CPACK_PACKAGE_CONTACT "Amirul Islam")
set(CPACK_PACKAGE_VENDOR "binutil.net")
set(CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}")
#set(CPACK_RPM_USER_BINARY_SPECFILE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.spec")
include(CPack)
###

# add_library(${PROJECT_NAME} SHARED "")
# # set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER include)

# include_directories(include)
# add_subdirectory(src)
# add_subdirectory(test)

# include_directories(${INCLUDE_DIRS})
# link_libraries(${LIBRARIES})
# target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBRARIES})

# configure_file(extras/${PROJECT_NAME}.pc.in ${PROJECT_NAME}.pc @ONLY)
# configure_file(extras/${PROJECT_NAME}.spec.in ${PROJECT_NAME}.spec @ONLY IMMEDIATE)

# install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
# install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/)
# install(FILES ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pc DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)

add_library(${PROJECT_NAME} SHARED "")
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR})

add_subdirectory(src)
add_subdirectory(test)

target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBRARIES})
target_link_options(${PROJECT_NAME} PRIVATE -static-libgcc -static-libstdc++)

configure_file(extras/${PROJECT_NAME}.pc.in ${PROJECT_NAME}.pc @ONLY)
configure_file(extras/${PROJECT_NAME}.spec.in ${PROJECT_NAME}.spec @ONLY IMMEDIATE)

# install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})
install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/)
install(FILES ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.pc DESTINATION ${CMAKE_INSTALL_DATADIR}/pkgconfig)

# make uninstall
add_custom_target("uninstall" COMMENT "Uninstall installed files")
add_custom_command(
    TARGET "uninstall"
    POST_BUILD
    COMMENT "Uninstall files with install_manifest.txt"
    COMMAND xargs rm -vf < install_manifest.txt || echo Nothing in install_manifest.txt to be uninstalled!
)
