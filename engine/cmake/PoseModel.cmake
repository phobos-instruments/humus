# Rebuilds the body model for a source distribution, which ships without it:
# cannot obtain = warn, wrong bytes = fatal (docs/dev/build.md).
set(HUM_BODY_MODEL "${CMAKE_CURRENT_SOURCE_DIR}/../packs/av/assets/Models/body/body.humnet")
set(HUM_BODY_SHA256 "a4039232262fc38c7220b58372792ab7358856441191386ecb1988519d9495cb")
set(HUM_BODY_TASK_SHA256 "5134a3aad27a58b93da0088d431f366da362b44e3ccfbe3462b3827a839011b1")
set(HUM_BODY_TASK_URL
    "https://storage.googleapis.com/mediapipe-models/pose_landmarker/pose_landmarker_full/float16/1/pose_landmarker_full.task"
    CACHE STRING "MediaPipe bundle the body model is converted from")

if(NOT EXISTS "${HUM_BODY_MODEL}")
  set(HUM_BODY_WORK "${CMAKE_CURRENT_BINARY_DIR}/body")
  set(HUM_BODY_TASK "${HUM_BODY_WORK}/pose_landmarker_full.task")
  file(REMOVE_RECURSE "${HUM_BODY_WORK}")
  file(MAKE_DIRECTORY "${HUM_BODY_WORK}")
  message(STATUS "body model not in the tree - rebuilding it from ${HUM_BODY_TASK_URL}")

  hum_find_python()
  file(DOWNLOAD "${HUM_BODY_TASK_URL}" "${HUM_BODY_TASK}"
       TLS_VERIFY ON STATUS HUM_BODY_STATUS SHOW_PROGRESS)
  list(GET HUM_BODY_STATUS 0 HUM_BODY_CODE)
  list(GET HUM_BODY_STATUS 1 HUM_BODY_TEXT)

  set(HUM_BODY_WHY "")
  if(NOT HUM_BODY_CODE EQUAL 0)
    set(HUM_BODY_WHY "the bundle could not be downloaded (${HUM_BODY_TEXT})")
  elseif(NOT HUM_PYTHON)
    set(HUM_BODY_WHY "no Python interpreter to run tools/pose_model_convert.py")
  else()
    file(SHA256 "${HUM_BODY_TASK}" HUM_BODY_TASK_GOT)
    if(NOT HUM_BODY_TASK_GOT STREQUAL HUM_BODY_TASK_SHA256)
      message(FATAL_ERROR
        "the bundle at ${HUM_BODY_TASK_URL} is not the one this tree expects.\n"
        "  expected ${HUM_BODY_TASK_SHA256}\n"
        "  got      ${HUM_BODY_TASK_GOT}")
    endif()
    file(ARCHIVE_EXTRACT INPUT "${HUM_BODY_TASK}" DESTINATION "${HUM_BODY_WORK}")
    execute_process(
      COMMAND "${HUM_PYTHON}"
              "${CMAKE_CURRENT_SOURCE_DIR}/../tools/pose_model_convert.py"
              "${HUM_BODY_WORK}/pose_detector.tflite"
              "${HUM_BODY_WORK}/pose_landmarks_detector.tflite"
              "${HUM_BODY_WORK}/body.humnet"
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../tools"
      RESULT_VARIABLE HUM_BODY_CONV OUTPUT_QUIET ERROR_VARIABLE HUM_BODY_CONV_ERR)
    if(NOT HUM_BODY_CONV EQUAL 0)
      set(HUM_BODY_WHY "the conversion failed (${HUM_BODY_CONV_ERR})")
    else()
      file(SHA256 "${HUM_BODY_WORK}/body.humnet" HUM_BODY_GOT)
      if(NOT HUM_BODY_GOT STREQUAL HUM_BODY_SHA256)
        message(FATAL_ERROR
          "the converted body model is not the one this tree expects.\n"
          "  expected ${HUM_BODY_SHA256}\n"
          "  got      ${HUM_BODY_GOT}")
      endif()
      file(COPY "${HUM_BODY_WORK}/body.humnet"
           DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/../packs/av/assets/Models/body")
      message(STATUS "body model rebuilt and verified")
    endif()
  endif()

  if(HUM_BODY_WHY)
    message(WARNING
      "no body model: ${HUM_BODY_WHY}.\n"
      "  Skeleton tracking will do nothing on this build.\n"
      "  packs/av/assets/Models/body/README.md has the commands that build the file by hand.")
  endif()
  file(REMOVE_RECURSE "${HUM_BODY_WORK}")
endif()
