# ------------------------------------------------------------------------------
#  MatPlotOpenCV - Minimal 2D plotting library using OpenCV
#  Copyright (c) 202t Michael Hannan
#
#  Licensed under the BSD 3-Clause License.
#  See the LICENSE file in the project root for details.
# ------------------------------------------------------------------------------


cmake_minimum_required(VERSION 3.14)
project(MatPlotOpenCV LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ------------------------------------------------------------------
# Find OpenCV
# ------------------------------------------------------------------
# Adjust this path if your OpenCV is installed elsewhere
set(OpenCV_DIR "C:/opencv/build/x64/vc16/lib/")
find_package(OpenCV 4.5 REQUIRED COMPONENTS core highgui imgproc)

# ------------------------------------------------------------------
# MatPlotOpenCV Library
# ------------------------------------------------------------------
add_library(mpocv STATIC
    src/figure.cpp           # Implementation source file
)

target_include_directories(mpocv PUBLIC include)   # Header path
target_link_libraries(mpocv PUBLIC
    opencv_core
    opencv_highgui
    opencv_imgproc
)

# ------------------------------------------------------------------
# Example / test executable
# ------------------------------------------------------------------
add_executable(test_MatPlotOpenCV tests/test_MatPlotOpenCV.cpp)
target_link_libraries(test_MatPlotOpenCV PRIVATE mpocv)

# ------------------------------------------------------------------
# Doxygen Documentation
# ------------------------------------------------------------------
find_package(Doxygen REQUIRED)

set(PROJECT_VERSION 0.1)

configure_file(
    docs/Doxyfile.in
    ${CMAKE_BINARY_DIR}/Doxyfile
    @ONLY
)

add_custom_target(doc ALL
    COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_BINARY_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Generating API documentation with Doxygen"
    VERBATIM
)
