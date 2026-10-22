# Legacy PMW3610 driver

Source: https://github.com/DoctorWangWang/zmk-pmw3610-driver/tree/1d9c2c68ca76012e1b1e5f6ef02fa5eadc4ca399

The original driver and headers are retained for this keyboard's shared
MOSI/MISO wiring and existing CPI/orientation defaults. Source copyright and
SPDX notices are preserved. The devicetree compatible and DT_DRV_COMPAT were
renamed to `zmk,pmw3610-legacy` / `zmk_pmw3610_legacy` to avoid colliding with
Zephyr 4.1's built-in `pixart,pmw3610` binding. Sensor I/O logic is unchanged.

The adapter in ../../charybdis.c keeps the peripheral in raw-motion mode.
Layer-dependent processing and DYA runtime tuning happen on the dongle.
This driver does not expose DYA's sensor-register, hardware-CPI or image RPCs.
