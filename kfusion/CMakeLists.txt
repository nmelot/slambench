cmake_minimum_required(VERSION 2.8)

set(appname kfusion)

find_package(TooN   REQUIRED)
find_package(OpenCL )
find_package(CUDA   )
find_package(GLUT   )
find_package(OpenGL )
find_package(OpenNI2)
find_package(PAPI)
find_package(Drake REQUIRED)

find_package(PkgConfig)
find_package(Qt5Core)
find_package(Qt5Widgets)
find_package(Qt5Gui)
find_package(Qt5OpenGL)
find_package(Qt5PrintSupport)

include_directories(./include ${TOON_INCLUDE_PATHS} ./thirdparty ${DRAKE_INCLUDE_PATHS})

if (CUDA_FOUND) 
 SET( CUDA_PROPAGATE_HOST_FLAGS OFF )
endif(CUDA_FOUND)

 # ---- PREPARE COMMON DEPENDENCIES  ------------ 
if(APPLE)
    set(common_libraries     stdc++ pthread)
else(APPLE)
    set(common_libraries     rt stdc++ pthread ${DRAKE_LIBRARIES})
endif(APPLE)


set(common_compile_flags "")

set(BUILD_EVERYTHING FALSE)

 # ----------------- CPP VERSION ----------------- 

add_library(${appname}-cpp src/cpp/kernels.cpp)
target_link_libraries(${appname}-cpp ${common_libraries} ${OPENNI2_LIBRARIES})
set(${appname}_cpp_rpath ${OPENNI2_LIB_PATH}:$ENV{LD_RUN_PATH})
add_version(${appname} cpp "" "")

if(BUILD_EVERYTHING)
 # ----------------- OPENMP VERSION ----------------- 

add_library(${appname}-openmp  src/cpp/kernels.cpp)
target_link_libraries(${appname}-openmp ${common_libraries} ${OPENNI2_LIBRARIES})
SET_TARGET_PROPERTIES(${appname}-openmp PROPERTIES COMPILE_FLAGS "-fopenmp")
add_version(${appname} openmp "-fopenmp" "-fopenmp")


 #  ----------------- OCL VERSION ----------------- 
 
if (OPENCL_FOUND) 
   include_directories(${OPENCL_INCLUDE_DIRS})
    add_library(${appname}-opencl  src/opencl/kernels.cpp src/opencl/common_opencl.cpp)
    target_link_libraries(${appname}-opencl ${common_libraries} ${OPENCL_LIBRARIES} ${OPENNI2_LIBRARIES})	
    add_version(${appname} opencl "" "")
endif(OPENCL_FOUND)


 #  ----------------- CUDA VERSION ----------------- 
if (CUDA_FOUND) 


set(kfusion_cuda_srcs
	src/cuda/kernels.cu
	thirdparty/kfusion.h
	)
   	
    cuda_add_library(${appname}-cuda STATIC ${kfusion_cuda_srcs} OPTIONS -O3 -std=c++11 -Xcompiler "-std=c++11" )
	target_link_libraries(${appname}-cuda ${common_libraries} ${OPENNI2_LIBRARIES})
    add_version(${appname} cuda    "" "")

endif(CUDA_FOUND)

#  ----------------- thirdparty ----------------- 
endif()

# ----------- Drake version ------
if (DRAKE_FOUND)
	string(TOUPPER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE_UPPER)
	IF(CMAKE_BUILD_TYPE_UPPER MATCHES DEBUG)
		set(${appname}_drake_rpath ${DRAKE_DEBUG_LIB_PATH}:${OPENNI2_LIB_PATH}:$ENV{LD_RUN_PATH})
	ELSE()
		set(${appname}_drake_rpath ${DRAKE_LIB_PATH}:${OPENNI2_LIB_PATH}:$ENV{LD_RUN_PATH})
	ENDIF()
	include_directories(${DRAKE_INCLUDE_DIRS})
	add_library(${appname}-drake src/drake/wrapper.cpp)
	target_link_libraries(${appname}-drake
		${common_libraries}
		${DRAKE_LIBRARIES}
    	)
	add_version(${appname} drake "" "")
endif(DRAKE_FOUND)

add_subdirectory( thirdparty    )


