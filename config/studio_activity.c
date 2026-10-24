/* SPDX-License-Identifier: MIT */
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/util.h>
#include <zmk/activity.h>
#include <zmk/events/activity_settings_changed.h>
#include <zmk/split/central.h>
#include <zmk/split/transport/central.h>
#include <zmk/workqueue.h>
#include <nano35/activity.h>
#include <nano35/relay.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

/* A complete snapshot distinguishes an explicitly saved global policy from the
 * existing, independently saved activity/s settings. No marker means no sync. */
struct activity_policy {
    uint32_t version;
    uint32_t idle_ms;
    uint32_t sleep_ms;
};

#define POLICY_VERSION 1
#define PERIPHERAL_MASK BIT_MASK(ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT)
#define SYNC_RETRY_MS 1000
#define SYNC_WAIT_MS 2000

BUILD_ASSERT(ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT < 32);

extern const struct zmk_split_transport_central *active_transport;

static K_MUTEX_DEFINE(policy_lock);
static K_SEM_DEFINE(sync_done, 0, 1);
static struct activity_policy policy;
static bool policy_enabled;
static uint32_t pending;
static uint32_t apply_pending;
static uint32_t requested;
static uint8_t request_ids[ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT];
static uint8_t next_request_id;
static uint32_t generation;

static void sync_work_handler(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(sync_work, sync_work_handler);

static void schedule_sync(k_timeout_t delay) {
    k_work_reschedule_for_queue(zmk_workqueue_lowprio_work_q(), &sync_work, delay);
}

static int apply_policy(const struct activity_policy *desired) {
    if (!IS_ENABLED(CONFIG_ZMK_SLEEP)) {
        return -ENOTSUP;
    }
    if (!zmk_activity_set_idle_ms(desired->idle_ms) ||
        !zmk_activity_set_sleep_ms(desired->sleep_ms)) {
        return -EIO;
    }
    return 0;
}

static uint32_t connected_mask(void) {
    const struct zmk_split_transport_central *transport = active_transport;
    if (!transport || !transport->api || !transport->api->get_available_source_ids) {
        return 0;
    }
    uint8_t sources[ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT];
    int count = transport->api->get_available_source_ids(sources);
    if (count < 0 || (size_t)count > ARRAY_SIZE(sources)) {
        return 0;
    }
    uint32_t mask = 0;
    for (int i = 0; i < count; i++) {
        if (sources[i] < ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT) {
            mask |= BIT(sources[i]);
        }
    }
    return mask;
}

static void sync_work_handler(struct k_work *work) {
    ARG_UNUSED(work);
    uint32_t connected = connected_mask();

    for (uint8_t slot = 0; slot < ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT; slot++) {
        uint32_t bit = BIT(slot);
        struct activity_policy desired;
        struct zmk_activity_settings_request query;
        bool apply;
        uint32_t epoch;

        k_mutex_lock(&policy_lock, K_FOREVER);
        if (!policy_enabled || !(pending & connected & bit)) {
            k_mutex_unlock(&policy_lock);
            continue;
        }
        desired = policy;
        epoch = generation;
        apply = (apply_pending & bit) != 0;
        query = (struct zmk_activity_settings_request){.request_id = ++next_request_id};
        request_ids[slot] = query.request_id;
        requested |= bit;
        k_mutex_unlock(&policy_lock);

        int err = 0;
        if (apply) {
            const struct zmk_activity_settings_changed changed = {
                .idle_ms = desired.idle_ms,
                .sleep_ms = desired.sleep_ms,
                .source = ZMK_RELAY_EVENT_SOURCE_SELF,
            };
            err = nano35_relay_send(slot + 1, "as", &changed, sizeof(changed));
        }
        if (!err) {
            err = nano35_relay_send(slot + 1, "srq", &query, sizeof(query));
        }
        if (err) {
            k_mutex_lock(&policy_lock, K_FOREVER);
            if (generation == epoch && request_ids[slot] == query.request_id) {
                requested &= ~bit;
            }
            k_mutex_unlock(&policy_lock);
            LOG_DBG("Activity sync to source %u pending: %d", slot + 1, err);
        }
    }

    k_mutex_lock(&policy_lock, K_FOREVER);
    bool retry = policy_enabled && pending != 0;
    k_mutex_unlock(&policy_lock);
    if (retry) {
        schedule_sync(K_MSEC(SYNC_RETRY_MS));
    }
}

void nano35_activity_sync_report(const struct zmk_activity_settings_report *report) {
    if (!report || report->source == 0 ||
        report->source > ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT) {
        return;
    }
    uint8_t slot = report->source - 1;
    uint32_t bit = BIT(slot);
    bool retry = false;

    k_mutex_lock(&policy_lock, K_FOREVER);
    if (policy_enabled && (pending & requested & bit) &&
        request_ids[slot] == report->request_id) {
        if (report->idle_ms == policy.idle_ms && report->sleep_ms == policy.sleep_ms) {
            pending &= ~bit;
            apply_pending &= ~bit;
            requested &= ~bit;
            if (!pending) {
                k_sem_give(&sync_done);
            }
        } else {
            apply_pending |= bit;
            requested &= ~bit;
            retry = true;
        }
    }
    k_mutex_unlock(&policy_lock);
    if (retry) {
        schedule_sync(K_NO_WAIT);
    }
}

int nano35_activity_sync_set(uint32_t idle_ms, uint32_t sleep_ms) {
    /* activity.c measures elapsed time with signed 32-bit millisecond values. */
    if (idle_ms > INT32_MAX || sleep_ms > INT32_MAX) {
        return -ERANGE;
    }
    const struct activity_policy desired = {
        .version = POLICY_VERSION, .idle_ms = idle_ms, .sleep_ms = sleep_ms,
    };
    int err = apply_policy(&desired);
    if (err) {
        return err;
    }
    /* The policy snapshot is authoritative after reboot, even if activity/s's
     * normal debounced write had not run when power was removed. */
    err = settings_save_one("nano35_activity/policy", &desired, sizeof(desired));
    if (err) {
        return err;
    }

    k_mutex_lock(&policy_lock, K_FOREVER);
    policy = desired;
    policy_enabled = true;
    pending = PERIPHERAL_MASK;
    apply_pending = PERIPHERAL_MASK;
    requested = 0;
    generation++;
    k_sem_reset(&sync_done);
    k_mutex_unlock(&policy_lock);
    schedule_sync(K_NO_WAIT);

    err = k_sem_take(&sync_done, K_MSEC(SYNC_WAIT_MS));
    k_mutex_lock(&policy_lock, K_FOREVER);
    bool complete = pending == 0;
    k_mutex_unlock(&policy_lock);
    if (err || !complete) {
        LOG_WRN("Activity policy saved; peripheral readback still pending");
        return -ETIMEDOUT;
    }
    return 0;
}

static int policy_load(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg) {
    if (strcmp(name, "policy") != 0) {
        return -ENOENT;
    }
    struct activity_policy loaded;
    if (len != sizeof(loaded)) {
        return -EINVAL;
    }
    int rc = read_cb(cb_arg, &loaded, sizeof(loaded));
    if (rc < 0) {
        return rc;
    }
    if (rc != sizeof(loaded) || loaded.version != POLICY_VERSION ||
        loaded.idle_ms > INT32_MAX || loaded.sleep_ms > INT32_MAX) {
        return -EINVAL;
    }
    k_mutex_lock(&policy_lock, K_FOREVER);
    policy = loaded;
    policy_enabled = true;
    k_mutex_unlock(&policy_lock);
    return 0;
}

static int policy_commit(void) {
    k_mutex_lock(&policy_lock, K_FOREVER);
    bool enabled = policy_enabled;
    struct activity_policy desired = policy;
    if (enabled) {
        pending = PERIPHERAL_MASK;
        apply_pending = 0; /* Read back first; avoid rewriting matching halves. */
        requested = 0;
        generation++;
    }
    k_mutex_unlock(&policy_lock);
    if (enabled) {
        int err = apply_policy(&desired);
        if (err) {
            return err;
        }
        schedule_sync(K_SECONDS(1));
    }
    return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(nano35_activity, "nano35_activity", NULL, policy_load,
                               policy_commit, NULL);

static void connection_changed(void) {
    k_mutex_lock(&policy_lock, K_FOREVER);
    bool enabled = policy_enabled;
    if (enabled) {
        pending = PERIPHERAL_MASK;
        requested = 0;
        generation++;
        k_sem_reset(&sync_done);
        /* Preserve explicit writes that are not yet confirmed. Other sources
         * are queried before applying, so reconnects do not cause flash churn. */
    }
    k_mutex_unlock(&policy_lock);
    if (enabled) {
        schedule_sync(K_MSEC(250));
    }
}

static void connected(struct bt_conn *conn, uint8_t err) {
    ARG_UNUSED(conn);
    if (!err) {
        connection_changed();
    }
}

static void disconnected(struct bt_conn *conn, uint8_t reason) {
    ARG_UNUSED(conn);
    ARG_UNUSED(reason);
    connection_changed();
}

BT_CONN_CB_DEFINE(nano35_activity_connection) = {
    .connected = connected,
    .disconnected = disconnected,
};
