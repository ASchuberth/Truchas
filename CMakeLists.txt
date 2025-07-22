cmake_minimum_required(VERSION 3.15)


#=========================================
# Project Title
#=========================================

project(truchas VERSION 0.1.0)

#=========================================
# C++/Compiler Options
#=========================================

# Must be called in top-level CMakeLists.txt
enable_testing()


#=========================================
# C++/Compiler Options
#=========================================

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra") 


#=========================================
# CMake Custom Modules Folder
#=========================================

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})


#=========================================
# Caldera Module Truchas Library
#=========================================

add_library(truchas  src/truchas.cpp
                     src/model.cpp
                     src/observer.cpp
                     src/sketch.cpp
                     src/subject.cpp
)


#=========================================
# Precompiled Headers
#=========================================

add_library(pch_interface INTERFACE)
target_precompile_headers(pch_interface INTERFACE src/pch.hpp) 
target_include_directories(pch_interface INTERFACE ${CMAKE_SOURCE_DIR}/src)

#=========================================
# CMake Add Directories
#=========================================

add_subdirectory(docs/)
add_subdirectory(test/)

#=========================================
# Libraries
#=========================================

## Find Libraries

find_package(glm CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(Stb REQUIRED)

## Link Libraries

### External 

target_link_libraries(pch_interface INTERFACE glfw)
target_link_libraries(pch_interface INTERFACE glm::glm)
target_link_libraries(pch_interface INTERFACE imgui::imgui)
target_include_directories(pch_interface INTERFACE ${Stb_INCLUDE_DIR})
target_link_libraries(pch_interface INTERFACE Vulkan::Vulkan)

### Precompiled Headers

target_link_libraries(truchas PUBLIC pch_interface)

#=========================================
# Shaders
#=========================================

get_filename_component(VERT_SHADER_FILE_PATH ${CMAKE_CURRENT_LIST_DIR}/shaders/vert.spv ABSOLUTE)
get_filename_component(FRAG_SHADER_FILE_PATH ${CMAKE_CURRENT_LIST_DIR}/shaders/frag.spv ABSOLUTE)
configure_file(${CMAKE_CURRENT_LIST_DIR}/src/config.h.in ${CMAKE_CURRENT_LIST_DIR}/src/config.h)


# Compile Shaders

if(WIN32)
    execute_process(COMMAND ${CMAKE_CURRENT_LIST_DIR}/shaders/compile_shaders.bat ${CMAKE_CURRENT_LIST_DIR}/shaders/)
endif(WIN32)

if(UNIX)
    execute_process(COMMAND bash ${CMAKE_CURRENT_LIST_DIR}/shaders/compile_shaders.sh ${CMAKE_CURRENT_LIST_DIR}/shaders/)
endif(UNIX)