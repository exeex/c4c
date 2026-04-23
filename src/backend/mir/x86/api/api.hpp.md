# `api/api.hpp` contract

## Legacy Evidence

There was no clean legacy equivalent for this seam. The old tree exposed x86
entry behavior through broad compatibility surfaces instead of one honest API
owner.

## Design Contract

This header is the only public x86 emission entry surface that shared backend
code should include directly.

Owned inputs:

- backend-owned BIR modules
- frontend-owned LIR modules after shared backend routing
- prepared BIR modules for target-local handoff
- target profile and output-path metadata for staging

Owned outputs:

- staged x86 assembly text
- staged assemble result records

Invariants:

- `emit_prepared_module(...)` is the canonical target-local handoff
- `emit_module(...)` overloads may perform preparation but must not own module
  emission internals
- `assemble_module(...)` may stage text before assembler ownership exists, but
  it must not fake object emission

Must not own:

- module text assembly details
- route-debug logic
- ABI naming policy beyond delegating to owners
