#
# Unit test compilation routines
#
# Copyright (C) 2017,2021 Arthur M
#

if (FLINE_BUILD_TESTS)
  enable_testing()

  configure_file(${CMAKE_SOURCE_DIR}/test/unit-tests.cmake.in googletest-download/CMakeLists.txt)
  execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
    RESULT_VARIABLE result
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/googletest-download )

  if(result)
    message(FATAL_ERROR "CMake step for googletest failed: ${result}")
  endif()

  execute_process(COMMAND ${CMAKE_COMMAND} --build .
    RESULT_VARIABLE result
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/googletest-download )

  if(result)
    message(FATAL_ERROR "Build step for googletest failed: ${result}")
  endif()

  # Prevent overriding the parent project's compiler/linker
  # settings on Windows
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

  # Add googletest directly to our build. This defines
  # the gtest and gtest_main targets.
  add_subdirectory(${CMAKE_SOURCE_DIR}/test/googletest
    ${CMAKE_BINARY_DIR}/googletest-build)

  set( SRC_TEST_FILES
    "${CMAKE_SOURCE_DIR}/test/test_colony_manager.cpp"
    "${CMAKE_SOURCE_DIR}/test/test_game.cpp"
    "${CMAKE_SOURCE_DIR}/test/test_input_recorder.cpp"
    "${CMAKE_SOURCE_DIR}/test/test_input_reproducer.cpp"
    "${CMAKE_SOURCE_DIR}/test/test_model_opener.cpp"
    "${CMAKE_SOURCE_DIR}/test/test_object_attack.cpp"
    "${CMAKE_SOURCE_DIR}/test/test_object_factory.cpp"
    "${CMAKE_SOURCE_DIR}/test/test_object_operations.cpp"
    "${CMAKE_SOURCE_DIR}/test/test_pathfinder.cpp"
    "${CMAKE_SOURCE_DIR}/test/test_player_manager.cpp"
    "${CMAKE_SOURCE_DIR}/test/test_scene_manager.cpp"
    "${CMAKE_SOURCE_DIR}/test/test_terrain.cpp"
    "${CMAKE_SOURCE_DIR}/test/tests.cpp"
    "${CMAKE_SOURCE_DIR}/test/utils.cpp"
    "${CMAKE_SOURCE_DIR}/test/utils/test_device.cpp"
    )
  
  file( GLOB SRC_UTIL_FILES "${CMAKE_SOURCE_DIR}/test/utils/test_renderer.cpp" )

  message("${SRC_SERVER_TEST}")

  find_package(OpenGL REQUIRED)

  add_executable(familyline-tests ${SRC_TEST_FILES} ${SRC_UTIL_FILES})
  target_link_libraries(familyline-tests familyline-common)
  target_link_libraries(familyline-tests familyline-client)
  target_link_libraries(familyline-tests ${OPENGL_LIBRARIES})
  target_link_libraries(familyline-tests ${YAML_LIBRARIES}) # You need to build YAML as a dll (use -DB  target_compile_features(familyline-tests PUBLIC cxx_std_20)

  target_compile_features(familyline-tests PUBLIC cxx_std_20)

  target_include_directories(familyline-tests PRIVATE "${CMAKE_SOURCE_DIR}/src/include")

  file(COPY "${CMAKE_SOURCE_DIR}/test" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Debug")

  if (FLINE_SET_COVERAGE)
    target_compile_options(familyline-tests PUBLIC "-fprofile-arcs")
    target_compile_options(familyline-tests PUBLIC "-ftest-coverage")
    target_compile_options(familyline-tests PUBLIC "-O0")
    target_link_libraries(familyline-tests "-fprofile-arcs")
    target_link_libraries(familyline-tests "-ftest-coverage")
    target_link_libraries(familyline-tests "gcov")
    target_link_libraries(familyline-tests "--coverage")
  endif()

  target_link_libraries( familyline-tests
    gtest gtest_main)

  if (FLINE_DO_CHECK_ASAN)
    if (WIN32)
      message("No support for windows + address sanitizer yet")
    else()
      target_compile_options(familyline-tests PUBLIC "-fsanitize=address")
      target_link_libraries( familyline-tests "-ggdb -fsanitize=address")
    endif()
  endif(FLINE_DO_CHECK_ASAN)


  target_compile_definitions(familyline-tests PUBLIC
    TESTS_DIR="${CMAKE_SOURCE_DIR}/test"
  )

  add_test(NAME general-test COMMAND familyline-tests)
endif(FLINE_BUILD_TESTS)
