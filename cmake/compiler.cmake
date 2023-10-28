################################
## set compiler features here ##
################################

# target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_CXX_FLAGS_RELEASE "-O2 -s")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")

if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Debug)
endif ()

if (CMAKE_BUILD_TYPE MATCHES Debug)
	message(STATUS "Configuring cmake for Debug build")
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG}")
	add_definitions(-DDEBUG)
elseif (CMAKE_BUILD_TYPE MATCHES Release)
	message(STATUS "Configuring cmake for Release build")
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE}")
else ()
	message(STATUS "Unknown build type")
endif ()

# # detect and set compiler capabilities
include(CheckCXXCompilerFlag)

enable_cxx_compiler_flag_if_supported("-D_GLIBCXX_USE_NANOSLEEP")
enable_cxx_compiler_flag_if_supported("-Wall")
enable_cxx_compiler_flag_if_supported("-Wextra")
enable_cxx_compiler_flag_if_supported("-pedantic")

# set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# set(GCC_COVERAGE_COMPILE_FLAGS "-D_GLIBCXX_USE_NANOSLEEP -Wall")

# SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}")
# set(CMAKE_CXX_FLAGS_RELEASE "-O2 -s")
# set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")

# if(NOT CMAKE_BUILD_TYPE)
# 	set(CMAKE_BUILD_TYPE Debug)
# endif()

# IF(CMAKE_BUILD_TYPE MATCHES Debug)
# 	message(STATUS "Configuring cmake for Debug build")
# 	SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG}")
# 	add_definitions(-DDEBUG)
# ELSEIF(CMAKE_BUILD_TYPE MATCHES Release)
# 	message(STATUS "Configuring cmake for Release build")
# 	SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE}")
# ELSE()
# 	message(STATUS "Unknown build type")
# ENDIF()
