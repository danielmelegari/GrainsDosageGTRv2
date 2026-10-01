cmake_minimum_required(VERSION 3.25)
project(GrainsDosage VERSION 0.13.0 LANGUAGES CXX)

if(MSVC)
  # The SDK selects its CRT using SMTG_USE_STATIC_CRT. Keep it aligned with
  # this project before add_subdirectory creates any SDK or plug-in targets.
  set(SMTG_USE_STATIC_CRT ON CACHE BOOL "Use the static CRT for all Windows targets" FORCE)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "Consistent static MSVC runtime" FORCE)
  add_compile_options(/utf-8)
endif()
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SMTG_ENABLE_VST3_HOSTING_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SMTG_ENABLE_VSTGUI_SUPPORT OFF CACHE BOOL "" FORCE)
set(SMTG_RUN_VST_VALIDATOR ON CACHE BOOL "" FORCE)

# AudioUnit (AU v2) wrapper support for macOS — needed by the Logic Pro AU build.
# Enabled only on Apple; the VST3-only workflow is unaffected because the SDK's
# own default (OFF) stays in place unless a caller passes -DSMTG_ENABLE_AUV2_PLUGIN=ON.
if(APPLE AND SMTG_ENABLE_AUV2_PLUGIN)
  set(SMTG_ENABLE_AUDIOUNIT_WRAPPER ON CACHE BOOL "Build the Steinberg AUv2 wrapper" FORCE)
endif()

if(NOT DEFINED VST3_SDK_ROOT)
  message(FATAL_ERROR "Set -DVST3_SDK_ROOT=/path/to/vst3sdk (with submodules)")
endif()
add_subdirectory("${VST3_SDK_ROOT}" "${CMAKE_BINARY_DIR}/vst3sdk")
set(SDK_ROOT "${VST3_SDK_ROOT}")
set(public_sdk_SOURCE_DIR "${VST3_SDK_ROOT}/public.sdk")
set(pluginterfaces_SOURCE_DIR "${VST3_SDK_ROOT}/pluginterfaces")

smtg_add_vst3plugin(GrainsDosage
  src/plugin.cpp
  src/rhythmic_loop.h
  src/master_fx.h
  src/level_match.h
  src/step_reverb.h
  src/preset_io.h
  src/factory_presets.h
  src/user_presets.h
  src/gater.h
  src/reslice.h
  src/filter_bank.h
  src/space_reverb.h
  src/declick.h
  src/engine.h
  src/stretch.h
  src/transpose.h
  src/modulation.h
  src/parameters.h
  src/editor.h
)
target_link_libraries(GrainsDosage PRIVATE sdk)
smtg_target_configure_version_file(GrainsDosage)
if(APPLE)
  enable_language(OBJCXX)
  set_property(TARGET GrainsDosage PROPERTY OBJCXX_STANDARD 17)
  set_property(TARGET GrainsDosage PROPERTY OBJCXX_STANDARD_REQUIRED ON)
  file(GLOB GRAINS_SPRITES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/assets/sprites/*.png")
  set_source_files_properties(${GRAINS_SPRITES} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
  target_sources(GrainsDosage PRIVATE ${GRAINS_SPRITES})
  set_source_files_properties(assets/GrainsDosage-skin.png PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
  target_sources(GrainsDosage PRIVATE src/editor_mac.mm assets/GrainsDosage-skin.png)
  set_source_files_properties(src/editor_mac.mm PROPERTIES COMPILE_FLAGS "-fobjc-arc")
  find_library(AZTEC_COCOA_FRAMEWORK Cocoa REQUIRED)
  find_library(AZTEC_COREFOUNDATION_FRAMEWORK CoreFoundation REQUIRED)
  target_link_libraries(GrainsDosage PRIVATE ${AZTEC_COCOA_FRAMEWORK} ${AZTEC_COREFOUNDATION_FRAMEWORK})

  # Optional AudioUnit (AU v2) variant for Logic Pro / GarageBand. Enabled with
  # -DSMTG_ENABLE_AUV2_PLUGIN=ON; adds the Steinberg VST3->AU wrapper sources
  # and links the required Apple frameworks onto the same plugin target.
  if(SMTG_ENABLE_AUV2_PLUGIN)
    smtg_target_add_plugin_sources(GrainsDosage
      "${VST3_SDK_ROOT}/public.sdk/source/vst/auWRAPPER"
      "${VST3_SDK_ROOT}/public.sdk/source/vst/shared/vst3classids.cpp")
    find_library(AU_AUDIOUNIT_FRAMEWORK AudioUnit REQUIRED)
    find_library(AU_AUDIOTOOLBOX_FRAMEWORK AudioToolbox REQUIRED)
    find_library(AU_COREAUDIO_FRAMEWORK CoreAudio REQUIRED)
    find_library(AU_CARBON_FRAMEWORK Carbon REQUIRED)
    target_link_libraries(GrainsDosage PRIVATE
      ${AU_AUDIOUNIT_FRAMEWORK} ${AU_AUDIOTOOLBOX_FRAMEWORK}
      ${AU_COREAUDIO_FRAMEWORK} ${AU_CARBON_FRAMEWORK})
  endif()
  # A native smoke test and screenshot, run on the macOS CI runner.
  add_executable(aztec_gui_smoke src/gui_smoke.mm src/editor_mac.mm)
  set_source_files_properties(src/gui_smoke.mm PROPERTIES COMPILE_FLAGS "-fobjc-arc")
  set_target_properties(aztec_gui_smoke PROPERTIES
    OBJCXX_STANDARD 17 OBJCXX_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/gui-test/$<CONFIG>")
  target_link_libraries(aztec_gui_smoke PRIVATE sdk ${AZTEC_COCOA_FRAMEWORK} ${AZTEC_COREFOUNDATION_FRAMEWORK})
  smtg_target_set_bundle(GrainsDosage BUNDLE_IDENTIFIER com.danielmelegari.aztecgrains COMPANY_NAME "Daniel Melegari")
endif()

if(WIN32)
  enable_language(RC)
  configure_file(src/skin.rc.in "${CMAKE_CURRENT_BINARY_DIR}/grains_skin.rc" @ONLY)
  target_sources(GrainsDosage PRIVATE src/editor_win.cpp "${CMAKE_CURRENT_BINARY_DIR}/grains_skin.rc")
  target_link_libraries(GrainsDosage PRIVATE user32 gdi32 comdlg32 shell32 gdiplus ole32)
  add_executable(aztec_gui_smoke_win src/gui_smoke_win.cpp src/editor_win.cpp "${CMAKE_CURRENT_BINARY_DIR}/grains_skin.rc")
  target_link_libraries(aztec_gui_smoke_win PRIVATE sdk user32 gdi32 comdlg32 shell32 gdiplus ole32)
  set_target_properties(aztec_gui_smoke_win PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/gui-test/$<CONFIG>")
endif()

option(GRAINS_BUILD_TESTS "Build DSP and state regression tests" OFF)
if(GRAINS_BUILD_TESTS)
  enable_testing()
  foreach(suite engine modulation features rhythm random master upgrade gater declick expansion state glitch_trigger musical gui_layout)
    add_executable(grains_test_${suite} test_${suite}.cpp)
    target_include_directories(grains_test_${suite} PRIVATE "${VST3_SDK_ROOT}")
    if(MSVC)
      target_compile_options(grains_test_${suite} PRIVATE /UNDEBUG)
    else()
      target_compile_options(grains_test_${suite} PRIVATE -UNDEBUG)
    endif()
    if(suite STREQUAL "state")
      target_sources(grains_test_${suite} PRIVATE
        "${VST3_SDK_ROOT}/public.sdk/source/common/memorystream.cpp"
        "${VST3_SDK_ROOT}/public.sdk/source/vst/hosting/parameterchanges.cpp")
      target_link_libraries(grains_test_${suite} PRIVATE sdk)
      # test_state includes plugin.cpp, whose controller references createEditor.
      # Link the real platform editor and its frameworks, just as for the plugin.
      if(APPLE)
        target_sources(grains_test_${suite} PRIVATE src/editor_mac.mm)
        set_target_properties(grains_test_${suite} PROPERTIES
          OBJCXX_STANDARD 17 OBJCXX_STANDARD_REQUIRED ON)
        target_link_libraries(grains_test_${suite} PRIVATE
          ${AZTEC_COCOA_FRAMEWORK} ${AZTEC_COREFOUNDATION_FRAMEWORK})
      elseif(WIN32)
        target_sources(grains_test_${suite} PRIVATE
          src/editor_win.cpp "${CMAKE_CURRENT_BINARY_DIR}/grains_skin.rc")
        target_link_libraries(grains_test_${suite} PRIVATE
          user32 gdi32 comdlg32 shell32 gdiplus ole32)
      endif()
    endif()
    add_test(NAME ${suite} COMMAND grains_test_${suite})
  endforeach()
endif()
