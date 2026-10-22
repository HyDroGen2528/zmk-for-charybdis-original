/* SPDX-License-Identifier: MIT */
#pragma once

#include <stddef.h>
#include <stdint.h>

/* Studio sources: 1..N address a connected slot; UINT32_MAX means discovery broadcast. */
int nano35_relay_send(uint32_t source, const char *event_type, const void *data, size_t size);
