# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

# This is a custom command to apply a patch to the iperf component after it's downloaded.
# This is necessary to fix a bug in the upstream component that prevents the iperf server
# from accepting connections correctly.
#
# The patch is applied using the 'git apply' command, which is part of the standard
# Git installation. The patch file is located in the 'patches' directory.
#
# The 'WORKING_DIRECTORY' is set to the project's source directory, and the patch is
# applied with '--directory' pointing to the component's directory. This ensures that
# the patch is applied to the correct files.
#
# The 'COMMENT' provides a message that is displayed during the build process, and the
# 'USES_TERMINAL' option ensures that the command is run in a terminal, which is

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/iperf_patched.stamp
    COMMAND patch -p1 -d ${CMAKE_SOURCE_DIR}/managed_components/espressif__iperf < ${CMAKE_SOURCE_DIR}/patches/iperf_accept_fix.patch || true
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Applying patch to espressif/iperf component"
    USES_TERMINAL
    COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/iperf_patched.stamp
    DEPENDS ${CMAKE_SOURCE_DIR}/managed_components/espressif__iperf/iperf.c
)

add_custom_target(patch_iperf ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/iperf_patched.stamp)

if(DEFINED ENV{IDF_PATH})
    include($ENV{IDF_PATH}/tools/cmake/project.cmake)
else()
    message(FATAL_ERROR "IDF_PATH environment variable is not set. Please run this with idf.py or source export.sh")
endif()
# "Trim" the build. Include the minimal set of components, main, and anything it depends on.
idf_build_set_property(MINIMAL_BUILD ON)
project(beaconbit)
