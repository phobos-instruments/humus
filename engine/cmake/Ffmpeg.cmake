# FFmpeg is probed on every platform, because reading a video's sound is
# something no system audio reader does from a Matroska or an AVI, and on
# Linux and Windows not from an MP4 either. Where the OS has a video decoder
# of its own it stays in front for the picture and FFmpeg is the fallback;
# where it has none, FFmpeg is the decoder. macOS and Windows take the
# decode-only LGPL library packaging/linux/ffmpeg-lite.sh makes - no x264, no
# muxers, nothing GPL - while Linux takes its GPL one, which also carries the
# x264 a bounce encodes with.
option(HUM_FFMPEG "Read video sound, and play what the system cannot, through FFmpeg" ON)
set(HUM_FFMPEG_ROOT "" CACHE PATH
  "Prefix of a private FFmpeg build to link and bundle; empty = the system's")
set(HUM_FFMPEG_FOUND OFF)

add_library(hum_ffmpeg INTERFACE)

if(HUM_FFMPEG)
  find_package(PkgConfig QUIET)
  if(PkgConfig_FOUND)
    if(HUM_FFMPEG_ROOT)
      set(ENV{PKG_CONFIG_PATH} "${HUM_FFMPEG_ROOT}/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
    endif()
    # Re-probed every configure, or a moved HUM_FFMPEG_ROOT keeps the old answer.
    unset(HUM_FFMPEG_LIBS_FOUND CACHE)
    foreach(lib avcodec avformat avutil swscale swresample)
      unset(pkgcfg_lib_HUM_FFMPEG_LIBS_${lib} CACHE)
    endforeach()
    pkg_check_modules(HUM_FFMPEG_LIBS QUIET IMPORTED_TARGET
      libavcodec libavformat libavutil libswscale libswresample)
  endif()
endif()

# Windows has no pkg-config, so a private build is found by looking in it.
if(HUM_FFMPEG AND NOT HUM_FFMPEG_LIBS_FOUND AND HUM_FFMPEG_ROOT)
  find_path(HUM_FFMPEG_INCLUDE libavcodec/avcodec.h
    PATHS "${HUM_FFMPEG_ROOT}/include" NO_DEFAULT_PATH)
  set(HUM_FFMPEG_PLAIN_LIBS "")
  foreach(lib avformat avcodec swresample swscale avutil)
    find_library(HUM_FFMPEG_LIB_${lib} NAMES ${lib} lib${lib}
      PATHS "${HUM_FFMPEG_ROOT}/lib" NO_DEFAULT_PATH)
    if(HUM_FFMPEG_LIB_${lib})
      list(APPEND HUM_FFMPEG_PLAIN_LIBS "${HUM_FFMPEG_LIB_${lib}}")
    endif()
  endforeach()
  list(LENGTH HUM_FFMPEG_PLAIN_LIBS HUM_FFMPEG_PLAIN_COUNT)
  if(HUM_FFMPEG_INCLUDE AND HUM_FFMPEG_PLAIN_COUNT EQUAL 5)
    set(HUM_FFMPEG_FOUND ON)
    target_include_directories(hum_ffmpeg SYSTEM INTERFACE "${HUM_FFMPEG_INCLUDE}")
    target_link_libraries(hum_ffmpeg INTERFACE ${HUM_FFMPEG_PLAIN_LIBS})
    if(WIN32)
      target_link_libraries(hum_ffmpeg INTERFACE bcrypt secur32 ws2_32 mfplat strmiids)
    endif()
    target_compile_definitions(hum_ffmpeg INTERFACE HUM_FFMPEG=1)
    message(STATUS "media decode: FFmpeg from ${HUM_FFMPEG_ROOT}")
  endif()
endif()

if(HUM_FFMPEG_LIBS_FOUND)
  set(HUM_FFMPEG_FOUND ON)
  target_link_libraries(hum_ffmpeg INTERFACE PkgConfig::HUM_FFMPEG_LIBS)
  target_compile_definitions(hum_ffmpeg INTERFACE HUM_FFMPEG=1)
  message(STATUS "media decode: FFmpeg libavcodec ${HUM_FFMPEG_LIBS_libavcodec_VERSION}")
elseif(NOT HUM_FFMPEG_FOUND AND NOT APPLE AND NOT WIN32)
  message(WARNING
    "no FFmpeg development files (libavcodec-dev libavformat-dev libswscale-dev "
    "libswresample-dev): VideoPlayer will play HAP videos only in this build, and "
    "no video will give up its sound.")
endif()
