# Rules that knows how to habdle flatbuffer files, and generates a header that
# knows how to read them
#
# Emits a dependency called input-flatbuffer, with a property
# called INPUT_FLATBUFFER_INCLUDE, where the generated header file
# is.

include_guard(GLOBAL)

include("${CMAKE_SOURCE_DIR}/cmake/BuildFlatBuffers.cmake")

build_flatbuffers("${CMAKE_SOURCE_DIR}/src/common/input_serialize.fbs" ""
  input-ser-flatbuffer ""
  "${CMAKE_SOURCE_DIR}/generated" "" "")

build_flatbuffers("${CMAKE_SOURCE_DIR}/src/common/network.fbs" ""
  network-flatbuffer ""
  "${CMAKE_SOURCE_DIR}/generated" "" "")

build_flatbuffers("${CMAKE_SOURCE_DIR}/src/common/input.fbs" ""
  input-flatbuffer ""
  "${CMAKE_SOURCE_DIR}/generated" "" "")

