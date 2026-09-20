# ------------------------------------------------------------------------------
#  MatPlotOpenCV - Minimal 2D plotting library using OpenCV
#  Copyright (c) 2025 Michael Hannan
#
#  Licensed under the BSD 3-Clause License.
#  See the LICENSE file in the project root for details.
# ------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.16)
project(MatPlotOpenCV VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(WIN32)
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/local_paths.cmake")
endif()

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(MATPLOTOPENCV_DEMO_DEFAULT ON)
else()
    set(MATPLOTOPENCV_DEMO_DEFAULT OFF)
endif()

option(MATPLOTOPENCV_BUILD_DEMO "Build the demo executable." ${MATPLOTOPENCV_DEMO_DEFAULT})
option(MATPLOTOPENCV_BUILD_DOCS "Build API documentation with Doxygen." OFF)
unset(MATPLOTOPENCV_DEMO_DEFAULT)

if(MATPLOTOPENCV_OPENCV_DIR AND NOT OpenCV_DIR)
    set(OpenCV_DIR "${MATPLOTOPENCV_OPENCV_DIR}")
endif()

find_package(OpenCV 4.5 REQUIRED COMPONENTS core highgui imgproc imgcodecs)

add_library(mpocv STATIC
    src/figure.cpp
)
add_library(MatPlotOpenCV::mpocv ALIAS mpocv)

target_include_directories(mpocv
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(mpocv
    PUBLIC
        opencv_core
        opencv_highgui
        opencv_imgproc
        opencv_imgcodecs
)

if(MATPLOTOPENCV_BUILD_DEMO)
    add_executable(matplotopencv_demo
        demo/main_demo.cpp
    )

    target_link_libraries(matplotopencv_demo
        PRIVATE
            MatPlotOpenCV::mpocv
    )
endif()

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    enable_testing()

    add_executable(matplotopencv_tests
        tests/figure_tests.cpp
    )

    target_link_libraries(matplotopencv_tests
        PRIVATE
            MatPlotOpenCV::mpocv
    )

    add_test(NAME matplotopencv_tests COMMAND matplotopencv_tests)
endif()

if(MATPLOTOPENCV_BUILD_DOCS)
    find_package(Doxygen REQUIRED)

    configure_file(
        "${PROJECT_SOURCE_DIR}/docs/Doxyfile.in"
        "${PROJECT_BINARY_DIR}/Doxyfile"
        @ONLY
    )

    add_custom_target(doc
        COMMAND "${DOXYGEN_EXECUTABLE}" "${PROJECT_BINARY_DIR}/Doxyfile"
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )
endif()
