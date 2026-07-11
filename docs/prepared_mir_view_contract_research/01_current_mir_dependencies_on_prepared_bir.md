# Current MIR Dependencies On Prepared BIR

This document is the Step 1 evidence baseline for the current MIR-facing
`PreparedBirModule` contract. It traces the x86 prepared-module entry path,
then inventories the live prepared field families currently consumed by MIR
targets. It intentionally separates compiled code from markdown mirrors and
legacy notes.

## Evidence Scope

Live code means compiled C++ under `src/backend/mir/**` plus the backend entry
handoff in `src/backend/backend.cpp`. Files such as
`src/backend/mir/x86/prepared_module_emit.cpp.md`,
`src/backend/mir/x86/module/module_emit.cpp.md`, and
`src/backend/mir/x86/debug/prepared_route_debug.cpp.md` are documentation or
legacy mirrors. They mention `PreparedBirModule`, but they are not current MIR
consumers.

The prepared module shape itself is declared in
`src/backend/prealloc/module.hpp:32`: it contains the semantic BIR module,
target profile, name tables, control-flow facts, value locations, stack layout,
addressing, liveness, register overrides, regalloc, frame and dynamic-stack
plans, call/publication plans, storage plans, object data, special carriers,
runtime-helper facts, completed phases, notes, and a private route.

## x86 Entry Path

The current x86 backend reaches MIR through a prepared-module handoff:

1. `src/backend/backend.cpp:1468` enters `emit_x86_bir_module_entry()`.
   It requires an x86 target, calls `prepare_semantic_bir_pipeline()`, and
   passes the resulting `PreparedBirModule` to
   `x86::api::emit_prepared_module()`.
2. `src/backend/backend.cpp:1506` follows the same path for LIR input after
   successful LIR-to-BIR lowering.
3. `prepare_semantic_bir_pipeline()` is a thin wrapper over
   `prepare::prepare_semantic_bir_module_with_options()` at
   `src/backend/backend.cpp:300`.
4. `BirPreAlloc::run()` in `src/backend/prealloc/prealloc.cpp:31` populates
   the prepared module. `publish_contract_plans()` at
   `src/backend/prealloc/prealloc.cpp:43` publishes frame, dynamic-stack,
   call, publication, variadic, storage, object-data, i128/f128, atomic,
   intrinsic, inline-asm, and runtime-helper families.
5. `src/backend/mir/x86/api/api.cpp:56` forwards the prepared module to
   `x86::module::emit()`.
6. `x86::module::emit()` starts at `src/backend/mir/x86/module/module.cpp:6452`.
   It resolves the target profile/triple, builds module data, iterates
   `module.module.functions`, and dispatches each defined function through
   supported scalar routes or the contract-first stub path.

The most important x86 dependency fan-out is `consume_plans()` in
`src/backend/mir/x86/x86.hpp:162`. It builds the per-function `ConsumedPlans`
view from frame plan, dynamic-stack plan, control flow, call plans, regalloc,
storage plan, prepared function lookups, and a Route 6 BIR call-use index.
This is already close to the desired contract shape, but it is still backed by
unrestricted `PreparedBirModule` access.

## Live x86 Dependency Families

| Field family | Current live use | Classification |
| --- | --- | --- |
| `module` | Function iteration, function bodies, BIR name tables, globals, string constants, and same-module symbol checks in `module.cpp`; target triple fallback in `abi.cpp`; data emission in `data.cpp`. | Required core for the current x86 route, but too broad as a first view. A view should expose function iteration, BIR name tables, globals/string constants, and target triple through narrower accessors. |
| `target_profile` | Target/profile resolution in `src/backend/mir/x86/abi/abi.cpp:10` and target gating in `module.cpp`. | Required core target identity. |
| `names` | Function/block/value id resolution, prepared label spelling, value-home lookups, and debug display throughout `x86.hpp`, `module.cpp`, `prepared.hpp`, and `debug.cpp`. | Required core identity and spelling service. |
| `control_flow` | `consume_plans()` and `validate_prepared_control_flow_handoff()` consume prepared blocks, branch conditions, join transfers, and parallel-copy bundles; branch-specific routes query short-circuit and compare-join facts. | Required core control-flow contract, with branch/join subfeatures. |
| `value_locations` | Queried directly or through `find_prepared_value_location_function()` for value-home lookup, formal publication planning, and local-memory/branch routes. | Required core value-location contract. |
| `stack_layout` | Frame-size adjustment, frame-slot/object lookup, local-memory addressing, and stack-backed edge-publication materialization in `module.cpp`. | Required core stack-frame contract. |
| `addressing` | Queried through `find_prepared_addressing()` for local/global memory access, address materialization, and branch/load routes. | Required core memory/addressing contract. |
| `frame_plan` | Pulled into `ConsumedPlans`; used by debug grouped-authority reporting and frame/call-boundary routes. | Required for nontrivial frame and call-boundary codegen; optional only for leaf/no-frame subsets. |
| `dynamic_stack_plan` | Pulled into `ConsumedPlans` and named in the x86 root contract as the authority for VLA/dynamic alloca handling. | Feature-specific, required when dynamic stack exists. |
| `call_plans` | Pulled into `ConsumedPlans`; `find_consumed_call_plan()` and argument/result helpers gate call emission and Route 6 agreement. | Feature-specific, required when calls are lowered. |
| `regalloc` | Pulled into `ConsumedPlans`; used by grouped authority diagnostics and prepared query home decoding. | Required for register-home codegen once values escape trivial scalar routes; diagnostic-only for grouped summaries. |
| `storage_plans` | Pulled into `ConsumedPlans`; used by prepared query home decoding and grouped storage diagnostics. | Required for value storage/codegen beyond trivial immediates. |
| prepared lookups derived from the module | `make_prepared_function_lookups()` is used by x86 `consume_prepared_function_lookups()` and edge-publication move intent lookup. | Required derived core index, but it should be exposed as a view-owned lookup rather than recomputed from unrestricted module access. |

The live x86 code does not directly consume `invariants`, `completed_phases`,
`notes`, `liveness`, `register_group_overrides`, `object_data`,
`i128_carriers`, `f128_carriers`, `atomic_operations`, `intrinsic_carriers`,
`inline_asm_carriers`, or runtime helper facts through x86 compiled C++ today.
Those may be needed for other targets or future x86 feature recovery, but they
are not part of the smallest x86-first view.

## Cross-Target Live Dependencies

The x86 path is not the only live MIR consumer of `PreparedBirModule`.
Field-family searches over compiled MIR sources show:

| Target | Live evidence | Field families |
| --- | --- | --- |
| RV64 | `emit_prepared_module()` forwards to `emit_prepared_module_text()` in `src/backend/mir/riscv/codegen/emit.cpp:62`; `prepared_module_emit.cpp:253` iterates `module.control_flow.functions` and builds prepared lookups; `prepared_function_emit.cpp:372` builds per-function facts for object emission. | `module`, `target_profile`, `names`, `control_flow`, `value_locations`, `stack_layout`, `addressing`, `frame_plan`, `call_plans`, `variadic_entry_plans`, `storage_plans`, `object_data`, `inline_asm_carriers`, `store_source_publications`, `call_argument_value_publications`, plus derived dependency/select authorities. |
| AArch64 | `compile_prepared_module()` enters `build_module()` in `src/backend/mir/aarch64/codegen/module_compile.cpp:73`; `traversal.cpp:59` builds a `FunctionLoweringContext` containing the full prepared pointer and selected per-function plan pointers; `abi.cpp:870` validates target handoff. | `module`, `target_profile`, `names`, `control_flow`, `value_locations`, `stack_layout`, `addressing`, `regalloc`, `frame_plan`, `dynamic_stack_plan`, `call_plans`, `storage_plans`, `variadic_entry_plans`, `i128_carriers`, `f128_carriers`, `atomic_operations`, `intrinsic_carriers`, `inline_asm_carriers`, `i128_runtime_helpers`, `f128_runtime_helpers`. |

These cross-target dependencies prove that a final view family cannot be only
the x86 subset. They should be staged as optional feature views after the
core x86 entry surface is narrowed.

## Dependency Classification

Required core dependencies for a first `PreparedMirView`:

- target identity: target profile and target triple fallback;
- semantic BIR read access scoped to defined functions, blocks,
  instructions, globals, string constants, and BIR name tables;
- prepared name tables for function, block, value, slot, and link-name ids;
- per-function control-flow facts: prepared blocks, branch targets, branch
  conditions, join transfers, and parallel-copy bundles;
- per-function value-location facts and value-home lookup;
- stack layout: frame size, frame slots, stack objects, and slot names;
- per-function addressing facts for local/global memory access and address
  materialization;
- per-function prepared lookup indexes, owned by the view rather than by
  target-local recomputation.

Feature-specific dependencies:

- frame, dynamic-stack, call, regalloc, and storage plans;
- variadic entry plans;
- object data;
- store-source and call-argument publication plans;
- i128/f128 carriers and runtime-helper facts;
- atomic, intrinsic, and inline-asm carriers.

Diagnostic-only dependencies:

- x86 route summaries and traces in `src/backend/mir/x86/debug/debug.cpp`
  consume `PreparedBirModule` to explain route choices, grouped register
  authority, and call-source agreement. They should receive an observational
  debug/proof view, not unrestricted semantic authority.
- Prepared printer and dump filtering under `src/backend/backend.cpp:422` are
  prepared-BIR diagnostics, not MIR lowering inputs.

Accidental or compatibility dependencies:

- x86 code repeatedly passes the whole `PreparedBirModule` into helper
  functions that need only a few families. `src/backend/mir/x86/prepared/prepared.hpp:18`
  stores a raw module pointer inside `prepared::Query`; most query methods then
  extract addressing, value locations, call plans, regalloc, storage, BIR
  function, and names. This is a convenience wrapper, not a semantic need for
  full module authority.
- `module.module` is used as an all-purpose access point for functions,
  target triple, name tables, globals, and string constants. A view can split
  those into narrower read-only accessors.
- Markdown mirrors under `src/backend/mir/x86/*.md` still describe older
  surfaces and should not be counted as live dependencies.

## Smallest First PreparedMirView Dependency Set

The smallest useful first view should be read-only and x86-first:

```text
PreparedMirView
  target_profile()
  target_triple()
  bir_names()
  functions()
  globals()
  string_constants()
  prepared_names()
  function_view(function_name)

PreparedMirFunctionView
  bir_function()
  control_flow()
  value_locations()
  stack_layout()
  addressing()
  prepared_lookups()
```

This set covers the live x86 entry path from `x86::module::emit()` through
`consume_plans()`, control-flow validation, scalar/memory/call route probes,
and data emission without exposing `PreparedBirModule` wholesale. Frame,
dynamic-stack, call, regalloc, storage, variadic, object-data, special-carrier,
atomic, intrinsic, inline-asm, and runtime-helper families should be modeled as
optional feature views in later documents rather than included in the first
core contract.
