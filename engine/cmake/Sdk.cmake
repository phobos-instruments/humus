# What packs compile against; host-free by design (no io/, no gui/).
add_library(hum_sdk STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../sdk/src/Transport.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../sdk/src/Organism.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../sdk/src/LiveWavWriter.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../sdk/src/NativeCamera.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../sdk/src/NativePicture.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../sdk/src/SerialPort.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../sdk/src/SerialHub.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../sdk/src/SerialSpeed.cpp
)
if(APPLE)
  target_sources(hum_sdk PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../sdk/src/NativeCameraMac.mm)
  target_link_libraries(hum_sdk PUBLIC
    "-framework AVFoundation" "-framework CoreMedia" "-framework CoreVideo"
    "-framework IOSurface")
endif()
target_include_directories(hum_sdk PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../sdk/include)
target_compile_definitions(hum_sdk PUBLIC
  HUM_PACKS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../packs")
target_compile_features(hum_sdk PUBLIC cxx_std_17)
# JUCE and its one configuration come from hum_juce (cmake/JuceShared.cmake).
target_link_libraries(hum_sdk PUBLIC hum_juce)
