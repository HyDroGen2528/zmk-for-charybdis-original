#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>

#if defined(CONFIG_SHIELD_CHARYBDIS_DONGLE)

#include <zephyr/drivers/led_strip.h>

// ZMK's global RGB behavior keeps state centrally before forwarding commands.
// The dongle has no LEDs; actual output happens on the two peripherals.
static int virtual_strip_update(const struct device *dev, struct led_rgb *pixels, size_t count) {
    ARG_UNUSED(dev);
    ARG_UNUSED(pixels);
    ARG_UNUSED(count);
    return 0;
}

static const struct led_strip_driver_api virtual_strip_api = {
    .update_rgb = virtual_strip_update,
};

DEVICE_DT_DEFINE(DT_NODELABEL(led_strip), NULL, NULL, NULL, NULL, POST_KERNEL,
                 CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &virtual_strip_api);

#elif defined(CONFIG_PMW3610) && !defined(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)

#include <zmk/keymap.h>

// This driver predates split pointing support and always queries the keymap.
// Peripherals have no keymap: keep it in raw-motion mode, then process layers
// on the dongle. Pin the driver revision in west.yml alongside this adapter.
BUILD_ASSERT(DT_PROP_LEN(DT_NODELABEL(trackball), scroll_layers) == 0);
BUILD_ASSERT(DT_PROP_LEN(DT_NODELABEL(trackball), snipe_layers) == 0);
BUILD_ASSERT(DT_PROP(DT_NODELABEL(trackball), automouse_layer) < 0);

zmk_keymap_layer_index_t zmk_keymap_highest_layer_active(void) { return 0; }

#endif
