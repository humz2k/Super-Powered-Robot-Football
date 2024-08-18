cmake_minimum_required(VERSION 3.16.3)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

cmake_policy(SET CMP0091 NEW)

#set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "package")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "package")


set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")


project(SPRF)

add_subdirectory(raylib)

file(GLOB_RECURSE sources src/*.cpp src/*.c src/*.h src/*.hpp)

add_custom_target(
    my_custom_target_that_always_runs4 ALL
    DEPENDS ${PROJECT_BINARY_DIR}/${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/nothing4
)

add_library(SPRF ${sources})

target_include_directories(SPRF PUBLIC src raylib-cpp/include raylib/src src/soloud/include)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    SET(CMAKE_EXE_LINKER_FLAGS  "-framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL")
    SET( CMAKE_CXX_FLAGS  "-Wall -Wpedantic -g -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer -fno-inline -fPIC")
    #SET( CMAKE_CXX_FLAGS  "-fPIC")
endif()

add_custom_command(
    OUTPUT ${PROJECT_BINARY_DIR}/${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/nothing4 ${PROJECT_BINARY_DIR}/${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/package
        COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/package
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_SOURCE_DIR}/assets
                ${PROJECT_BINARY_DIR}/${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/package/assets
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_SOURCE_DIR}/server_cfg.ini
                ${PROJECT_BINARY_DIR}/${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/package/server_cfg.ini
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_SOURCE_DIR}/client_cfg.ini
                ${PROJECT_BINARY_DIR}/${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/package/client_cfg.ini
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_SOURCE_DIR}/autoexec.cfg
                ${PROJECT_BINARY_DIR}/${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/package/autoexec.cfg
        )

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
    if (${name} MATCHES "game")
        if (WIN32)
            # /ENTRY:mainCRTStartup keeps the same "main" function instead of requiring "WinMain"
            # set(CMAKE_EXE_LINKER_FLAGS " /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
            target_link_options( ${name} PRIVATE "/SUBSYSTEM:WINDOWS" "/ENTRY:mainCRTStartup")
        else()

        endif()
    endif()
endforeach( sourcefile ${DRIVER_SOURCES} )