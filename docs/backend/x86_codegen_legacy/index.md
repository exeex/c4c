# X86 Codegen Legacy Extraction Index

This directory is the stage-1 Phoenix extraction set for
`src/backend/mir/x86/codegen/`. The artifacts treat the live subsystem as
evidence, not as the target design.

## Boundary

In-scope legacy sources:

- `src/backend/mir/x86/codegen/*.cpp`
- `src/backend/mir/x86/codegen/x86_codegen.hpp`

Extraction rules applied here:

- one markdown companion per in-scope legacy source file
- compressed ownership and contract summaries instead of source dumps
- short fenced `cpp` blocks only for representative surfaces
- explicit special-case labels: `core lowering`, `optional fast path`,
  `legacy compatibility`, `overfit to reject`

## Dependency Direction

Observed subsystem flow:

1. `emit.cpp` is the public front door and normalizes BIR/LIR entry into prepared-module handoff.
2. `prepared_module_emit.cpp` validates target/module state, emits data/symbol scaffolding, and dispatches into prepared renderers.
3. `prepared_param_zero_render.cpp`, `prepared_countdown_render.cpp`, and especially `prepared_local_slot_render.cpp` consume prepared metadata directly and often produce final assembly text themselves.
4. Canonical lowering files such as `calls.cpp`, `returns.cpp`, `memory.cpp`, `comparison.cpp`, and `prologue.cpp` still own reusable machine policy, but the prepared family does not consistently behave like a thin client of those seams.

## Canonical Families

Entry and shared contracts:

- [x86_codegen.hpp.md](/workspaces/c4c/docs/backend/x86_codegen_legacy/x86_codegen.hpp.md)
- [mod.cpp.md](/workspaces/c4c/docs/backend/x86_codegen_legacy/mod.cpp.md)
- [emit.cpp.md](/workspaces/c4c/docs/backend/x86_codegen_legacy/emit.cpp.md)
- [asm_emitter.cpp.md](/workspaces/c4c/docs/backend/x86_codegen_legacy/asm_emitter.cpp.md)
- [shared_call_support.cpp.md](/workspaces/c4c/docs/backend/x86_codegen_legacy/shared_call_support.cpp.md)
- [route_debug.cpp.md](/workspaces/c4c/docs/backend/x86_codegen_legacy/route_debug.cpp.md)

Core lowering buckets:

- [calls.cpp.md](/workspaces/c4c/docs/backend/x86_codegen_legacy/calls.cpp.md)
- [returns.cpp.md](/workspaces/c4c/docs/backend/x86_codegen_legacy/returns.cpp.md)
- [memory.cpp.md](/workspaces/c4c/docs/backend/x86_codegen_legacy/memory.cpp.md)
- [prologue.cpp.md](/workspaces/c4c/docs/backend/x86_codegen_legacy/prologue.cpp.md)
- [variadic.cpp.md](/workspaces/c4c/docs/backend/x86_codegen_legacy/variadic.cpp.md)
- [alu.cpp.md](/workspaces/c4c/docs/backend/x86_codegen_legacy/alu.cpp.md)
- [atomics.cpp.md](/workspaces/c4c/docs/backend/x86_codegen_legacy/atomics.cpp.md)
- [cast_ops.cpp.md](/workspaces/c4c/docs/backend/x86_codegen_legacy/cast_ops.cpp.md)
- [comparison.cpp.md](/workspaces/c4c/docs/backend/x86_codegen_legacy/comparison.cpp.md)
- [f128.cpp.md](/workspaces/c4c/docs/backend/x86_codegen_legacy/f128.cpp.md)
- [float_ops.cpp.md](/workspaces/c4c/docs/backend/x86_codegen_legacy/float_ops.cpp.md)
- [globals.cpp.md](/workspaces/c4c/docs/backend/x86_codegen_legacy/globals.cpp.md)
- [i128_ops.cpp.md](/workspaces/c4c/docs/backend/x86_codegen_legacy/i128_ops.cpp.md)
- [inline_asm.cpp.md](/workspaces/c4c/docs/backend/x86_codegen_legacy/inline_asm.cpp.md)
- [intrinsics.cpp.md](/workspaces/c4c/docs/backend/x86_codegen_legacy/intrinsics.cpp.md)

Prepared-route divergence:

- [prepared_module_emit.cpp.md](/workspaces/c4c/docs/backend/x86_codegen_legacy/prepared_module_emit.cpp.md)
- [prepared_param_zero_render.cpp.md](/workspaces/c4c/docs/backend/x86_codegen_legacy/prepared_param_zero_render.cpp.md)
- [prepared_countdown_render.cpp.md](/workspaces/c4c/docs/backend/x86_codegen_legacy/prepared_countdown_render.cpp.md)
- [prepared_local_slot_render.cpp.md](/workspaces/c4c/docs/backend/x86_codegen_legacy/prepared_local_slot_render.cpp.md)

## Responsibility Pressure

Rebuild pressure made explicit by this extraction set:

- `x86_codegen.hpp` mixes canonical helpers, prepared ABI helpers, and top-level dispatch declarations in one shared header.
- `prepared_module_emit.cpp` combines route choice with string/global/data emission and prepared dispatcher ownership.
- `prepared_local_slot_render.cpp` reopens local-slot, addressing, byval, and call-lane policy that already has canonical homes elsewhere.
- `prepared_param_zero_render.cpp` and `prepared_countdown_render.cpp` keep predicate and branch shaping in prepared-specialized files instead of one shared canonical owner.

## Coverage Check

This directory now contains one companion markdown file for every in-scope
legacy source named by idea 78:

- `alu.cpp`
- `asm_emitter.cpp`
- `atomics.cpp`
- `calls.cpp`
- `cast_ops.cpp`
- `comparison.cpp`
- `emit.cpp`
- `f128.cpp`
- `float_ops.cpp`
- `globals.cpp`
- `i128_ops.cpp`
- `inline_asm.cpp`
- `intrinsics.cpp`
- `memory.cpp`
- `mod.cpp`
- `prepared_countdown_render.cpp`
- `prepared_local_slot_render.cpp`
- `prepared_module_emit.cpp`
- `prepared_param_zero_render.cpp`
- `prologue.cpp`
- `returns.cpp`
- `route_debug.cpp`
- `shared_call_support.cpp`
- `variadic.cpp`
- `x86_codegen.hpp`

Stage-2 review should treat this full set, not the older single-file
`docs/backend/x86_codegen_subsystem.md`, as the canonical Phoenix extraction
artifact package.
