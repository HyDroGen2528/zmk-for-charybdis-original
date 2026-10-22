# Nano35 DYA compatibility patches

These patches apply only to the revisions pinned in `config/west.yml`:

- [PMW3610 RPC driver, 5c34ea0](https://github.com/cormoran/zmk-driver-pmw3610-with-custom-studio-rpc/tree/5c34ea0eec246a1c986111417cd779b53144629a), MIT.
- [Watchdog, 84ad14c](https://github.com/cormoran/zmk-feature-watchdog/tree/84ad14c6c3645dc40b23f45d08ab45418655f912), MIT.

Upstream copyright and license notices remain in the generated source copies.
`config/CMakeLists.txt` copies each selected source into the build directory,
applies its exact zero-context patch with Git, then compiles that copy instead of the original.
Dependency checkouts remain unmodified. A failed patch stops configuration.
No separate `patch` utility is required; the CI image already needs Git for West.

The sensor patch preserves Nano35's shared MOSI/MISO wiring using separate
command and receive transfers under manual CS, including frame-read segments.
It also serializes motion reads with Studio operations and makes force-awake
reversible. Keep the matching right overlay's `disable-burst-read` setting for
ordinary motion reports.

The relay patches send targeted requests through the pinned ZMK core's selected
transport using `nano35_relay_send`; only PMW3610 discovery broadcasts. Studio
source N maps to transport slot N-1, not to a hard-coded left/right identity.
Watchdog incident responses are queued so a pending chunk is not overwritten.

When updating dependencies, review and refresh these patches against the new
sources, build all four matrix targets, and verify hardware sensor access,
both peripheral sources, frame capture/recovery, and sleep/wake behavior.
