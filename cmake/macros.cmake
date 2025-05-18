##
# some helper macros and functiions
##

##
## find the list of directories

MACRO(SUBDIRLIST result curdir)
  FILE(GLOB children RELATIVE ${curdir} ${curdir}/*)
  SET(dirlist "")
  FOREACH(child ${children})
	IF(IS_DIRECTORY ${curdir}/${child})
	  LIST(APPEND dirlist ${child})
	ENDIF()
  ENDFOREACH()
  SET(${result} ${dirlist})
ENDMACRO()

##
## find if the flag is supported by compiler

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

# remove duplicates in list then generate string
function(removeDuplicateSubstring stringIn stringOut)
	separate_arguments(stringIn)
	list(REMOVE_DUPLICATES stringIn)
	string(REPLACE ";" " " stringIn "${stringIn}")
	set(${stringOut} "${stringIn}" PARENT_SCOPE)
endfunction()
