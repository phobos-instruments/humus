# Rebuilds the hand model for a source distribution, which ships without it:
# cannot obtain = warn, wrong bytes = fatal (docs/dev/build.md).
set(HUM_HANDS_MODEL "${CMAKE_CURRENT_SOURCE_DIR}/../packs/av/assets/Models/hands/hands.humnet")
set(HUM_HANDS_SHA256 "29f7f8ab9e50dc1eb82cc904262198edb150c7473a856b79352e4b85c7976b85")
set(HUM_HANDS_TASK_SHA256 "fbc2a30080c3c557093b5ddfc334698132eb341044ccee322ccf8bcf3607cde1")
set(HUM_HANDS_TASK_URL
    "https://storage.googleapis.com/mediapipe-models/hand_landmarker/hand_landmarker/float16/1/hand_landmarker.task"
    CACHE STRING "MediaPipe bundle the hand model is converted from")

if(NOT EXISTS "${HUM_HANDS_MODEL}")
  set(HUM_HANDS_WORK "${CMAKE_CURRENT_BINARY_DIR}/hands")
  set(HUM_HANDS_TASK "${HUM_HANDS_WORK}/hand_landmarker.task")
  file(REMOVE_RECURSE "${HUM_HANDS_WORK}")
  file(MAKE_DIRECTORY "${HUM_HANDS_WORK}")
  message(STATUS "hand model not in the tree - rebuilding it from ${HUM_HANDS_TASK_URL}")

  hum_find_python()
  file(DOWNLOAD "${HUM_HANDS_TASK_URL}" "${HUM_HANDS_TASK}"
       TLS_VERIFY ON STATUS HUM_HANDS_STATUS SHOW_PROGRESS)
  list(GET HUM_HANDS_STATUS 0 HUM_HANDS_CODE)
  list(GET HUM_HANDS_STATUS 1 HUM_HANDS_TEXT)

  set(HUM_HANDS_WHY "")
  if(NOT HUM_HANDS_CODE EQUAL 0)
    set(HUM_HANDS_WHY "the bundle could not be downloaded (${HUM_HANDS_TEXT})")
  else()
    file(SHA256 "${HUM_HANDS_TASK}" HUM_HANDS_TASK_GOT)
    if(NOT HUM_HANDS_TASK_GOT STREQUAL HUM_HANDS_TASK_SHA256)
      message(FATAL_ERROR
        "the bundle at ${HUM_HANDS_TASK_URL} is not the one this tree expects.\n"
        "  expected ${HUM_HANDS_TASK_SHA256}\n"
        "  got      ${HUM_HANDS_TASK_GOT}")
    endif()
    file(ARCHIVE_EXTRACT INPUT "${HUM_HANDS_TASK}" DESTINATION "${HUM_HANDS_WORK}")
    execute_process(
      COMMAND "${HUM_PYTHON}"
              "${CMAKE_CURRENT_SOURCE_DIR}/../tools/hand_model_convert.py"
              "${HUM_HANDS_WORK}/hand_detector.tflite"
              "${HUM_HANDS_WORK}/hand_landmarks_detector.tflite"
              "${HUM_HANDS_WORK}/hands.humnet"
      RESULT_VARIABLE HUM_HANDS_CONV OUTPUT_QUIET ERROR_VARIABLE HUM_HANDS_CONV_ERR)
    if(NOT HUM_HANDS_CONV EQUAL 0)
      set(HUM_HANDS_WHY "the conversion failed (${HUM_HANDS_CONV_ERR})")
    else()
      file(SHA256 "${HUM_HANDS_WORK}/hands.humnet" HUM_HANDS_GOT)
      if(NOT HUM_HANDS_GOT STREQUAL HUM_HANDS_SHA256)
        message(FATAL_ERROR
          "the converted hand model is not the one this tree expects.\n"
          "  expected ${HUM_HANDS_SHA256}\n"
          "  got      ${HUM_HANDS_GOT}")
      endif()
      file(COPY "${HUM_HANDS_WORK}/hands.humnet"
           DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/../packs/av/assets/Models/hands")
      message(STATUS "hand model rebuilt and verified")
    endif()
  endif()

  if(HUM_HANDS_WHY)
    message(WARNING
      "no hand model: ${HUM_HANDS_WHY}.\n"
      "  Hand tracking will do nothing where the system has no tracker of its own.\n"
      "  packs/av/assets/Models/hands/README.md has the commands that build the file by hand.")
  endif()
  file(REMOVE_RECURSE "${HUM_HANDS_WORK}")
endif()
