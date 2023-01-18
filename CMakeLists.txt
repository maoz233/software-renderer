cmake_minimum_required(VERSION 3.24.0)

project(renderer VERSION 0.1)

message(STATUS "CMAKE_PROJECT_NAME = ${CMAKE_PROJECT_NAME}")
message(STATUS "PROJECR_SOURCE_DIR = ${PROJECT_SOURCE_DIR}")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler options
if (MSVC)
    # warning level 4 and all warnings as errors
    add_compile_options(/W4 /WX)
else()   
    # lots of warnings and all warnings as errors
    add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

file(GLOB SRC_FILES ${PROJECT_SOURCE_DIR}/src/*.cc)
add_executable(${PROJECT_NAME} ${SRC_FILES})
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/include)

# SDL2
find_package(SDL2 CONFIG REQUIRED)
if(WIN32)
target_link_libraries(${PROJECT_NAME} PRIVATE SDL2::SDL2 SDL2::SDL2main)
endif()
if (APPLE)
target_link_libraries(${PROJECT_NAME} PRIVATE SDL2::SDL2 SDL2::SDL2main SDL2::SDL2-static)
endif()


# SDL_image
find_package(sdl2-image CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE SDL2::SDL2_image)
