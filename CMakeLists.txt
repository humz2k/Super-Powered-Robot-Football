cmake_minimum_required(VERSION 3.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project(SPRF)

find_package(OpenGL)

add_custom_command(OUTPUT raylib/src/libraylib.a
                   COMMAND make MACOSX_DEPLOYMENT_TARGET=10.9
                   WORKING_DIRECTORY raylib/src
)

file(GLOB_RECURSE sources src/*.cpp src/*.c src/*.h src/*.hpp)

add_library(SPRF ${sources})

target_include_directories(SPRF PUBLIC src raylib-cpp/include raylib/src)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    SET(CMAKE_EXE_LINKER_FLAGS  "-framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL")
else()
    SET(CMAKE_EXE_LINKER_FLAGS  "-lopengl32 -lgdi32 -lws2_32 -lwinmm")
endif()

file( GLOB DRIVER_SOURCES drivers/*.cpp )
foreach( sourcefile ${DRIVER_SOURCES} )
    get_filename_component( name ${sourcefile} NAME_WE )
    add_executable( ${name} ${sourcefile} )
    target_link_directories( ${name} PUBLIC raylib/src)
    target_link_libraries( ${name} SPRF raylib )
endforeach( sourcefile ${DRIVER_SOURCES} )