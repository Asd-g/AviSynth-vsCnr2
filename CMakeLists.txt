cmake_minimum_required(VERSION 3.25)

if (NOT CMAKE_GENERATOR MATCHES "Visual Studio")
    if (NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
    endif()
endif()

project(vsCnr2 LANGUAGES CXX)

set(sources ${CMAKE_CURRENT_SOURCE_DIR}/src/vsCnr2.cpp)

if (WIN32)
    set(sources ${sources} ${CMAKE_CURRENT_SOURCE_DIR}/src/vsCnr2.rc)
endif()

add_library(vsCnr2 SHARED ${sources})

if (UNIX)
    target_include_directories(vsCnr2 PRIVATE
        /usr/local/include/avisynth
        /usr/local/include
    )
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "IntelLLVM")
    target_link_libraries(vsCnr2 PRIVATE libmmds)
endif()

if (NOT CMAKE_GENERATOR MATCHES "Visual Studio")
    string(TOLOWER ${CMAKE_BUILD_TYPE} build_type)
    if (build_type STREQUAL Debug)
        target_compile_definitions(vsCnr2 PRIVATE DEBUG_BUILD)
    else (build_type STREQUAL Release)
        target_compile_definitions(vsCnr2 PRIVATE RELEASE_BUILD)
    endif()

    if (NOT MSVC)
        target_compile_options(vsCnr2 PRIVATE $<$<CONFIG:Release>:-s>)
    endif()

    message(STATUS "Build type - ${CMAKE_BUILD_TYPE}")
endif()

target_compile_features(vsCnr2 PRIVATE cxx_std_17)

if (CMAKE_CXX_COMPILER_ID STREQUAL "IntelLLVM")
    target_compile_options(vsCnr2 PRIVATE "/fp:precise")
endif()

if (UNIX)
    find_package (Git)

    if (GIT_FOUND)
        execute_process (COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0
            OUTPUT_VARIABLE ver
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        set_target_properties(vsCnr2 PROPERTIES OUTPUT_NAME "vscnr2.${ver}")
    else ()
        message (STATUS "GIT not found")
    endif ()

    include(GNUInstallDirs)

    INSTALL(TARGETS vsCnr2 LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/avisynth")

    # uninstall target
    if(NOT TARGET uninstall)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
        IMMEDIATE @ONLY)

    add_custom_target(uninstall
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
    endif()
endif()
