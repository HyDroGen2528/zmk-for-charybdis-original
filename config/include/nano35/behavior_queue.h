/* SPDX-License-Identifier: MIT */
#pragma once

#include <stdint.h>
#include <zmk/behavior.h>

/* Queue a keypress/release as one queue entry so a full queue cannot orphan it. */
int nano35_behavior_queue_add_tap(const struct zmk_behavior_binding_event *event,
                                  const struct zmk_behavior_binding binding, uint32_t duration_ms);
