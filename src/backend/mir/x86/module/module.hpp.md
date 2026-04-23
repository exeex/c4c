# `module/module.hpp` contract

## Legacy Evidence

The old tree did not keep module orchestration behind a small dedicated header;
that absence is part of the reason orchestration leaked into utility surfaces.

## Design Contract

This header is the single public header for `module/`.

Owned inputs:

- `PreparedBirModule`
- module-local data emission helpers

Owned outputs:

- whole-module x86 assembly text

Invariants:

- module emission is the owner of top-level sequencing, not per-instruction
  lowering internals
- callers should treat the returned text as staged assembly, not as proof that
  assembler or linker ownership already exists

Must not own:

- public backend entry routing
- route-debug presentation
