# The Core ML packages the trackers run on the Neural Engine: derived at
# configure from the committed .humnet by tools/humnet_to_coreml.py, and
# gitignored themselves. No coremltools, or a failed conversion, means the
# interpreter carries the tracker at CPU speed; make dist-macos refuses that.
if(APPLE)
  set(HUM_COREML_PYTHON "" CACHE STRING
      "Python interpreter with coremltools, for the Neural Engine models")

  set(HUM_COREML_CANDIDATES "${HUM_COREML_PYTHON}"
      "${CMAKE_CURRENT_SOURCE_DIR}/../.venv-coreml/bin/python" "python3")
  set(HUM_COREML_FOUND "")
  foreach(candidate IN LISTS HUM_COREML_CANDIDATES)
    if(NOT candidate STREQUAL "" AND NOT HUM_COREML_FOUND)
      execute_process(COMMAND "${candidate}" -c "import coremltools"
                      RESULT_VARIABLE HUM_COREML_PROBE
                      OUTPUT_QUIET ERROR_QUIET)
      if(HUM_COREML_PROBE EQUAL 0)
        set(HUM_COREML_FOUND "${candidate}")
      endif()
    endif()
  endforeach()

  if(NOT HUM_COREML_FOUND)
    message(STATUS
      "no Python with coremltools - trackers use the CPU interpreter "
      "(make dist-macos refuses without the models; "
      "packs/av/assets/Models/body/README.md has the two setup commands)")
  else()
    set(HUM_COREML_CONVERTER
        "${CMAKE_CURRENT_SOURCE_DIR}/../tools/humnet_to_coreml.py")
    foreach(model body hands)
      set(HUM_COREML_SRC
          "${CMAKE_CURRENT_SOURCE_DIR}/../packs/av/assets/Models/${model}/${model}.humnet")
      set(HUM_COREML_OUT
          "${CMAKE_CURRENT_SOURCE_DIR}/../packs/av/assets/Models/${model}")
      set(HUM_COREML_STAMP "${HUM_COREML_OUT}/lm.mlpackage/Manifest.json")
      if(EXISTS "${HUM_COREML_SRC}"
         AND (NOT EXISTS "${HUM_COREML_STAMP}"
              OR "${HUM_COREML_SRC}" IS_NEWER_THAN "${HUM_COREML_STAMP}"
              OR "${HUM_COREML_CONVERTER}" IS_NEWER_THAN "${HUM_COREML_STAMP}"))
        message(STATUS "converting ${model}.humnet for the Neural Engine")
        execute_process(
          COMMAND "${HUM_COREML_FOUND}" "${HUM_COREML_CONVERTER}"
                  "${HUM_COREML_SRC}" "${HUM_COREML_OUT}"
          RESULT_VARIABLE HUM_COREML_CONV
          OUTPUT_QUIET ERROR_VARIABLE HUM_COREML_ERR)
        if(NOT HUM_COREML_CONV EQUAL 0)
          message(WARNING
            "${model} Core ML conversion failed - the interpreter carries "
            "that tracker.\n${HUM_COREML_ERR}")
          file(REMOVE_RECURSE "${HUM_COREML_OUT}/det.mlpackage"
               "${HUM_COREML_OUT}/lm.mlpackage")
        endif()
      endif()
    endforeach()
  endif()
endif()
