# MiniVideo "library" build system
# This file allows you to build a shared library of MiniVideo
###############################################################################

cmake_minimum_required(VERSION 2.8)
cmake_policy(SET CMP0012 NEW)
cmake_policy(SET CMP0054 NEW)

project(minivideo)

set(minivideo_VERSION_MAJOR 0)
set(minivideo_VERSION_MINOR 8)
set(minivideo_VERSION_PATCH 1)


# Build settings
###############################################################################

set(ENABLE_COLORS 1)        # Toggle colored terminal output
set(ENABLE_C99_STDINT 1)    # Toggle C99 <stdint.h> usage
set(ENABLE_C99_STDBOOL 1)   # Toggle C99 <stdbool.h> usage
set(ENABLE_C11_STDALIGN 0)  # Toggle C11 <stdalign.h> usage

set(ENABLE_WEBP 0)          # Toggle external libwebp support
set(ENABLE_JPEG 0)          # Toggle external libjpeg support
set(ENABLE_PNG 0)           # Toggle external libpng support
set(ENABLE_STBIMWRITE 1)    # Toggle internal stb_image_write library for bmp/png/tga support

set(ENABLE_MEMFD 1)         # Enable memfd support, for Linux kernels 3.17+


# Build dependencies
###############################################################################

# custom memfd module
if(ENABLE_MEMFD)
    SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})
    find_package(MemFD) # FindMemFD.cmake

    if(MEMFD_FOUND)
        message(STATUS "* memfd enabled")
    else()
        message(WARNING "> memfd not found! Disabling MEMFD support.")
        set(ENABLE_MEMFD 0)
    endif()
endif()

# custom libwebp module
if(ENABLE_WEBP)
    SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})
    find_package(WebP) # FindWebP.cmake

    if(WEBP_FOUND)
        message(STATUS "* libwebp enabled")
    else()
        message(WARNING "> libwebp not found! Disabling WEBP support.")
        set(ENABLE_WEBP 0)
    endif()
endif()

# libjpeg module
if(ENABLE_JPEG)
    find_package(JPEG) # FindJPEG.cmake

    if(JPEG_FOUND)
        message(STATUS "* libjpeg enabled")
    else()
        message(WARNING "> libjpeg not found! Disabling JPEG support.")
        set(ENABLE_JPEG 0)
    endif()
endif()

# libpng module
if(ENABLE_PNG)
    find_package(PNG) # FindPNG.cmake

    if(PNG_FOUND)
        message(STATUS "* libpng enabled")
    else()
        message(WARNING "> libpng not found! Disabling external PNG support. 'stb_image_write' library will still provide PNG export.")
        set(ENABLE_PNG 0)
    endif()
endif()

# Doxygen OPTIONAL module
find_package(Doxygen) # FindDoxygen.cmake

if(DOXYGEN_FOUND)
    message(STATUS "* Doxygen found, you can use it to generate a nice API documentation!")
endif(DOXYGEN_FOUND)


# Build configuration
###############################################################################

# Generic build flags
set(CMAKE_C_FLAGS "-std=c99 -D_GNU_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64")

# Detect build type (default is release)
# Change it using "-DCMAKE_BUILD_TYPE= Debug / Release / RelWithDebInfo / MinSizeRel"
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(WARNING "> This is a DEBUG build. You can change this using -DCMAKE_BUILD_TYPE=Release")
    set(ENABLE_DEBUG 1)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g")
    set(CMAKE_VERBOSE_MAKEFILE off)
else()
    message(STATUS "> This is a RELEASE build. You can change this using -DCMAKE_BUILD_TYPE=Debug")
    set(CMAKE_C_FLAGS_RELEASE "-O3 -fPIC -fstack-protector-strong -D_FORTIFY_SOURCE=2")
    set(ENABLE_DEBUG 0)
endif()

# Add per-compiler custom rules
if(${CMAKE_C_COMPILER_ID} MATCHES "Clang")
    message(STATUS "Building with Clang compiler")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wshadow -Wno-unused-function -Wno-unused-parameter -Wno-unused-variable")
elseif(${CMAKE_C_COMPILER_ID} STREQUAL "Intel")
    message(STATUS "Building with Intel C compiler")
elseif(${CMAKE_C_COMPILER_ID} STREQUAL "GNU")
    message(STATUS "Building with GCC compiler")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wshadow -Wno-unused-function -Wno-unused-parameter -Wno-unused-variable -Wno-unused-but-set-variable")
elseif(${CMAKE_C_COMPILER_ID} STREQUAL "MSVC")
    message(STATUS "Building with Visual Studio C compiler")
endif()


# Platform specific
################################################################################

# Linux specific stuff
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

    message(STATUS "Building on Linux plateform")

    set(CMAKE_SHARED_LINKER_FLAGS "-lm -Wl,-z,now -Wl,-z,relro")

endif()

# macOS specific stuff
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

    message(STATUS "Building on macOS plateform")

    set(CMAKE_SHARED_LINKER_FLAGS "-lm")

endif()

# Windows specific stuff
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")

    message(STATUS "Building on Windows plateform")

    # Turn on the ability to create folders to organize projects (.vcproj)
    # It creates "CMakePredefinedTargets" folder by default and adds CMake
    # defined projects like INSTALL.vcproj and ZERO_CHECK.vcproj
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)

    if(CMAKE_VERSION VERSION_GREATER "3.3")
        include(GenerateExportHeader)
    endif()

    set(CMAKE_SHARED_LINKER_FLAGS "-lm -Wl,-no-undefined -Wl,--enable-runtime-pseudo-reloc")

endif()


# Source files
###############################################################################

# Generate "minivideo_settings.h" file from a template
configure_file("${CMAKE_SOURCE_DIR}/src/minivideo_settings.h.in"
               "${CMAKE_SOURCE_DIR}/src/minivideo_settings.h")

# MiniVideo source code
set(minivideo_SRC
    src/minivideo.c
        src/minivideo_settings.h src/minivideo_typedef.h
        src/minivideo_mediafile.h src/minivideo_mediastream.h
        src/minivideo_codecs.c src/minivideo_avutils.h
        src/minivideo_fourcc.c src/minivideo_twocc.c
        src/utils.c src/thirdparty/simple_vector.c
    src/minitraces.c src/minitraces_conf.h
    src/bitstream.c
        src/bitstream_utils.c
        src/bitstream_map.c
    src/import.c
    src/export.c
        src/export_utils.c
        src/thirdparty/stb_image_write.h
    src/demuxer/mp4/mp4.c src/demuxer/mp4/mp4_struct.h src/demuxer/mp4/mp4_box.c
        src/demuxer/mp4/mp4_stbl.c src/demuxer/mp4/mp4_spatial.c src/demuxer/mp4/mp4_convert.c
    src/demuxer/mp3/mp3.c src/demuxer/mp3/mp3_struct.h
    src/demuxer/mkv/mkv.c src/demuxer/mkv/mkv_struct.h src/demuxer/mkv/ebml.c
        src/demuxer/mkv/mkv_tracks.c src/demuxer/mkv/mkv_cluster.c src/demuxer/mkv/mkv_convert.c
    src/demuxer/esparser/esparser.c
    src/demuxer/idr_filter.c
    src/demuxer/xml_mapper.c
    src/demuxer/mpeg/pes/pes.c src/demuxer/mpeg/pes/pes_struct.h
        src/demuxer/mpeg/descriptor_struct.h
        src/demuxer/mpeg/ps/ps.c src/demuxer/mpeg/ps/ps_struct.h
        src/demuxer/mpeg/ts/ts.c src/demuxer/mpeg/ts/ts_struct.h
    src/demuxer/riff/riff.c src/demuxer/riff/riff_common.c src/demuxer/riff/riff_struct.h
        src/demuxer/avi/avi.c src/demuxer/avi/avi_convert.c src/demuxer/avi/avi_struct.h
        src/demuxer/wave/wave.c src/demuxer/wave/wave_convert.c src/demuxer/wave/wave_struct.h
    src/muxer/muxer.c
        src/muxer/pes_packetizer.c
    src/depacketizer/depack.c src/depacketizer/depack_struct.h
        src/depacketizer/h264/depack_h264.c
    src/decoder/h264/h264.c
        src/decoder/h264/h264_decodingcontext.h
        src/decoder/h264/h264_expgolomb.c
        src/decoder/h264/h264_cavlc.c
        src/decoder/h264/h264_cavlc_tables.h
        src/decoder/h264/h264_cabac.c
        src/decoder/h264/h264_cabac_tables.h
        src/decoder/h264/h264_nalu.c
        src/decoder/h264/h264_parameterset.c
        src/decoder/h264/h264_parameterset_struct.h
        src/decoder/h264/h264_slice.c
        src/decoder/h264/h264_slice_struct.h
        src/decoder/h264/h264_macroblock.c
        src/decoder/h264/h264_macroblock_utils.c
        src/decoder/h264/h264_macroblock_struct.h
        src/decoder/h264/h264_intra_prediction.c
        src/decoder/h264/h264_inter_prediction.c
        src/decoder/h264/h264_spatial.c
        src/decoder/h264/h264_transform.c
)


# Build
###############################################################################

# Third party libraries
include_directories(${WEBP_INCLUDE_DIR} ${JPEG_INCLUDE_DIR} ${PNG_INCLUDE_DIR})

# Build shared library
message("** Building shared library")
add_library(minivideo_shared SHARED ${minivideo_SRC})
target_link_libraries(minivideo_shared ${WEBP_LIBRARIES} ${JPEG_LIBRARY} ${PNG_LIBRARY})
set_target_properties(minivideo_shared PROPERTIES OUTPUT_NAME minivideo)

if(CMAKE_BUILD_MODE STREQUAL "Static")
    message("** Building static library")
    add_library(minivideo_static STATIC ${minivideo_SRC})
    target_link_libraries(minivideo_static ${WEBP_LIBRARY} ${JPEG_LIBRARY} ${PNG_LIBRARY})
    set_target_properties(minivideo_static PROPERTIES OUTPUT_NAME minivideo)
endif(CMAKE_BUILD_MODE STREQUAL "Static")

if(WIN32)
if(CMAKE_VERSION VERSION_GREATER "3.3")
    message("* Generating symbols export header")
    GENERATE_EXPORT_HEADER(minivideo_shared
                           BASE_NAME minivideo_shared
                           EXPORT_MACRO_NAME minivideo_EXPORT
                           EXPORT_FILE_NAME "${CMAKE_SOURCE_DIR}/src/minivideo_export.h"
                           STATIC_DEFINE minivideo_BUILT_AS_STATIC)
endif()
endif()


# Install the shared library and its header into the system (optional step, requires root credentials)
# Relative to $<INSTALL_PREFIX>
###############################################################################

if(UNIX)
    install(TARGETS minivideo_shared DESTINATION lib/)
    install(FILES src/minivideo.h DESTINATION include/)
else(WIN32)
    # TODO
endif()
