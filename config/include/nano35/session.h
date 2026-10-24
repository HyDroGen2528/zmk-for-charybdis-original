/* SPDX-License-Identifier: MIT */
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <cormoran/pmw3610/pmw3610.pb.h>

void nano35_frame_session_dispatch(uint32_t source, const cormoran_pmw3610_Request *request,
                                   cormoran_pmw3610_Response *response);
void nano35_frame_session_result(uint32_t source, uint32_t request_id, bool success,
                                 bool streaming);
