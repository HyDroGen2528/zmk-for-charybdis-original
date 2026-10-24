/* SPDX-License-Identifier: MIT */
#pragma once

#include <stddef.h>
#include <stdint.h>

/* Studio sources: 1..N address a connected slot. UINT32_MAX is reserved for
 * explicit all-source settings operations and PMW device discovery. */
int nano35_relay_send(uint32_t source, const char *event_type, const void *data, size_t size);

/* Snapshot connected Studio sources as BIT(source); bit zero is never set. */
int nano35_relay_connected_sources(uint32_t *mask);
