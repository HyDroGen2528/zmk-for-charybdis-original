if(CONFIG_SHIELD_CHARYBDIS_DONGLE OR
   (CONFIG_SHIELD_CHARYBDIS_RIGHT AND NOT CONFIG_ZMK_SPLIT_ROLE_CENTRAL))
  zephyr_library()
  zephyr_library_sources(charybdis.c)
  zephyr_library_include_directories(${APPLICATION_SOURCE_DIR}/include)
endif()
