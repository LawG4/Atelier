cmake_minimum_required(VERSION 3.16.0 FATAL_ERROR)
project(Atelier VERSION 0.1.0 LANGUAGES CXX C)
if(NOT TARGET atelier_interface)
	add_library(atelier_interface INTERFACE)
else()
	return()
endif()

# Add executable
add_executable(atelier WIN32
	include/atelier/atelier.h
	source/_application_wwinmain.cpp
	source/logger.cpp
	source/vk_presurface.cpp
	source/win32_window_class.cpp)

target_include_directories(atelier PRIVATE 
	${CMAKE_CURRENT_LIST_DIR}/include)

# Set C++ 17
set_target_properties(atelier PROPERTIES 
	CXX_STANDARD 17)

# Add precompiled headers 
target_precompile_headers(atelier PRIVATE "<vector>" "<optional>")

# Add vulkan to the executable
find_package(Vulkan REQUIRED)
target_link_libraries(atelier PRIVATE ${Vulkan_LIBRARIES})
target_include_directories(atelier PRIVATE ${Vulkan_INCLUDE_DIRS})

