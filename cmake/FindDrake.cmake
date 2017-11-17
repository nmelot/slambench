find_path(DRAKE_INCLUDE_PATH drake.h
	~/.local/include
)
find_path(DRAKE_DEBUG_INCLUDE_PATH drake.h
	~/.local/debug/include
)
find_path(DRAKE_DEBUG_LIB_PATH libdrake.so
	~/.local/debug/lib
)	
find_path(DRAKE_LIB_PATH libdrake.so
	~/.local/lib
)

string(TOUPPER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE_UPPER)
if(DRAKE_DEBUG_INCLUDE_PATH AND DRAKE_DEBUG_LIB_PATH AND CMAKE_BUILD_TYPE_UPPER MATCHES DEBUG)
	set(DRAKE_FOUND TRUE)
	set(DRAKE_INCLUDE_PATHS ${DRAKE_DEBUG_INCLUDE_PATH} CACHE STRING "The include paths needed to use Drake")
	set(DRAKE_LIBRARIES -L${DRAKE_DEBUG_LIB_PATH} -ldrake_kfusion -ldrake-intel-ia -ldrake -lpelib-core -ldrake-scheduler -lrapl CACHE STRING "Library path for Drake")
	message("-- Found Drake: ${DRAKE_DEBUG_INCLUDE_PATH}, ${DRAKE_DEBUG_LIB_PATH}")
elseif(DRAKE_INCLUDE_PATH AND DRAKE_LIB_PATH AND NOT CMAKE_BUILD_TYPE_UPPER MATCHES DEBUG)
	set(DRAKE_FOUND TRUE)
	set(DRAKE_INCLUDE_PATHS ${DRAKE_INCLUDE_PATH} CACHE STRING "The include paths needed to use Drake")
	set(DRAKE_LIBRARIES -L${DRAKE_LIB_PATH} -ldrake_kfusion -ldrake-intel-ia -ldrake -lpelib-core -ldrake-scheduler -lrapl CACHE STRING "Library path for Drake")
	message("-- Found Drake: ${DRAKE_INCLUDE_PATH}, ${DRAKE_LIB_PATH}")
endif()

mark_as_advanced(
	DRAKE_INCLUDE_PATHS
)


# Generate appropriate messages
if(DRAKE_FOUND)
    if(NOT DRAKE_FIND_QUIETLY)
    endif(NOT DRAKE_FIND_QUIETLY)
else(DRAKE_FOUND)
    if(DRAKE_FIND_REQUIRED)
	message(FATAL_ERROR "-- Could NOT find DRAKE (missing: DRAKE_INCLUDE_PATH)")
    endif(DRAKE_FIND_REQUIRED)
endif(DRAKE_FOUND)
