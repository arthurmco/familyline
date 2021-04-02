
# Add all files that are exclusive to the client:
# (rendering and input gathering code)

include("${CMAKE_SOURCE_DIR}/src/common/generated.cmake")

add_library(
  familyline-client
  "params.cpp"
  "config_reader.cpp"
  "game.cpp"
  "preview_renderer.cpp"
  "player_enumerator.cpp"
  "graphical/asset_file.cpp"
  "graphical/asset_manager.cpp"
  "graphical/asset_object.cpp"
  "graphical/camera.cpp"
  "graphical/deform_animator.cpp"
  "graphical/exceptions.cpp"
  "graphical/gfx_debug_drawer.cpp"
  "graphical/gfx_service.cpp"
  "graphical/GraphicalPlotInterface.cpp"
  "graphical/gui/gui_button.cpp"
  "graphical/gui/control.cpp"
  "graphical/gui/root_control.cpp"
  "graphical/gui/gui_container_component.cpp"
  "graphical/gui/gui_imageview.cpp"
  "graphical/gui/gui_label.cpp"
  "graphical/gui/gui_checkbox.cpp"
  "graphical/gui/gui_listbox.cpp"
  "graphical/gui/gui_textbox.cpp"
  "graphical/gui/gui_manager.cpp"
  "graphical/gui/gui_window.cpp"
  "graphical/material.cpp"
  "graphical/material_manager.cpp"
  "graphical/materialopener/MTLOpener.cpp"
  "graphical/mesh.cpp"
  "graphical/meshopener/MD2Opener.cpp"
  "graphical/meshopener/MeshOpener.cpp"
  "graphical/meshopener/OBJOpener.cpp"
  "graphical/object_renderer.cpp"
  "graphical/scene_manager.cpp"
  "graphical/shader_manager.cpp"
  "graphical/static_animator.cpp"
  "graphical/Texture.cpp"
  "graphical/TextureFile.cpp"
  "graphical/texture_manager.cpp"
  "graphical/TextureOpener.cpp"
  "graphical/vertexdata.cpp"
  "graphical/opengl/gl_device.cpp"
  "graphical/opengl/gl_window.cpp"
  "graphical/opengl/gl_renderer.cpp"
  "graphical/opengl/gl_terrain_renderer.cpp"
  "graphical/opengl/gl_framebuffer.cpp"
  "graphical/opengl/gl_gui_manager.cpp"
  "graphical/opengl/gl_shader.cpp"
  "HumanPlayer.cpp"
  "input/Cursor.cpp"
  "input/input_processor.cpp"
  "input/input_manager.cpp"
  "input/InputPicker.cpp"
  "input/input_service.cpp"
  "Timer.cpp"
  )


if (MSVC)

  include(${CMAKE_TOOLCHAIN_FILE})
  find_library(PANGO_LIB pango-1.0)
  find_library(PANGOCAIRO_LIB pangocairo-1.0)
  find_library(GOBJECT_LIB gobject-2.0)
  find_path(PANGO_INCLUDE pango/pango.h)

  target_link_libraries(familyline-client PUBLIC ${PANGO_LIB} ${PANGOCAIRO_LIB} ${GOBJECT_LIB})
  target_include_directories(familyline-client PUBLIC ${PANGO_INCLUDE} ${GLEW_INCLUDE_DIRS} ${SDL2_INCLUDE_DIRS} ${OPENGL_INCLUDE_DIR} ${IL_INCLUDE_DIRS} ${CAIRO_INCLUDE_DIRS} ${GLM_INCLUDE_DIRS} ${YAML_INCLUDE_DIR})

else()
  find_package(PkgConfig REQUIRED)

  pkg_search_module(PANGOCAIRO REQUIRED pangocairo) # finds pango-cairo
  target_include_directories(familyline-client PUBLIC ${PANGOCAIRO_INCLUDE_DIRS})
  
endif(MSVC)

target_include_directories(familyline-client PRIVATE "${CMAKE_SOURCE_DIR}/src/include" )
target_link_libraries(familyline-client PUBLIC familyline-common)
target_compile_features(familyline-client PRIVATE cxx_std_20)


if (FLINE_DO_CHECK_ASAN)
  if (WIN32)
    message("No support for windows + address sanitizer yet")
  else() 
    target_compile_options(familyline-client PUBLIC "-fsanitize=address")
    target_link_libraries(familyline-client PUBLIC "-fsanitize=address")
    target_compile_options(familyline-client PUBLIC "-fno-omit-frame-pointer")
    target_link_libraries(familyline-client PUBLIC "-fno-omit-frame-pointer")
  endif()
endif()
