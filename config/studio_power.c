/* SPDX-License-Identifier: MIT */
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <cormoran/zmk/custom_settings.h>
#include <zmk/event_manager.h>
#include <zmk/events/activity_state_changed.h>
#include <zmk/activity.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

/* The boot GPIO hog supplies the safe, off state until custom settings have loaded. */
static const struct gpio_dt_spec vcc =
    GPIO_DT_SPEC_GET_BY_IDX(DT_PATH(ext_power), control_gpios, 0);

/* A settings-only extension uses the registry's already registered RPC namespace. */
ZMK_CUSTOM_SETTING_DEFINE(nano35_left_vcc, "cormoran_custom_settings", "nano35_left_vcc_enabled",
    ZMK_CUSTOM_SETTING_VALUE_TYPE_BOOL, ZMK_CUSTOM_SETTING_VALUE_BOOL(false),
    ZMK_CUSTOM_SETTING_CONFIDENTIALITY_RPC_PUBLIC,
    ZMK_CUSTOM_SETTING_PERMISSION_UNSECURE, ZMK_CUSTOM_SETTING_PERMISSION_SECURE,
    ZMK_CUSTOM_SETTING_NO_CONSTRAINT);

static int apply_vcc(bool sleeping) {
    struct zmk_custom_setting_value value;
    int err = zmk_custom_setting_read(&nano35_left_vcc, &value);
    if (err < 0) {
        return err;
    }
    return gpio_pin_set_dt(&vcc, !sleeping && value.bool_value);
}

static int power_listener(const zmk_event_t *eh) {
    const struct zmk_custom_setting_changed *changed = as_zmk_custom_setting_changed(eh);
    const struct zmk_activity_state_changed *activity = as_zmk_activity_state_changed(eh);
    if (as_zmk_custom_settings_initialized(eh) ||
        (changed && changed->setting == &nano35_left_vcc) || activity) {
        int err = apply_vcc(zmk_activity_get_state() == ZMK_ACTIVITY_SLEEP);
        if (err < 0) {
            LOG_ERR("Failed to apply left VCC setting: %d", err);
        }
    }
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(nano35_power, power_listener);
ZMK_SUBSCRIPTION(nano35_power, zmk_custom_setting_changed);
ZMK_SUBSCRIPTION(nano35_power, zmk_custom_settings_initialized);
ZMK_SUBSCRIPTION(nano35_power, zmk_activity_state_changed);
