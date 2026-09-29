cmake_minimum_required(VERSION 3.25)
project(GrainsDosage VERSION 0.10.1 LANGUAGES CXX)

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
  src/gater.h
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
  set_source_files_properties(assets/GrainsDosage-skin.png PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
  target_sources(GrainsDosage PRIVATE src/editor_mac.mm assets/GrainsDosage-skin.png)
  set_source_files_properties(src/editor_mac.mm PROPERTIES COMPILE_FLAGS "-fobjc-arc")
  find_library(AZTEC_COCOA_FRAMEWORK Cocoa REQUIRED)
  target_link_libraries(GrainsDosage PRIVATE ${AZTEC_COCOA_FRAMEWORK})
  # A native smoke test and screenshot, run on the macOS CI runner.
  add_executable(aztec_gui_smoke src/gui_smoke.mm src/editor_mac.mm)
  set_source_files_properties(src/gui_smoke.mm PROPERTIES COMPILE_FLAGS "-fobjc-arc")
  set_target_properties(aztec_gui_smoke PROPERTIES
    OBJCXX_STANDARD 17 OBJCXX_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/gui-test/$<CONFIG>")
  target_link_libraries(aztec_gui_smoke PRIVATE sdk ${AZTEC_COCOA_FRAMEWORK})
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
  foreach(suite engine modulation features rhythm random master upgrade gater declick)
    add_executable(grains_test_${suite} test_${suite}.cpp)
    target_include_directories(grains_test_${suite} PRIVATE "${VST3_SDK_ROOT}")
    if(MSVC)
      target_compile_options(grains_test_${suite} PRIVATE /UNDEBUG)
    else()
      target_compile_options(grains_test_${suite} PRIVATE -UNDEBUG)
    endif()
    add_test(NAME ${suite} COMMAND grains_test_${suite})
  endforeach()
endif()
