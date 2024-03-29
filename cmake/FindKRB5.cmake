# Copied from
# https://raw.githubusercontent.com/nfs-ganesha/nfs-ganesha/master/src/cmake/modules/FindKrb5.cmake
# Removed gssapi if not requested in find package invocation.
# Added version detection by Majlen
# - Find kerberos 5
# Find the native Kerberos 5 headers and libraries.
#    KRB5_INCLUDE_DIRS            - where to find krb5.h, etc.
#    KRB5_LIBRARIES                 - List of libraries when using kerberos 5.
#    KRB5_FOUND                         - True if kerberos 5 found.
#    KRB5_VERSION                     - Kerberos5 libraries version
# KRB5 modules may be specified as components for this find module.
# Modules may be listed by running "krb5-config".    Modules include:
#    krb5                            Kerberos 5 application
#    gssapi                        GSSAPI application with Kerberos 5 bindings
#    krb4                            Kerberos 4 application
#    kadm-client             Kadmin client
#    kadm-server             Kadmin server
#    kdb                             Application that accesses the kerberos database
# Typical usage:
#    FIND_PACKAGE(KRB5 REQUIRED gssapi)

# First find the config script from which to obtain other values.
IF(KRB5_PREFIX)
    FIND_PROGRAM(KRB5_C_CONFIG NAMES krb5-config
        PATHS ${KRB5_PREFIX}
        NO_SYSTEM_ENVIRONMENT_PATH
        NO_DEFAULT_PATH
        )
ENDIF(KRB5_PREFIX)
FIND_PROGRAM(KRB5_C_CONFIG NAMES krb5-config PATHS /usr/lib/mit/bin)

# Check whether we found anything.
IF(KRB5_C_CONFIG)
    SET(KRB5_FOUND 1)
ELSE(KRB5_C_CONFIG)
    SET(KRB5_FOUND 0)
ENDIF(KRB5_C_CONFIG)

# Lookup the include directories needed for the components requested.
IF(KRB5_FOUND)
    # Use the newer EXECUTE_PROCESS command if it is available.
    IF(COMMAND EXECUTE_PROCESS)
        EXECUTE_PROCESS(
            COMMAND ${KRB5_C_CONFIG} ${KRB5_FIND_COMPONENTS} --cflags
            OUTPUT_VARIABLE KRB5_C_CONFIG_CFLAGS
            OUTPUT_STRIP_TRAILING_WHITESPACE
            RESULT_VARIABLE KRB5_C_CONFIG_RESULT
            )
    ELSE(COMMAND EXECUTE_PROCESS)
        EXEC_PROGRAM(${KRB5_C_CONFIG} ARGS "${KRB5_FIND_COMPONENTS} --cflags"
            OUTPUT_VARIABLE KRB5_C_CONFIG_CFLAGS
            RETURN_VALUE KRB5_C_CONFIG_RESULT
            )
    ENDIF(COMMAND EXECUTE_PROCESS)

    # Parse the include flags.
    IF("${KRB5_C_CONFIG_RESULT}" MATCHES "^0$")
        # Convert the compile flags to a CMake list.
        STRING(REGEX REPLACE " +" ";"
            KRB5_C_CONFIG_CFLAGS "${KRB5_C_CONFIG_CFLAGS}")

        # Look for -I options.
        SET(KRB5_INCLUDE_DIRS)
        FOREACH(flag ${KRB5_C_CONFIG_CFLAGS})
            IF("${flag}" MATCHES "^-I")
                STRING(REGEX REPLACE "^-I" "" DIR "${flag}")
                FILE(TO_CMAKE_PATH "${DIR}" DIR)
                SET(KRB5_INCLUDE_DIRS ${KRB5_INCLUDE_DIRS} "${DIR}")
            ENDIF("${flag}" MATCHES "^-I")
        ENDFOREACH(flag)
    ELSE("${KRB5_C_CONFIG_RESULT}" MATCHES "^0$")
        MESSAGE("Error running ${KRB5_C_CONFIG}: [${KRB5_C_CONFIG_RESULT}]")
        SET(KRB5_FOUND 0)
    ENDIF("${KRB5_C_CONFIG_RESULT}" MATCHES "^0$")
ENDIF(KRB5_FOUND)

IF (KRB5_PREFIX AND EXISTS "${KRB5_PREFIX}/include")
    SET(KRB5_INCLUDE_DIRS "${KRB5_PREFIX}/include" ${KRB5_INCLUDE_DIRS})
ENDIF (KRB5_PREFIX AND EXISTS "${KRB5_PREFIX}/include")

# Lookup the libraries needed for the components requested.
IF(KRB5_FOUND)
    # Use the newer EXECUTE_PROCESS command if it is available.
    IF(COMMAND EXECUTE_PROCESS)
        EXECUTE_PROCESS(
            COMMAND ${KRB5_C_CONFIG} ${KRB5_FIND_COMPONENTS} --libs
            OUTPUT_VARIABLE KRB5_C_CONFIG_LIBS
            OUTPUT_STRIP_TRAILING_WHITESPACE
            RESULT_VARIABLE KRB5_C_CONFIG_RESULT
            )
    ELSE(COMMAND EXECUTE_PROCESS)
        EXEC_PROGRAM(${KRB5_C_CONFIG} ARGS "${KRB5_FIND_COMPONENTS} --libs"
            OUTPUT_VARIABLE KRB5_C_CONFIG_LIBS
            RETURN_VALUE KRB5_C_CONFIG_RESULT
            )
    ENDIF(COMMAND EXECUTE_PROCESS)

    # Parse the library names and directories.
    IF("${KRB5_C_CONFIG_RESULT}" MATCHES "^0$")
        STRING(REGEX REPLACE " +" ";"
            KRB5_C_CONFIG_LIBS "${KRB5_C_CONFIG_LIBS}")

        # Look for -L flags for directories and -l flags for library names.
        SET(KRB5_LIBRARY_DIRS)
        SET(KRB5_LIBRARY_NAMES)
        FOREACH(flag ${KRB5_C_CONFIG_LIBS})
            IF("${flag}" MATCHES "^-L")
                STRING(REGEX REPLACE "^-L" "" DIR "${flag}")
                FILE(TO_CMAKE_PATH "${DIR}" DIR)
                SET(KRB5_LIBRARY_DIRS ${KRB5_LIBRARY_DIRS} "${DIR}")
            ELSEIF("${flag}" MATCHES "^-l")
                STRING(REGEX REPLACE "^-l" "" NAME "${flag}")
                SET(KRB5_LIBRARY_NAMES ${KRB5_LIBRARY_NAMES} "${NAME}")
            ENDIF("${flag}" MATCHES "^-L")
        ENDFOREACH(flag)

        # add gssapi_krb5 (MIT)
        # SET(KRB5_LIBRARY_NAMES ${KRB5_LIBRARY_NAMES} "gssapi_krb5")
        SET(KRB5_LIBRARY_NAMES ${KRB5_LIBRARY_NAMES} "krb5support")

        # Search for each library needed using the directories given.
        FOREACH(name ${KRB5_LIBRARY_NAMES})
            # Look for this library.
            FIND_LIBRARY(KRB5_${name}_LIBRARY
                NAMES ${name}
                PATHS ${KRB5_LIBRARY_DIRS}
                NO_DEFAULT_PATH
                )
            FIND_LIBRARY(KRB5_${name}_LIBRARY NAMES ${name})
            MARK_AS_ADVANCED(KRB5_${name}_LIBRARY)

            # If any library is not found then the whole package is not found.
            IF(NOT KRB5_${name}_LIBRARY)
                SET(KRB5_FOUND 0)
            ENDIF(NOT KRB5_${name}_LIBRARY)

            # Build an ordered list of all the libraries needed.
            SET(KRB5_LIBRARIES ${KRB5_LIBRARIES} "${KRB5_${name}_LIBRARY}")
        ENDFOREACH(name)
    ELSE("${KRB5_C_CONFIG_RESULT}" MATCHES "^0$")
        MESSAGE("Error running ${KRB5_C_CONFIG}: [${KRB5_C_CONFIG_RESULT}]")
        SET(KRB5_FOUND 0)
    ENDIF("${KRB5_C_CONFIG_RESULT}" MATCHES "^0$")
ENDIF(KRB5_FOUND)

# Lookup the version
IF(KRB5_FOUND)
    # Use the newer EXECUTE_PROCESS command if it is available.
    IF(COMMAND EXECUTE_PROCESS)
        EXECUTE_PROCESS(
            COMMAND ${KRB5_C_CONFIG} --version
            OUTPUT_VARIABLE KRB5_C_CONFIG_VERSION
            OUTPUT_STRIP_TRAILING_WHITESPACE
            RESULT_VARIABLE KRB5_C_CONFIG_RESULT
            )
    ELSE(COMMAND EXECUTE_PROCESS)
        EXEC_PROGRAM(${KRB5_C_CONFIG} ARGS "--version"
            OUTPUT_VARIABLE KRB5_C_CONFIG_VERSION
            RETURN_VALUE KRB5_C_CONFIG_RESULT
            )
    ENDIF(COMMAND EXECUTE_PROCESS)

    # Parse the include flags.
    IF("${KRB5_C_CONFIG_RESULT}" MATCHES "^0$")
        # Convert the compile flags to a CMake list.
        STRING(REGEX REPLACE "^Kerberos 5 release " ""
        KRB5_VERSION "${KRB5_C_CONFIG_VERSION}")
    ELSE("${KRB5_C_CONFIG_RESULT}" MATCHES "^0$")
        MESSAGE("Error running ${KRB5_C_CONFIG}: [${KRB5_C_CONFIG_RESULT}]")
        SET(KRB5_FOUND 0)
    ENDIF("${KRB5_C_CONFIG_RESULT}" MATCHES "^0$")
ENDIF(KRB5_FOUND)

# Report the results.
IF(NOT KRB5_FOUND)
    SET(KRB5_DIR_MESSAGE
        "KRB5 was not found. Make sure the entries KRB5_* are set.")
    IF(NOT KRB5_FIND_QUIETLY)
        MESSAGE(STATUS "${KRB5_DIR_MESSAGE}")
    ELSE(NOT KRB5_FIND_QUIETLY)
        IF(KRB5_FIND_REQUIRED)
            MESSAGE(FATAL_ERROR "${KRB5_DIR_MESSAGE}")
        ENDIF(KRB5_FIND_REQUIRED)
    ENDIF(NOT KRB5_FIND_QUIETLY)
ELSE(NOT KRB5_FOUND)
    # SET(CMAKE_REQUIRED_INCLUDES ${KRB5_INCLUDE_DIRS})
    # SET(CMAKE_REQUIRED_LIBRARIES ${KRB5_LIBRARIES})
    IF(NOT DEFINED KRB5_USE_KRB5H)
        INCLUDE(CheckIncludeFile)
        CHECK_INCLUDE_FILE("krb5/krb5.h" KRB5_krb5_krb5_h_exists)
        IF(NOT KRB5_krb5_krb5_h_exists)
            CHECK_INCLUDE_FILE("krb5.h" KRB5_USE_KRB5H)
        ELSE(NOT KRB5_krb5_krb5_h_exists)
            SET(KRB5_USE_KRB5H FALSE)
        ENDIF(NOT KRB5_krb5_krb5_h_exists)
    ENDIF(NOT DEFINED KRB5_USE_KRB5H)
    INCLUDE(CheckFunctionExists)
    IF(NOT DEFINED KRB5_FLAVOR)
        CHECK_FUNCTION_EXISTS(krb5_free_keytab_entry_contents
            KRB5_HAS_krb5_free_keytab_entry_contents)
        IF(KRB5_HAS_krb5_free_keytab_entry_contents)
            SET(KRB5_FLAVOR "MIT")
        ELSE(KRB5_HAS_krb5_free_keytab_entry_contents)
            SET(KRB5_FLAVOR "HEIMDAL")
        ENDIF(KRB5_HAS_krb5_free_keytab_entry_contents)
    ENDIF(NOT DEFINED KRB5_FLAVOR)
    IF("${KRB5_FLAVOR}" STREQUAL "MIT")
        IF(NOT DEFINED KRB5_HAS_krb5_get_init_creds_opt_set_out_ccache)
            CHECK_FUNCTION_EXISTS(krb5_get_init_creds_opt_set_out_ccache
                KRB5_HAS_krb5_get_init_creds_opt_set_out_ccache)
        ENDIF(NOT DEFINED KRB5_HAS_krb5_get_init_creds_opt_set_out_ccache)
        IF(NOT DEFINED KRB5_HAS_krb5_unparse_name_flags_ext)
            CHECK_FUNCTION_EXISTS(krb5_unparse_name_flags_ext
                KRB5_HAS_krb5_unparse_name_flags_ext)
        ENDIF(NOT DEFINED KRB5_HAS_krb5_unparse_name_flags_ext)
    ENDIF("${KRB5_FLAVOR}" STREQUAL "MIT")

    MESSAGE(STATUS "Found kerberos 5 headers:        ${KRB5_INCLUDE_DIRS}")
    MESSAGE(STATUS "Found kerberos 5 libs:             ${KRB5_LIBRARIES}")
    MESSAGE(STATUS "Detected kerberos 5 version: ${KRB5_VERSION}")
    MESSAGE(STATUS "Detected kerberos 5 flavor:    ${KRB5_FLAVOR}")
ENDIF(NOT KRB5_FOUND)