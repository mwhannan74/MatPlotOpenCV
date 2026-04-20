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

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/local_paths.cmake")

option(MATPLOTOPENCV_BUILD_DEMO "Build the demo executable." ON)
option(MATPLOTOPENCV_BUILD_DOCS "Build API documentation with Doxygen." ON)

if(MATPLOTOPENCV_OPENCV_DIR)
    set(OpenCV_DIR "${MATPLOTOPENCV_OPENCV_DIR}")
endif()

find_package(OpenCV 4.5 REQUIRED COMPONENTS core highgui imgproc)

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

if(MATPLOTOPENCV_BUILD_DOCS)
    find_package(Doxygen REQUIRED)

    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in"
        "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile"
        @ONLY
    )

    add_custom_target(doc
        COMMAND "${DOXYGEN_EXECUTABLE}" "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )
endif()
