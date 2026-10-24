zephyr_include_directories(include)

# Compile patched copies in the build directory; pinned dependency checkouts stay clean.
function(nano35_patch_source original patch_name)
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/patches/${patch_name})
  find_program(NANO35_GIT git REQUIRED)
  get_filename_component(stem ${original} NAME)
  set(patched ${CMAKE_CURRENT_BINARY_DIR}/nano35_compat/${stem})
  configure_file(${original} ${patched} COPYONLY)
  execute_process(COMMAND ${NANO35_GIT} apply --unsafe-paths --unidiff-zero
                          --directory=${CMAKE_CURRENT_BINARY_DIR}/nano35_compat
                          ${CMAKE_CURRENT_SOURCE_DIR}/patches/${patch_name}
                  RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
  if(NOT result EQUAL 0)
    message(FATAL_ERROR "Nano35 compatibility patch failed: ${patch_name}\n${output}\n${error}")
  endif()
  set_source_files_properties(${original} TARGET_DIRECTORY app PROPERTIES HEADER_FILE_ONLY TRUE)
  target_sources(app PRIVATE ${patched})
endfunction()

if(CONFIG_PMW3610)
  nano35_patch_source(${ZEPHYR_ZMK_DRIVER_PMW3610_WITH_CUSTOM_STUDIO_RPC_MODULE_DIR}/src/pmw3610.c
                      pmw3610-shared-data.patch)
  target_include_directories(app PRIVATE
    ${ZEPHYR_ZMK_DRIVER_PMW3610_WITH_CUSTOM_STUDIO_RPC_MODULE_DIR}/src)
endif()

if(CONFIG_ZMK_PMW3610_PROTOBUF)
  nano35_patch_source(${ZEPHYR_ZMK_DRIVER_PMW3610_WITH_CUSTOM_STUDIO_RPC_MODULE_DIR}/src/studio/pmw3610_request_exec.c
                      pmw3610-frame-session.patch)
endif()

if(CONFIG_ZMK_PMW3610_STUDIO_RPC)
  nano35_patch_source(${ZEPHYR_ZMK_DRIVER_PMW3610_WITH_CUSTOM_STUDIO_RPC_MODULE_DIR}/src/studio/pmw3610_handler.c
                      pmw3610-host-session.patch)
endif()

if(CONFIG_ZMK_PMW3610_SPLIT_RPC_RELAY)
  nano35_patch_source(${ZEPHYR_ZMK_DRIVER_PMW3610_WITH_CUSTOM_STUDIO_RPC_MODULE_DIR}/src/split/pmw3610_relay.c
                      pmw3610-addressed-relay.patch)
endif()

if(CONFIG_ZMK_WATCHDOG_SPLIT_RELAY)
  nano35_patch_source(${ZEPHYR_ZMK_FEATURE_WATCHDOG_MODULE_DIR}/src/split/watchdog_relay.c
                      watchdog-addressed-relay.patch)
endif()

if(CONFIG_ZMK_CUSTOM_SETTINGS_PROTOBUF AND CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
  nano35_patch_source(${ZEPHYR_ZMK_FEATURE_CUSTOM_SETTINGS_MODULE_DIR}/src/studio/custom_settings_handler.c
                      custom-settings-addressed-relay.patch)
endif()

if(CONFIG_ZMK_RUNTIME_SENSOR_ROTATE)
  nano35_patch_source(${CMAKE_SOURCE_DIR}/src/behavior_queue.c
                      behavior-queue-safe-tap.patch)
  nano35_patch_source(${ZEPHYR_ZMK_BEHAVIOR_RUNTIME_SENSOR_ROTATE_MODULE_DIR}/src/behaviors/behavior_runtime_sensor_rotate.c
                      runtime-sensor-rotate-safe-tap.patch)
endif()

if(CONFIG_ZMK_DEFAULT_LAYER)
  nano35_patch_source(${ZEPHYR_ZMK_FEATURE_DEFAULT_LAYER_MODULE_DIR}/src/default_layer.c
                      default-layer-safe-base.patch)
endif()

if(CONFIG_ZMK_DEFAULT_LAYER_STUDIO_RPC)
  nano35_patch_source(${ZEPHYR_ZMK_FEATURE_DEFAULT_LAYER_MODULE_DIR}/src/studio/default_layer_handler.c
                      default-layer-safe-base-rpc.patch)
endif()

if(CONFIG_ZMK_SETTING_EXPOSE_SPLIT AND CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
  nano35_patch_source(${ZEPHYR_ZMK_FEATURE_ZEPHYR_SETTING_EXPOSE_MODULE_DIR}/src/split/setting_expose_relay.c
                      setting-expose-addressed-relay.patch)
endif()

if(CONFIG_ZMK_SETTINGS_RPC_STUDIO)
  nano35_patch_source(${ZEPHYR_ZMK_MODULE_SETTINGS_RPC_MODULE_DIR}/src/studio/settings_rpc_handler.c
                      settings-rpc-reliable-sync.patch)
endif()

if(CONFIG_SHIELD_CHARYBDIS_DONGLE)
  target_sources(app PRIVATE studio_relay.c studio_activity.c studio_session.c)
endif()

if(CONFIG_SHIELD_CHARYBDIS_LEFT)
  target_sources(app PRIVATE studio_power.c)
endif()
