# `lowering/lowering.hpp` contract

## Design Contract

This header is the single public header for `x86/lowering/`.

Owned outputs:

- narrow summary strings for each lowering family
- small frame and memory formatting helpers

Must not own:

- whole-module routing
- prepared-query archaeology
- public backend entry routing
