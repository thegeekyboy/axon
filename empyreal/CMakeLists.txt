cmake_minimum_required(VERSION 3.16 FATAL_ERROR)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

set(CMAKE_C_COMPILER "gcc")
set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w" ${CMAKE_CXX_FLAGS})
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

execute_process(
    COMMAND bash "-c" "cat /etc/passwd | grep empyreal | cut -d ':' -f6"
    OUTPUT_VARIABLE empyreal_HOME_PATH
)
string(REGEX REPLACE "\n$" "" empyreal_HOME_PATH "${empyreal_HOME_PATH}")
if (empyreal_HOME_PATH)
    message(STATUS "Application home folder: ${empyreal_HOME_PATH}")
endif()

project (empyreal VERSION 0.0.0 LANGUAGES C CXX)

set(default_build_type "Release")
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_executable(empyreal "")
include_directories (include)
add_subdirectory(src)

#file (GLOB SOURCES "src/*.c*")
#add_executable(empyreal ${SOURCES})

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

target_compile_features(empyreal PUBLIC cxx_std_11)

#######################################
## Finding the libraries and packages #
#######################################

find_package(PkgConfig REQUIRED)

## systemd unit file path
pkg_check_modules(SYSTEMD "systemd")
if (SYSTEMD_FOUND AND "${SYSTEMD_SERVICES_INSTALL_DIR}" STREQUAL "")
	execute_process(COMMAND ${PKG_CONFIG_EXECUTABLE} --variable=systemdsystemunitdir systemd OUTPUT_VARIABLE SYSTEMD_SERVICES_INSTALL_DIR)
	string(REGEX REPLACE "[ \t\n]+" "" SYSTEMD_SERVICES_INSTALL_DIR	"${SYSTEMD_SERVICES_INSTALL_DIR}")
elseif (NOT SYSTEMD_FOUND AND SYSTEMD_SERVICES_INSTALL_DIR)
	message (FATAL_ERROR "Variable SYSTEMD_SERVICES_INSTALL_DIR is defined, but we can't find systemd using pkg-config")
endif()
if (SYSTEMD_FOUND)
	set(WITH_SYSTEMD "ON")
	message(STATUS "systemd services install dir: ${SYSTEMD_SERVICES_INSTALL_DIR}")
    configure_file(extras/empyreal.service.cmake ${CMAKE_CURRENT_BINARY_DIR}/extras/empyreal.service)
    install (
        FILES ${CMAKE_CURRENT_BINARY_DIR}/extras/empyreal.service 
        DESTINATION ${SYSTEMD_SERVICES_INSTALL_DIR}
    )
else()
	set(WITH_SYSTEMD "OFF")
endif (SYSTEMD_FOUND)

# Crypt
find_package(Crypt REQUIRED)
if (CRYPT_FOUND)
    list(APPEND INCLUDE_DIRS ${CRYPT_INCLUDE_DIR})
    list(APPEND LIBRARIES ${CRYPT_LIBRARIES})
else (NOT CRYPT_FOUND)
    message(FATAL_ERROR "crypt library not found!")
endif ()

# SSH2
option(CMAKE_USE_LIBSSH2 "Use libSSH2" ON)
mark_as_advanced(CMAKE_USE_LIBSSH2)
set(USE_LIBSSH2 OFF)
set(HAVE_LIBSSH2 OFF)
set(HAVE_LIBSSH2_H OFF)

# OpenSSL
find_package (OpenSSL REQUIRED)
if (OPENSSL_FOUND)
    list(APPEND INCLUDE_DIRS ${OPENSSL_INCLUDE_DIRS})
    list(APPEND LIBRARIES ${OPENSSL_LIBRARIES})
else (NOT OPENSSL_FOUND)
    message(FATAL_ERROR "OpenSSL Library missing!")
endif ()

# CURL
find_package (CURL REQUIRED)
if (CURL_FOUND)
    list(APPEND INCLUDE_DIRS ${CURL_INCLUDE_DIRS})
    list(APPEND LIBRARIES ${CURL_LIBRARIES})
else (NOT CURL_FOUND)
    message(FATAL_ERROR "CURL library missing!")
endif ()

# BZip2
find_package (BZip2 REQUIRED)
if (BZIP2_FOUND)
    list(APPEND INCLUDE_DIRS ${BZIP2_INCLUDE_DIRS})
    list(APPEND LIBRARIES ${BZIP2_LIBRARIES})
else (NOT BZIP2_FOUND)
    message(FATAL_ERROR "BZIP2 Library missing!")
endif ()

# Boost (regex, iostreams, system)
set(Boost_USE_STATIC_LIBS OFF) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF) 
find_package(Boost REQUIRED COMPONENTS regex iostreams system) 
if (Boost_FOUND)
    list(APPEND INCLUDE_DIRS ${Boost_INCLUDE_DIRS})
    list(APPEND LIBRARIES ${Boost_LIBRARIES})
else (NOT Boost_FOUND)
    message(FATAL_ERROR "Boost RegEx library not found!")
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
if(CMAKE_USE_LIBSSH2)
    find_package(LibSSH2)
    if(LIBSSH2_FOUND)
        list(APPEND CURL_LIBS ${LIBSSH2_LIBRARY})
        set(CMAKE_REQUIRED_LIBRARIES ${LIBSSH2_LIBRARY})
        list(APPEND CMAKE_REQUIRED_INCLUDES "${LIBSSH2_INCLUDE_DIR}")
        include_directories("${LIBSSH2_INCLUDE_DIR}")

        list(APPEND INCLUDE_DIRS ${LIBSSH2_INCLUDE_DIR})
        list(APPEND LIBRARIES ${LIBSSH2_LIBRARY})

        set(HAVE_LIBSSH2 ON)
        set(USE_LIBSSH2 ON)

        # find_package has already found the headers
        set(HAVE_LIBSSH2_H ON)
        set(CURL_INCLUDES ${CURL_INCLUDES} "${LIBSSH2_INCLUDE_DIR}/libssh2.h")
        set(CURL_TEST_DEFINES "${CURL_TEST_DEFINES} -DHAVE_LIBSSH2_H")

        # now check for specific libssh2 symbols as they were added in different versions
        set(CMAKE_EXTRA_INCLUDE_FILES "libssh2.h")
        #check_function_exists(libssh2_version           HAVE_LIBSSH2_VERSION)
        #check_function_exists(libssh2_init              HAVE_LIBSSH2_INIT)
        #check_function_exists(libssh2_exit              HAVE_LIBSSH2_EXIT)
        #check_function_exists(libssh2_scp_send64        HAVE_LIBSSH2_SCP_SEND64)
        #check_function_exists(libssh2_session_handshake HAVE_LIBSSH2_SESSION_HANDSHAKE)
        #set(CMAKE_EXTRA_INCLUDE_FILES "")
        #unset(CMAKE_REQUIRED_LIBRARIES)
    else (NOT LIBSSH2_FOUND)
        message(FATAL_ERROR "LIBSSH2 Library missing!")
    endif()
endif()

# Config++
find_package(Config++ REQUIRED)
if (CONFIG++_FOUND)
    list(APPEND INCLUDE_DIRS ${CONFIG++_INCLUDE_DIR})
    list(APPEND LIBRARIES ${CONFIG++_LIBRARY})
else (NOT CONFIG++_FOUND)
    message(FATAL_ERROR "Config++ library not found!")
endif ()

# AWS CPP SDK
find_package(AWSSDK REQUIRED COMPONENTS s3)
if (AWSSDK_FOUND)
    list(APPEND INCLUDE_DIRS ${AWSSDK_INCLUDE_DIRS})
    list(APPEND LIBRARIES ${AWSSDK_LINK_LIBRARIES})
else (NOT AWSSDK_FOUND)
    message(FATAL_ERROR "AWS CPP library not found!")
endif ()

# Samba share
find_package(SMB2 REQUIRED)
if (SMB2_FOUND)
    list(APPEND INCLUDE_DIRS ${SMB2_INCLUDE_DIRS})
    list(APPEND LIBRARIES ${SMB2_LIBRARIES})
else (NOT SMB2_FOUND)
    message(FATAL_ERROR "SMB library not found!")
endif ()

#####

include_directories(${INCLUDE_DIRS})
set_property(TARGET empyreal PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)

link_libraries(${LIBRARIES})
target_link_libraries(empyreal PRIVATE ${LIBRARIES})

install(DIRECTORY DESTINATION /home/empyreal)
install(DIRECTORY DESTINATION ${empyreal_HOME_PATH}/logs)
install(DIRECTORY DESTINATION ${empyreal_HOME_PATH}/config)
install(TARGETS empyreal DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)