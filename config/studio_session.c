/* SPDX-License-Identifier: MIT */
#include <errno.h>
#include <pb_encode.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zmk/event_manager.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/split/central.h>
#include <zmk/studio/core.h>
#include <zmk/usb.h>
#include <zmk/workqueue.h>
#include <cormoran/pmw3610/pmw3610.pb.h>
#include <cormoran/pmw3610/pmw3610_relay.h>
#include <nano35/session.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define STOP_RETRY_MS 3000
#define SESSION_FAILSAFE_MS 66000

struct frame_session {
    uint32_t device;
    uint32_t start_request_id;
    uint32_t stop_request_id;
    int64_t started;
    int64_t stop_sent_at;
    bool active;
    bool dtr_seen;
    bool stopping;
};

static struct frame_session sessions[ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT];
K_MUTEX_DEFINE(session_lock);
static void session_work_handler(struct k_work *work);
K_WORK_DELAYABLE_DEFINE(session_work, session_work_handler);

static void schedule_session_check(void) {
    k_work_reschedule_for_queue(zmk_workqueue_lowprio_work_q(), &session_work, K_NO_WAIT);
}

void nano35_frame_session_dispatch(uint32_t source, const cormoran_pmw3610_Request *request,
                                   cormoran_pmw3610_Response *response) {
    const bool is_stream = request->which_request_type ==
                           cormoran_pmw3610_Request_set_frame_stream_tag;
    if (!is_stream || source == 0 || source > ARRAY_SIZE(sessions)) {
        pmw3610_relay_dispatch_request(source, request, response);
        return;
    }

    /* Serialize client starts/stops with disconnect cleanup, preserving relay order. */
    k_mutex_lock(&session_lock, K_FOREVER);
    pmw3610_relay_dispatch_request(source, request, response);
    if (response->which_response_type == cormoran_pmw3610_Response_deferred_tag) {
        struct frame_session *session = &sessions[source - 1];
        if (request->request_type.set_frame_stream.enable) {
            *session = (struct frame_session){
                .device = request->request_type.set_frame_stream.device_index,
                .start_request_id = response->response_type.deferred.request_id,
                .started = k_uptime_get(),
                .active = true,
            };
        } else if (session->active) {
            session->device = request->request_type.set_frame_stream.device_index;
            session->stop_request_id = response->response_type.deferred.request_id;
            session->stop_sent_at = k_uptime_get();
            session->stopping = true;
        }
    }
    k_mutex_unlock(&session_lock);
    schedule_session_check();
}

void nano35_frame_session_result(uint32_t source, uint32_t request_id, bool success,
                                 bool streaming) {
    if (source == 0 || source > ARRAY_SIZE(sessions)) {
        return;
    }
    k_mutex_lock(&session_lock, K_FOREVER);
    struct frame_session *session = &sessions[source - 1];
    if (!session->active) {
        k_mutex_unlock(&session_lock);
        return;
    }
    if (request_id == session->start_request_id) {
        if (!success || !streaming) {
            session->active = false;
        }
    } else if (request_id == session->stop_request_id && session->stop_request_id != 0) {
        if (success && !streaming) {
            session->active = false;
        } else {
            /* Retry a rejected stop; an old response can never clear a newer start. */
            session->stop_request_id = 0;
            session->stopping = true;
        }
    }
    k_mutex_unlock(&session_lock);
    schedule_session_check();
}

static int queue_stop_locked(size_t index, struct frame_session *session) {
    cormoran_pmw3610_Request request = cormoran_pmw3610_Request_init_zero;
    request.which_request_type = cormoran_pmw3610_Request_set_frame_stream_tag;
    request.request_type.set_frame_stream.device_index = session->device;
    request.request_type.set_frame_stream.source = index + 1;
    request.request_type.set_frame_stream.enable = false;
    cormoran_pmw3610_Response response = cormoran_pmw3610_Response_init_zero;
    pmw3610_relay_dispatch_request(index + 1, &request, &response);
    if (response.which_response_type != cormoran_pmw3610_Response_deferred_tag) {
        return -EIO;
    }
    session->stop_request_id = response.response_type.deferred.request_id;
    session->stop_sent_at = k_uptime_get();
    return 0;
}

static void session_work_handler(struct k_work *work) {
    ARG_UNUSED(work);
    uint32_t dtr = 0;
    int line_result = uart_line_ctrl_get(DEVICE_DT_GET(DT_CHOSEN(zmk_studio_rpc_uart)),
                                         UART_LINE_CTRL_DTR, &dtr);
    bool host_gone = zmk_usb_get_conn_state() != ZMK_USB_CONN_HID ||
                     zmk_studio_core_get_lock_state() == ZMK_STUDIO_CORE_LOCK_STATE_LOCKED;
    bool pending = false;
    for (size_t i = 0; i < ARRAY_SIZE(sessions); i++) {
        k_mutex_lock(&session_lock, K_FOREVER);
        struct frame_session *session = &sessions[i];
        if (!session->active) {
            k_mutex_unlock(&session_lock);
            continue;
        }
        if (k_uptime_get() - session->started >= SESSION_FAILSAFE_MS) {
            session->active = false;
            k_mutex_unlock(&session_lock);
            continue;
        }
        if (line_result == 0 && dtr != 0) {
            session->dtr_seen = true;
        }
        session->stopping |= host_gone ||
                             (line_result == 0 && session->dtr_seen && dtr == 0);
        if (session->stopping &&
            (session->stop_request_id == 0 ||
             k_uptime_get() - session->stop_sent_at >= STOP_RETRY_MS)) {
            session->stop_request_id = 0;
            int err = queue_stop_locked(i, session);
            if (err < 0) {
                LOG_DBG("Frame session stop pending for source %u: %d", (unsigned int)i + 1,
                        err);
            }
        }
        pending = session->active;
        k_mutex_unlock(&session_lock);
    }
    if (pending) {
        k_work_reschedule_for_queue(zmk_workqueue_lowprio_work_q(), &session_work, K_MSEC(500));
    }
}

static int session_listener(const zmk_event_t *eh) {
    ARG_UNUSED(eh);
    schedule_session_check();
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(nano35_session, session_listener);
ZMK_SUBSCRIPTION(nano35_session, zmk_usb_conn_state_changed);
ZMK_SUBSCRIPTION(nano35_session, zmk_studio_core_lock_state_changed);
