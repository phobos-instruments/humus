# builtin = linked into the app; addon = shipped only as a .humpack;
# tutorial = built dynamically, never linked.
set(HUM_PACK_ROLE tutorial PARENT_SCOPE)

# Sample pack - the SDK tutorial. Unlike the other packs this one is NOT
# statically linked into Humus: it exists only as a .humpack bundle, exactly
# like a third-party pack would (install it via Edit > Organism Manager).
file(GLOB HUM_SAMPLE_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/organisms/*/*.cpp)

add_library(hum_pack_sample STATIC
  register.cpp
  ${HUM_SAMPLE_SOURCES}
)
target_include_directories(hum_pack_sample PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/organisms)
target_link_libraries(hum_pack_sample PUBLIC hum_sdk)

hum_add_pack_dyn(sample)
hum_pack_bundle(sample)
