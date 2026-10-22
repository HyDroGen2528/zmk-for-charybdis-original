#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>

#if defined(CONFIG_PMW3610) && !defined(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)

#include <zmk/keymap.h>

// This driver predates split pointing support and always queries the keymap.
// Peripherals have no keymap: keep it in raw-motion mode, then process layers
// on the dongle. The source revision is recorded in drivers/pmw3610/README.md.
BUILD_ASSERT(DT_PROP_LEN(DT_NODELABEL(trackball), scroll_layers) == 0);
BUILD_ASSERT(DT_PROP_LEN(DT_NODELABEL(trackball), snipe_layers) == 0);
BUILD_ASSERT(DT_PROP(DT_NODELABEL(trackball), automouse_layer) < 0);

zmk_keymap_layer_index_t zmk_keymap_highest_layer_active(void) { return 0; }

#endif
