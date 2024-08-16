cmake_minimum_required(VERSION 3.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

#set(CMAKE_BUILD_TYPE Debug)

project(SPRF)

add_subdirectory(raylib)

file(GLOB_RECURSE sources src/*.cpp src/*.c src/*.h src/*.hpp)

add_library(SPRF ${sources})

target_include_directories(SPRF PUBLIC src raylib-cpp/include raylib/src)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    SET(CMAKE_EXE_LINKER_FLAGS  "-framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL")
    SET( CMAKE_CXX_FLAGS  "-Wall -Wpedantic -g -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer -fno-inline -fPIC")
else()
    SET( CMAKE_CXX_FLAGS  "-fPIC")
endif()

file( GLOB DRIVER_SOURCES drivers/*.cpp )
foreach( sourcefile ${DRIVER_SOURCES} )
    get_filename_component( name ${sourcefile} NAME_WE )
    add_executable( ${name} ${sourcefile} )
    target_link_directories( ${name} PUBLIC raylib/src)
    if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        target_link_libraries( ${name} SPRF raylib )
    else()
        target_link_libraries( ${name} SPRF raylib opengl32 gdi32 ws2_32 winmm)
    endif()


endforeach( sourcefile ${DRIVER_SOURCES} )