# ESP Radio Runtime

Final owner for ESP radio/protocol adapter code that is platform-bound but not
an app shell responsibility.

Current migrated source:

- `meshtastic_radio_adapter.h`
- `meshtastic_radio_adapter.cpp`

Evidence:

- migrated from the removed historical ESP-IDF app implementation root
- active source ownership now lives under final platform ownership

Rules:

- do not select UX packs here
- do not own app shell startup or loop sequencing here
- do not include `removed legacy app implementation roots`
- keep protocol behavior unchanged unless a protocol-specific change is made
