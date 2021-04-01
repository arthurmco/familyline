
# Add all files that are common to the server and client:
# (logic, logging, net and object code)

include("${CMAKE_SOURCE_DIR}/src/common/generated.cmake")
get_target_property(INPUT_FLATBUFFER_INCLUDE input-flatbuffer GENERATED_INCLUDES_DIR)
get_target_property(INPUT_FLATBUFFER_INCLUDE input-ser-flatbuffer GENERATED_INCLUDES_DIR)
get_target_property(INPUT_FLATBUFFER_INCLUDE network-flatbuffer GENERATED_INCLUDES_DIR)

add_library(
  familyline-common
  "logger.cpp"
  "logic/action_queue.cpp"
  "logic/attack_manager.cpp"
  "logic/BuildQueue.cpp"
  "logic/colony.cpp"
  "logic/colony_manager.cpp"
  "logic/debug_drawer.cpp"
  "logic/GameActionManager.cpp"
  "logic/game_event.cpp"
  "logic/game_object.cpp"
  "logic/input_recorder.cpp"
  "logic/input_reproducer.cpp"
  "logic/lifecycle_manager.cpp"
  "logic/logic_service.cpp"
  "logic/object_components.cpp"
  "logic/object_factory.cpp"
  "logic/object_listener.cpp"
  "logic/object_manager.cpp"
  "logic/ObjectPathManager.cpp"
  "logic/PathFinder.cpp"
  "logic/player.cpp"
  "logic/replay_player.cpp"
  "logic/player_manager.cpp"
  "logic/terrain.cpp"
  "logic/terrain_file.cpp"
  "objects/Tent.cpp"
  "objects/WatchTower.cpp"
  "net/server_finder.cpp"
  "net/server.cpp"
  "net/game_packet_server.cpp"
  "net/network_client.cpp"
  )

add_dependencies(familyline-common input-flatbuffer input-ser-flatbuffer network-flatbuffer)
 
find_package(fmt CONFIG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(tl-expected CONFIG REQUIRED)

if (MSVC)
	
  include(${CMAKE_TOOLCHAIN_FILE})
  find_package(GLM CONFIG REQUIRED)
  find_package(unofficial-curlpp CONFIG REQUIRED)
  target_include_directories(familyline-common PUBLIC
    ${GLM_INCLUDE_DIRS} ${YAML_INCLUDE_DIR} ${ZLIB_INCLUDE_DIR}
    ${INPUT_FLATBUFFER_INCLUDE})
  target_link_libraries(familyline-common PUBLIC fmt::fmt glm ${ZLIB_LIBRARIES})
  target_link_libraries(familyline-common PUBLIC nlohmann_json::nlohmann_json)
  target_link_libraries(familyline-common PUBLIC unofficial::curlpp::curlpp)
  target_link_libraries(familyline-common PUBLIC tl::expected)
  
else()
  find_package(PkgConfig REQUIRED)
  pkg_search_module(GLM REQUIRED glm) # finds glm
  pkg_search_module(CURLPP REQUIRED curlpp)
  target_link_libraries(familyline-common PUBLIC fmt::fmt ${GLM_LIBRARIES} ${ZLIB_LIBRARIES})
  target_link_libraries(familyline-common PUBLIC nlohmann_json::nlohmann_json)
  target_link_libraries(familyline-common PUBLIC tl::expected)
  target_link_libraries(familyline-common PUBLIC ${CURLPP_LDFLAGS})
  target_include_directories(familyline-common PUBLIC ${GLM_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR} ${INPUT_FLATBUFFER_INCLUDE} ${CURLPP_INCLUDE_DIRS})
endif(MSVC)

if (FLINE_SET_COVERAGE)
  target_compile_options(familyline-common PUBLIC "-fprofile-arcs")
  target_compile_options(familyline-common PUBLIC "-ftest-coverage")
  target_compile_options(familyline-common PUBLIC "-O0")
  target_link_libraries(familyline-common PUBLIC "-fprofile-arcs")
  target_link_libraries(familyline-common PUBLIC "-ftest-coverage")
  target_link_libraries(familyline-common PUBLIC "gcov")
  target_link_libraries(familyline-common PUBLIC "--coverage")
endif()

if (FLINE_DO_CHECK_ASAN)
  target_compile_options(familyline-common PUBLIC "-fsanitize=address")
  target_link_libraries(familyline-common PUBLIC "-fsanitize=address")
endif()


target_include_directories(familyline-common PRIVATE "${CMAKE_SOURCE_DIR}/src/include")
target_compile_features(familyline-common PRIVATE cxx_std_20)

# Retrieve commit number (useful when getting logs from git clones)
execute_process(
  COMMAND git log -1 --format=%h
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE COMMIT_HASH OUTPUT_STRIP_TRAILING_WHITESPACE )

# Retrieve the user who compiled
execute_process(COMMAND whoami OUTPUT_VARIABLE USERNAME
  OUTPUT_STRIP_TRAILING_WHITESPACE)

target_compile_definitions(familyline-common PUBLIC VERSION="${PROJECT_VERSION}"
  COMMIT="${COMMIT_HASH}" USERNAME="${USERNAME}")
