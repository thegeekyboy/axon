cmake_minimum_required(VERSION 3.26 FATAL_ERROR)
project(experiment)
# include_directories(${INCLUDE_DIRS} "${CMAKE_BINARY_DIR}/include")

# Get a list of all items in the current directory
file(GLOB children LIST_DIRECTORIES true "*")

foreach(child ${children})
    # Check if the item is a valid directory
    if(IS_DIRECTORY ${child})
        if (EXISTS "${child}/CMakeLists.txt")
			cmake_path(GET child FILENAME LAST_DIR)
			message(STATUS "Adding subdirectory: ${child}")
			add_subdirectory(${child})
			list(APPEND EXPERIMENT_LIST ${LAST_DIR})
		endif()
    endif()
endforeach()

if (EXPERIMENT_LIST)
	add_custom_target(experiment)
	add_dependencies(experiment ${EXPERIMENT_LIST})
endif()