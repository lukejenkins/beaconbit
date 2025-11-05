# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

if(DEFINED ENV{IDF_PATH})
    include($ENV{IDF_PATH}/tools/cmake/project.cmake)
else()
    message(FATAL_ERROR "IDF_PATH environment variable is not set. Please run this with idf.py or source export.sh")
endif()
# "Trim" the build. Include the minimal set of components, main, and anything it depends on.
idf_build_set_property(MINIMAL_BUILD ON)
project(beaconbit)
