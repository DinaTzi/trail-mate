# ui_legacy_adapters

Retired legacy adapter module.

This module name remains only as a documented legacy boundary. Runtime-owned UI
adapters now live in their runtime modules:

- chat delivery: `modules/ui_chat_runtime`
- key verification: `modules/ui_key_verification_runtime`
- map overlay: `modules/ui_map_runtime`

Must not contain:

- forwarding headers for runtime-owned adapters
- source/sink implementations
- alias smoke tests
- product behavior
- renderer widgets
- app composition roots
- build entrypoints
- board facts

Active build manifests must not expose `modules/ui_legacy_adapters/include`.
