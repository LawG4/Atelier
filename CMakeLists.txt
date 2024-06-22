cmake_minimum_required(VERSION 3.16.0 FATAL_ERROR)
project(Atelier VERSION 0.1.0 LANGUAGES CXX C)
if(NOT TARGET atelier_interface)
	add_library(atelier_interface INTERFACE)
else()
	return()
endif()

add_executable(atelier WIN32
	include/atelier/atelier.h
	source/_application_wwinmain.cpp)

target_include_directories(atelier PRIVATE 
	${CMAKE_CURRENT_LIST_DIR}/include)