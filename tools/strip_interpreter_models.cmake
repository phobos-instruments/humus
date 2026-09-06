# Usage: cmake -DDIR=<staged packs dir> -P tools/strip_interpreter_models.cmake
#
# A tracker model travels in two forms: the .humnet the portable interpreter
# reads, and the Core ML packages CoreMLModels.cmake derives from it for the
# Neural Engine. A macOS bundle that has the packages never opens the .humnet
# (Hands/Skeleton take Core ML first, BodyPortable.cpp / HandsPortable.cpp),
# so shipping both doubled the model bytes in every dmg. Where the packages
# were produced, drop the .humnet beside them; where the conversion did not
# happen, the interpreter file stays and carries that tracker.
if(NOT DIR)
  message(FATAL_ERROR "strip_interpreter_models.cmake: DIR is required")
endif()
file(GLOB HUM_MODEL_DIRS "${DIR}/*/assets/Models/*")
foreach(model_dir ${HUM_MODEL_DIRS})
  if(EXISTS "${model_dir}/det.mlpackage/Manifest.json"
     AND EXISTS "${model_dir}/lm.mlpackage/Manifest.json")
    file(GLOB HUM_INTERPRETER_MODELS "${model_dir}/*.humnet")
    foreach(f ${HUM_INTERPRETER_MODELS})
      file(REMOVE "${f}")
    endforeach()
  endif()
endforeach()
