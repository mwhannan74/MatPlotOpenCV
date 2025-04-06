cmake_minimum_required(VERSION 3.14)
project(MatPlotOpenCV LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ------------------------------------------------------------------
# Find OpenCV
# ------------------------------------------------------------------
set(OpenCV_DIR "C:/opencv/build/x64/vc16/lib/")
find_package(OpenCV 4.5 REQUIRED COMPONENTS core highgui imgproc)

# ------------------------------------------------------------------
# Library target
# ------------------------------------------------------------------
add_library(mpocv INTERFACE)
target_include_directories(mpocv INTERFACE include)
target_link_libraries(mpocv INTERFACE opencv_core opencv_highgui opencv_imgproc)

# ------------------------------------------------------------------
# Example / test executable
# ------------------------------------------------------------------
add_executable(test_MatPlotOpenCV tests/test_MatPlotOpenCV.cpp)
target_link_libraries(test_MatPlotOpenCV PRIVATE mpocv)


# ------------------------------------------------------------------
# Documentation target
# ------------------------------------------------------------------
find_package(Doxygen REQUIRED)

set(PROJECT_VERSION 0.1)          # or pull from a variable

configure_file(
    docs/Doxyfile.in
    ${CMAKE_BINARY_DIR}/Doxyfile
    @ONLY)

add_custom_target(doc
    COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_BINARY_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Generating API documentation with Doxygen"
    VERBATIM)