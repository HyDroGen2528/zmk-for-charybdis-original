/* SPDX-License-Identifier: MIT */
#pragma once

#include <stdint.h>
#include <zmk/events/activity_settings_report.h>

/* Only the Studio RPC thread may wait for synchronization; workers never wait. */
int nano35_activity_sync_set(uint32_t idle_ms, uint32_t sleep_ms);
void nano35_activity_sync_report(const struct zmk_activity_settings_report *report);
