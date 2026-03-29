# Backend Port Roadmap

## Status

This file is the umbrella roadmap for the native backend effort.

Execution should usually activate one of the narrower backend ideas in `ideas/open/`.

## Goal

Port `ref/claudes-c-compiler/src/backend/` to `src/backend/` in C++ and make it consume the existing LIR in `src/codegen/lir/`.

Targets remain:

- AArch64
- x86-64
- rv64

Linux remains the execution target for produced binaries.

## Default Strategy

Default strategy: perform a mechanical C++ port of `ref/claudes-c-compiler/src/backend/` with minimal architectural deviation.

That means:

- prefer matching the ref backend's file structure, phase structure, and mechanism boundaries
- avoid redesigning shared abstractions up front
- avoid mixing cleanup with behavior changes
- treat the LIR attachment layer as the main intentional divergence from ref

## Current Open Child Ideas

- `16_backend_x86_regalloc_peephole_enablement_plan.md`
- `17_backend_x86_local_memory_addressing_plan.md`
- `18_backend_x86_global_addressing_plan.md`
- `19_backend_x86_extern_global_array_addressing_plan.md`
- `20_backend_x86_global_char_pointer_diff_plan.md`
- `21_backend_x86_global_int_pointer_diff_plan.md`
- `22_backend_x86_global_int_pointer_roundtrip_plan.md`
- `24_backend_builtin_linker_x86_plan.md`

## Shared Guardrails

- Do not change existing LIR data structures just to make the next backend slice easier.
- Prefer an adapter or shim over an early LIR redesign.
- Keep target-specific code isolated by architecture.
- Prefer external toolchain fallback over partial built-in encoding/linking until the bounded backend-owned slice is explicit.
- Keep patches mechanical where possible so ref/backend diffs stay reviewable.
- One mechanism family per patch.
