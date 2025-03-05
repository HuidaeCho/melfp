cmake_minimum_required(VERSION 3.31)
#set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
project(MELFP)

file(GLOB src *.c *.h)
add_executable(melfp ${src})

find_package(OpenMP REQUIRED)
if(OpenMP_C_FOUND)
	target_link_libraries(melfp PUBLIC OpenMP::OpenMP_C)
endif()

find_library(MATH_LIBRARY m)
if(MATH_LIBRARY)
	target_link_libraries(melfp PUBLIC ${MATH_LIBRARY})
endif()

find_package(GDAL REQUIRED)
if(GDAL_FOUND)
	target_link_libraries(melfp PUBLIC ${GDAL_LIBRARY})
endif()
