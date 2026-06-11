# `modules/core_hostlink`

Home for shared hostlink protocol logic and state handling.

## Migrated now

- `hostlink_protocol.md`
- `include/hostlink/hostlink_types.h`
- `include/hostlink/hostlink_codec.h`
- `src/hostlink_codec.cpp`
- `include/hostlink/hostlink_config_codec.h`
- `src/hostlink_config_codec.cpp`
- `include/hostlink/hostlink_service_codec.h`
- `src/hostlink_service_codec.cpp`
- `include/hostlink/hostlink_session.h`
- `src/hostlink_session.cpp`
- `include/hostlink/hostlink_frame_router.h`
- `src/hostlink_frame_router.cpp`
- `include/hostlink/hostlink_app_data_codec.h`
- `src/hostlink_app_data_codec.cpp`
- `include/hostlink/hostlink_event_codec.h`
- `src/hostlink_event_codec.cpp`
- `include/hostlink/c6/c6_protocol.h`
- `include/hostlink/c6/c6_frame_codec.h`
- `include/hostlink/c6/c6_frame_codec_c.h`
- `src/c6_frame_codec.cpp`
- `src/c6_frame_codec_c.c`

These pieces are pure protocol/codec logic and do not depend on Arduino, FreeRTOS,
USB, board support, or application composition.

The `hostlink/c6/*` path is intentionally distinct from the Data Exchange
HostLink frame format. C6 HostLink is the P4-to-ESP32-C6 wireless companion
protocol defined by the companion architecture; it shares the module because it
is portable wire protocol code, not because the two transports are
interchangeable.

## Deferred for later

- hostlink runtime ownership has moved out of `src/hostlink`, but it still remains in the ESP/Arduino platform layer because it owns USB transport, FreeRTOS tasks/queues, and current runtime scheduling policy
