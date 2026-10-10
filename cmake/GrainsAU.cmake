# AUv2 wrapper around the same VST3 engine and Cocoa editor.
set(_au_library "${CMAKE_BINARY_DIR}/AudioUnitSDK/Release/libAudioUnitSDK.a")
add_custom_command(OUTPUT "${_au_library}"
  COMMAND xcodebuild -project "${SMTG_AUDIOUNIT_SDK_PATH}/AudioUnitSDK.xcodeproj"
    -target AudioUnitSDK -configuration Release build
    "SYMROOT=${CMAKE_BINARY_DIR}/AudioUnitSDK" "ARCHS=x86_64 arm64"
    ONLY_ACTIVE_ARCH=NO "MACOSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}" CODE_SIGNING_ALLOWED=NO
    "WARNING_CFLAGS=$(inherited) -Werror=unguarded-availability -Werror=unguarded-availability-new"
  VERBATIM)
add_custom_target(GrainsAppleAudioUnitSDK DEPENDS "${_au_library}")
set(_wrapper "${public_sdk_SOURCE_DIR}/source/vst/auwrapper")
add_library(GrainsDosageAU MODULE
  "${_wrapper}/aucocoaview.mm" "${_wrapper}/auwrapper.mm" "${_wrapper}/NSDataIBStream.mm")
add_dependencies(GrainsDosageAU GrainsDosage GrainsAppleAudioUnitSDK)
target_compile_features(GrainsDosageAU PRIVATE cxx_std_17)
set_target_properties(GrainsDosageAU PROPERTIES OBJCXX_STANDARD 17 OBJCXX_STANDARD_REQUIRED ON)
target_compile_definitions(GrainsDosageAU PRIVATE SMTG_AUWRAPPER_USES_AUSDK
  SMTG_AUCocoaUIBase_CLASS_NAME=GrainsDosageAUCocoaUI CA_USE_AUDIO_PLUGIN_ONLY=0)
target_include_directories(GrainsDosageAU PRIVATE "${SMTG_AUDIOUNIT_SDK_PATH}/include")
target_link_libraries(GrainsDosageAU PRIVATE sdk_hosting "${_au_library}"
  "-framework AudioUnit" "-framework CoreMIDI" "-framework AudioToolbox"
  "-framework CoreFoundation" "-framework Carbon" "-framework Cocoa" "-framework CoreAudio")
smtg_target_set_bundle(GrainsDosageAU INFOPLIST "${CMAKE_CURRENT_SOURCE_DIR}/src/au_info.plist.in" EXTENSION component)
set_target_properties(GrainsDosageAU PROPERTIES OUTPUT_NAME GrainsDosage
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/AU"
  XCODE_ATTRIBUTE_PRODUCT_NAME GrainsDosage
  XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER com.danielmelegari.aztecgrains.au)
# Copy the real bundle, not the SDK's development-only absolute symlink.
add_custom_command(TARGET GrainsDosageAU POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/AU/$<CONFIG>/GrainsDosage.component/Contents/Resources"
  COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_BINARY_DIR}/VST3/$<CONFIG>/GrainsDosage.vst3"
    "${CMAKE_BINARY_DIR}/AU/$<CONFIG>/GrainsDosage.component/Contents/Resources/plugin.vst3"
  VERBATIM)
