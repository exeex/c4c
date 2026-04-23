# `core/core.hpp` contract

## Design Contract

This header is the single public header for `x86/core/`.

Owned outputs:

- `core::Text` for staged assembly text accumulation
- tiny symbol-query helpers for emitted text
- small shared result carriers used by public x86 entrypoints

Must not own:

- lowering policy
- target admission logic
- module orchestration
