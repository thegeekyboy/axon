get_property(LIB64 GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS)

if (${LIB64} STREQUAL "TRUE")
	set(LIB_SUFFIX 64)
	message(STATUS "64bit system detected")
else()
	set(LIB_SUFFIX "")
endif()

# configure_file(${CMAKE_CURRENT_SOURCE_DIR}/extras/axon.h.in ${CMAKE_CURRENT_SOURCE_DIR}/include/axon.h)
# configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/axon/defines.h.in ${CMAKE_CURRENT_SOURCE_DIR}/include/axon/defines.h)

# try_compile(HAS_STD_VARIANT ${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/test/try_compile_variant.cpp)

# get_cmake_property(_variableNames VARIABLES)
# list (SORT _variableNames)
# foreach (_variableName ${_variableNames})
#     message(STATUS "${_variableName}=${${_variableName}}")
# endforeach()
