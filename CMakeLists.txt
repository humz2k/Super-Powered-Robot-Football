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
    #SET(CMAKE_EXE_LINKER_FLAGS  "-framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL")
    SET( CMAKE_CXX_FLAGS  "-Wall -Wpedantic -g -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer -fno-inline -fPIC")
    #SET( CMAKE_CXX_FLAGS  "-fPIC")
endif()

if (APPLE)
    # Need to force OpenGL 3.3 on OS X
    # See: https://github.com/raysan5/raylib/issues/341
    set(GRAPHICS "GRAPHICS_API_OPENGL_33")
    find_library(OPENGL_LIBRARY OpenGL)
    set(LIBS_PRIVATE ${OPENGL_LIBRARY})
    if (NOT CMAKE_SYSTEM STRLESS "Darwin-18.0.0")
        add_definitions(-DGL_SILENCE_DEPRECATION)
        MESSAGE(AUTHOR_WARNING "OpenGL is deprecated starting with macOS 10.14 (Mojave)!")
    endif ()
elseif (WIN32)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    find_package(OpenGL QUIET)
    set(LIBS_PRIVATE ${OPENGL_LIBRARIES} winmm ws2_32)
elseif (UNIX)
    find_library(pthread NAMES pthread)
    find_package(OpenGL QUIET)
    if ("${OPENGL_LIBRARIES}" STREQUAL "")
        set(OPENGL_LIBRARIES "GL")
    endif ()

    if ("${CMAKE_SYSTEM_NAME}" MATCHES "(Net|Open)BSD")
        find_library(OSS_LIBRARY ossaudio)
    endif ()

    set(LIBS_PRIVATE m pthread ${OPENGL_LIBRARIES} ${OSS_LIBRARY})
else ()
    find_library(pthread NAMES pthread)
    find_package(OpenGL QUIET)
    if ("${OPENGL_LIBRARIES}" STREQUAL "")
        set(OPENGL_LIBRARIES "GL")
    endif ()

    set(LIBS_PRIVATE m atomic pthread ${OPENGL_LIBRARIES} ${OSS_LIBRARY})

    if ("${CMAKE_SYSTEM_NAME}" MATCHES "(Net|Open)BSD")
        find_library(OSS_LIBRARY ossaudio)
        set(LIBS_PRIVATE m pthread ${OPENGL_LIBRARIES} ${OSS_LIBRARY})
    endif ()

    if (NOT "${CMAKE_SYSTEM_NAME}" MATCHES "(Net|Open)BSD" AND USE_AUDIO)
        set(LIBS_PRIVATE ${LIBS_PRIVATE} dl)
    endif ()
endif ()

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
    #if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_link_libraries( ${name} SPRF raylib ${LIBS_PRIVATE})
    #else()
    #    target_link_libraries( ${name} SPRF raylib opengl32 gdi32 ws2_32 winmm)
    #endif()
    if (${name} MATCHES "game")
        if (WIN32)
            # /ENTRY:mainCRTStartup keeps the same "main" function instead of requiring "WinMain"
            # set(CMAKE_EXE_LINKER_FLAGS " /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
            target_link_options( ${name} PRIVATE "/SUBSYSTEM:WINDOWS" "/ENTRY:mainCRTStartup")
        else()

        endif()
    endif()
endforeach( sourcefile ${DRIVER_SOURCES} )