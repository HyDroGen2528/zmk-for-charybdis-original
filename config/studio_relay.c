/* SPDX-License-Identifier: MIT */
#include <errno.h>
#include <string.h>
#include <zephyr/sys/util.h>
#include <zmk/split/central.h>
#include <zmk/split/transport/central.h>
#include <nano35/relay.h>

/* Exported by the pinned ZMK app/src/split/central.c. Use its selected transport. */
extern const struct zmk_split_transport_central *active_transport;

int nano35_relay_connected_sources(uint32_t *mask) {
    if (!mask) {
        return -EINVAL;
    }
    *mask = 0;
    const struct zmk_split_transport_central *transport = active_transport;
    if (!transport || !transport->api || !transport->api->get_available_source_ids) {
        return -ENODEV;
    }
    uint8_t connected[ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT];
    int count = transport->api->get_available_source_ids(connected);
    if (count < 0) {
        return count;
    }
    if ((size_t)count > ARRAY_SIZE(connected)) {
        return -EOVERFLOW;
    }
    for (int i = 0; i < count; i++) {
        if (connected[i] >= ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT || connected[i] >= 31) {
            *mask = 0;
            return -EOVERFLOW;
        }
        *mask |= UINT32_C(1) << (connected[i] + 1);
    }
    return 0;
}

int nano35_relay_send(uint32_t source, const char *event_type, const void *data, size_t size) {
    struct zmk_split_relay_event_payload payload = {0};
    if (!event_type || !data || size > sizeof(payload.event_data) || size > UINT8_MAX) {
        return -EINVAL;
    }
    size_t name_len = strlen(event_type);
    if (name_len == 0 || name_len >= sizeof(payload.event_type)) {
        return -EINVAL;
    }
    payload.header.event_data_size = size;
    payload.header.event_type_size = name_len;
    memcpy(payload.event_type, event_type, name_len + 1);
    memcpy(payload.event_data, data, size);

    if (source == UINT32_MAX) {
        if (strcmp(event_type, "pmq") != 0 && strcmp(event_type, "csr") != 0 &&
            strcmp(event_type, "SEq") != 0) {
            return -EINVAL;
        }
        return zmk_split_central_send_relay_event(&payload);
    }
    if (source == 0 || source > ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT) {
        return -EINVAL;
    }
    const struct zmk_split_transport_central *transport = active_transport;
    if (!transport || !transport->api || !transport->api->send_command ||
        !transport->api->get_available_source_ids) {
        return -ENODEV;
    }
    uint8_t connected[ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT];
    int count = transport->api->get_available_source_ids(connected);
    if (count < 0) {
        return count;
    }
    if ((size_t)count > ARRAY_SIZE(connected)) {
        return -EOVERFLOW;
    }
    bool found = false;
    for (int i = 0; i < count; i++) {
        found |= connected[i] == source - 1;
    }
    if (!found) {
        return -ENOTCONN;
    }
    struct zmk_split_transport_central_command command = {
        .type = ZMK_SPLIT_TRANSPORT_CENTRAL_CMD_TYPE_RELAY_EVENT,
        .data.relay_event = payload,
    };
    return transport->api->send_command(source - 1, command);
}
