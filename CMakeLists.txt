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
# The Steinberg SDK enables its AUv2 machinery only when it finds an Apple
# AudioUnitSDK checkout; see the resolution block below. The VST3-only workflow
# is unaffected: nothing changes unless -DSMTG_ENABLE_AUV2_PLUGIN=ON is passed.
# Resolve the Apple AudioUnit SDK location BEFORE the sdk subproject runs its
# CoreAudio support setup (which only auto-enables AUv2 builds when it finds an
# AudioUnitSDK.xcodeproj). If not provided and not already sitting next to the
# sdk, fetch it once from Apple's official repo.
if(APPLE AND SMTG_ENABLE_AUV2_PLUGIN)
  # Always validate/resolve here (a caller-provided path may point at a folder
  # that does not exist yet — CI passes it before the SDK auto-fetch runs).
  get_filename_component(_gd_au_parent "${VST3_SDK_ROOT}" DIRECTORY)
  set(_gd_au_fallback "${_gd_au_parent}/AudioUnitSDK")
  set(_gd_au_sdk "${SMTG_AUDIOUNIT_SDK_PATH}")
  if(_gd_au_sdk STREQUAL "" OR NOT EXISTS "${_gd_au_sdk}/AudioUnitSDK.xcodeproj")
    set(_gd_au_sdk "${_gd_au_fallback}")
  endif()
  if(NOT EXISTS "${_gd_au_sdk}/AudioUnitSDK.xcodeproj")
    find_package(Git REQUIRED)
    execute_process(COMMAND "${GIT_EXECUTABLE}" clone --depth 1
      https://github.com/Apple/AudioUnitSDK.git "${_gd_au_sdk}"
      RESULT_VARIABLE _gd_au_clone OUTPUT_VARIABLE _gd_au_clone_msg ERROR_VARIABLE _gd_au_clone_err)
    if(NOT _gd_au_clone EQUAL 0)
      message(FATAL_ERROR "[GrainsDosage] could not clone Apple AudioUnitSDK into ${_gd_au_sdk}: ${_gd_au_clone_msg}${_gd_au_clone_err}")
    endif()
  endif()
  if(NOT EXISTS "${_gd_au_sdk}/AudioUnitSDK.xcodeproj")
    message(FATAL_ERROR "[GrainsDosage] AudioUnitSDK present at ${_gd_au_sdk} but AudioUnitSDK.xcodeproj is missing")
  endif()
  message(STATUS "[GrainsDosage] using Apple AudioUnitSDK at ${_gd_au_sdk}")
  set(SMTG_AUDIOUNIT_SDK_PATH "${_gd_au_sdk}" CACHE PATH "Path to the AudioUnit SDK" FORCE)
  # The SDK builds the hosting library only with hosting examples enabled;
  # the AU wrapper needs sdk_hosting, so force it ON just for AU builds.
  set(SMTG_ENABLE_VST3_HOSTING_EXAMPLES ON CACHE BOOL "" FORCE)
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
# Built against the SDK: parameters.h must use the real Steinberg::Vst::ParamID.
target_compile_definitions(GrainsDosage PRIVATE GRAINS_HAVE_VST3_SDK)
smtg_target_configure_version_file(GrainsDosage)
if(APPLE)
  enable_language(OBJCXX)
  set_property(TARGET GrainsDosage PROPERTY OBJCXX_STANDARD 17)
  set_property(TARGET GrainsDosage PROPERTY OBJCXX_STANDARD_REQUIRED ON)
  configure_file(assets/mockup/knob.png "${CMAKE_CURRENT_BINARY_DIR}/mockup-knob.png" COPYONLY)
  set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/mockup-knob.png" PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
  target_sources(GrainsDosage PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/mockup-knob.png")
  # knobOK.png is the user-supplied knob face: copy it over the shipped knob
  # sprite so both the macOS bundle and the Windows RC resource pick it up.
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/workspace/assets/knobOK.png")
    configure_file(workspace/assets/knobOK.png assets/sprites/knob.png COPYONLY)
  endif()
  file(GLOB GRAINS_SPRITES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/assets/sprites/*.png")
  set_source_files_properties(${GRAINS_SPRITES} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
  target_sources(GrainsDosage PRIVATE ${GRAINS_SPRITES})
  set_source_files_properties(assets/GrainsDosage-skin.png PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
  target_sources(GrainsDosage PRIVATE src/editor_mac.mm assets/GrainsDosage-skin.png)
  set_source_files_properties(src/editor_mac.mm PROPERTIES COMPILE_FLAGS "-fobjc-arc")
  find_library(AZTEC_COCOA_FRAMEWORK Cocoa REQUIRED)
  find_library(AZTEC_COREFOUNDATION_FRAMEWORK CoreFoundation REQUIRED)
  target_link_libraries(GrainsDosage PRIVATE ${AZTEC_COCOA_FRAMEWORK} ${AZTEC_COREFOUNDATION_FRAMEWORK})

  # Optional AudioUnit (AU v2) bundle for Logic Pro / GarageBand. Enabled with
  # -DSMTG_ENABLE_AUV2_PLUGIN=ON: builds a separate GrainsDosage.component whose
  # binary is the Steinberg VST3->AU wrapper, embedding the freshly built
  # GrainsDosage.vst3 via Resources/plugin.vst3 (same code path as the SDK's own
  # AUv2 samples). Requires the Xcode generator and an Apple AudioUnitSDK checkout.
  if(SMTG_ENABLE_AUV2_PLUGIN AND SMTG_ENABLE_AUV2_BUILDS)
    configure_file(src/au_info.plist.in "${CMAKE_CURRENT_BINARY_DIR}/au_info.plist" COPYONLY)
    smtg_target_add_auv2(GrainsDosageAU
      BUNDLE_NAME            "GrainsDosage"
      BUNDLE_IDENTIFIER      com.danielmelegari.aztecgrains.au
      INFO_PLIST_TEMPLATE    "${CMAKE_CURRENT_BINARY_DIR}/au_info.plist"
      VST3_PLUGIN_TARGET     GrainsDosage
    )
  elseif(SMTG_ENABLE_AUV2_PLUGIN)
    message(FATAL_ERROR
      "[GrainsDosage] -DSMTG_ENABLE_AUV2_PLUGIN=ON was requested but the SDK did not "
      "enable AUv2 builds. Use the Xcode generator (-G Xcode) and make sure the Apple "
      "AudioUnitSDK is available (SMTG_AUDIOUNIT_SDK_PATH or cloned next to the sdk).")
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
  # Post-build: drop the editable skin folder next to the built bundle
  # (build/VST3/<cfg>/GrainsDosage.vst3/Contents/x86_64-win/GrainsDosage-skin/).
  # The Windows editor loads <name>.png from that folder before falling back to
  # the RC-baked factory images, so users can reskin by swapping PNGs only.
  add_custom_command(TARGET GrainsDosage POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${CMAKE_CURRENT_SOURCE_DIR}/assets/GrainsDosage-skin"
      "$<TARGET_FILE_DIR:GrainsDosage>/GrainsDosage-skin"
    COMMENT "Copying replaceable skin PNGs next to GrainsDosage.vst3")
  add_executable(aztec_gui_smoke_win src/gui_smoke_win.cpp src/editor_win.cpp "${CMAKE_CURRENT_BINARY_DIR}/grains_skin.rc")
  target_link_libraries(aztec_gui_smoke_win PRIVATE sdk user32 gdi32 comdlg32 shell32 gdiplus ole32)
  set_target_properties(aztec_gui_smoke_win PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/gui-test/$<CONFIG>")
endif()

option(GRAINS_BUILD_TESTS "Build DSP and state regression tests" OFF)
if(GRAINS_BUILD_TESTS)
  enable_testing()
  foreach(suite engine modulation features rhythm random master upgrade gater declick expansion state glitch_trigger musical gui_layout knob_rotation routing_polarity ui_motion)
    add_executable(grains_test_${suite} test_${suite}.cpp)
    target_include_directories(grains_test_${suite} PRIVATE "${VST3_SDK_ROOT}")
    if(MSVC)
      target_compile_options(grains_test_${suite} PRIVATE /UNDEBUG)
    else()
      target_compile_options(grains_test_${suite} PRIVATE -UNDEBUG)
    endif()
    # parameters.h uses Steinberg::Vst::ParamID only when built against the SDK;
    # standalone DSP tests get a binary-identical uint32 fallback instead.
    if(suite STREQUAL "state")
      target_compile_definitions(grains_test_${suite} PRIVATE GRAINS_HAVE_VST3_SDK)
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
      else()
        # Linux CI runner: no native editor exists, but the state test never
        # opens a view (it checks the guard returns null). Provide a stub.
        target_sources(grains_test_${suite} PRIVATE src/editor_stub.cpp)
      endif()
    endif()
    add_test(NAME ${suite} COMMAND grains_test_${suite})
  endforeach()
endif()

