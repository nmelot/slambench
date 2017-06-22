find_path(TOON_DEBUG_INCLUDE_PATH TooN/TooN.h
	~/usr/debug/include
	~/usr/.local/debug/include
	~/.local/debug/include
	~/usr/local/debug/include
	/usr/debug/include
	/usr/local/debug/include
	thirdparty
)

find_path(TOON_INCLUDE_PATH TooN/TooN.h
	~/usr/include
	~/usr/.local/include
	~/.local/include
	~/usr/local/include
	/usr/include
	/usr/local/include
	thirdparty
)

string(TOUPPER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE_UPPER)
if(TOON_INCLUDE_PATH AND NOT CMAKE_BUILD_TYPE_UPPER MATCHES DEBUG)
	set(TooN_FOUND TRUE)
    	   message("-- Found Toon: ${TOON_INCLUDE_PATH}")
	set(TOON_INCLUDE_PATHS ${TOON_INCLUDE_PATH} CACHE STRING "The include paths needed to use TooN")
endif()

if(TOON_DEBUG_INCLUDE_PATH AND CMAKE_BUILD_TYPE_UPPER MATCHES DEBUG)
	set(TooN_FOUND TRUE)
    	   message("-- Found Toon: ${TOON_DEBUG_INCLUDE_PATH}")
	set(TOON_INCLUDE_PATHS ${TOON_DEBUG_INCLUDE_PATH} CACHE STRING "The include paths needed to use TooN")
endif()

mark_as_advanced(
	TOON_INCLUDE_PATHS
)


# Generate appropriate messages
if(TooN_FOUND)
    if(NOT TooN_FIND_QUIETLY)
    endif(NOT TooN_FIND_QUIETLY)
else(TooN_FOUND)
    if(TooN_FIND_REQUIRED)
	message(FATAL_ERROR "-- Could NOT find TooN (missing: TOON_INCLUDE_PATH)")
    endif(TooN_FIND_REQUIRED)
endif(TooN_FOUND)
