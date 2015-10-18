project(example_rpi2)
cmake_minimum_required(VERSION 2.8)

#SET(CMAKE_FIND_ROOT_PATH ~/develop/cross_compile/tools/arm_bcm2708/)
#SET(CMAKE_C_COMPILER arm-linux-gnueabihf-gcc)
#SET(CMAKE_CXX_COMPILER arm-linux-gnueabihf-gpp)

find_package(Qt5Core)
find_package(Qt5Network)

if(BUILD_ARM)
	set(CMAKE_C_COMPILER arm-linux-gnueabihf-gcc)
	set(CMAKE_CXX_COMPILER arm-linux-gnueabihf-c++)
else()
	set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

include_directories(. ./submodules/struct_controls)

aux_source_directory(. SRC_LIST)
aux_source_directory(./submodules/struct_controls SRC_LIST)
add_executable(${PROJECT_NAME} ${SRC_LIST})
target_link_libraries(${PROJECT_NAME}  Qt5::Core Qt5::Network)

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
		message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()
