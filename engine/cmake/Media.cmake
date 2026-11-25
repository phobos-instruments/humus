# Video and FFmpeg, on their own so a target can choose to go without them.
# hum_media is the real thing: the FFmpeg sound reader, the video layers and
# encoders, FFmpeg itself and the platform video frameworks (hum_video).
# hum_media_none answers the same calls with "not available" (the sound
# reader's own no-FFmpeg branch and gui/video/MediaNone.cpp), and is what
# the Humus plug-in links, so it carries neither FFmpeg nor the video code.
# hum_core and hum_host link neither: each program picks one, the app, the
# snapshot tool, the tests and the CLI through hum_link_video or directly.
# Both are object libraries: their objects go straight into the program, so
# GNU ld finds them whatever order the link line puts them in.
if(HUM_GUI)
  set(HUM_MEDIA_ALL_SOURCES src/io/FfmpegAudioFormat.cpp ${HUM_MEDIA_SOURCES})
else()
  set(HUM_MEDIA_ALL_SOURCES src/io/FfmpegAudioFormat.cpp)
endif()
add_library(hum_media OBJECT ${HUM_MEDIA_ALL_SOURCES})
target_link_libraries(hum_media PUBLIC hum_core hum_ffmpeg)
if(TARGET hum_video)
  target_link_libraries(hum_media PUBLIC hum_video)
endif()
hum_warnings(hum_media)

if(HUM_GUI)
  add_library(hum_media_none OBJECT src/io/FfmpegAudioFormat.cpp src/gui/video/MediaNone.cpp)
  target_link_libraries(hum_media_none PUBLIC hum_core)
  hum_warnings(hum_media_none)
endif()

if(TARGET hum_tests)
  target_link_libraries(hum_tests PRIVATE hum_media)
endif()
