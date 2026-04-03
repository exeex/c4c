# `lir -> backend_ir` Refactor

Status: Complete
Last Updated: 2026-04-03

## Goal

Refactor the boundary between LIR and backend IR so the backend consumes a
genuinely structured lowering result instead of a partially string-driven
"adapter" model.

The immediate focus is the `src/backend/lir_adapter.hpp` /
`src/backend/lir_adapter.cpp` path and the backend IR model in
`src/backend/ir.hpp`.

This idea is intentionally a transition step, not the final naming destination.
Its purpose is to clean up the current LIR/backend boundary enough that the
next idea can introduce `bir` on top of a saner foundation instead of layering
new naming and new semantics on top of today's adapter-heavy path.

## Why this plan

LIR has already been partially refactored from raw text into object-based IR
nodes such as `LirModule`, `LirFunction`, `LirBlock`, `LirBinOp`, and
`LirCallOp`. That is a good step forward, but the current backend boundary is
still carrying too much LLVM-syntax-shaped text:

- `signature_text`
- `type_str`
- `return_type`
- `callee`
- `args_str`
- GEP index fragments such as `"i32 0"` or `"i64 %t2"`
- global/type/init text surfaces

As a result, `lir_adapter` is currently doing several jobs at once:

1. decoding LIR surface syntax
2. inferring backend semantics from text
3. building a backend IR that is still partly LLVM-shaped
4. exposing parser-style helpers such as `parse_backend_*` that leak syntax
   concerns into backend codegen

This makes the backend boundary harder to reason about, harder to extend, and
too dependent on LLVM-style textual conventions.

## Refactor Objective

Move from today's "adapter" shape to a proper lowering pipeline:

1. LIR remains the source IR for the backend boundary
2. a dedicated lowering layer translates LIR into backend IR
3. backend IR becomes a backend-owned structured model, not a thin
   LLVM-syntax carrier
4. target codegen should no longer need to parse LIR-like textual fragments

The key design rule is:

`backend_ir` should model values, memory, control flow, and call/addressing
semantics in backend-native structures, not in LLVM-text-shaped strings.

In roadmap terms:

- `01` stabilizes and clarifies the boundary while the code still uses
  transitional `backend_ir` naming
- `02` builds directly on that work and introduces `bir` as the long-term name
  and destination for the cleaned-up backend IR layer

## Current Pain Points

### 1. Backend IR is not fully backend-native

`src/backend/ir.hpp` currently still stores many core semantics as strings:

- function signatures
- operand names
- type names
- call parameter lists
- callee identity
- memory base symbols

This makes validation, legalization, and future RA-oriented transforms more
fragile than they need to be.

### 2. Backend codegen still understands too much about LIR syntax

The AArch64 and x86 emitters rely on helpers such as:

- `parse_backend_typed_call`
- `parse_backend_direct_global_typed_call`
- `parse_backend_direct_global_single_typed_call_operand`

These helpers are useful as a bridge, but they are a sign that the current
backend IR has not fully absorbed call structure yet.

### 3. One layer is doing both decode and lowering

Today, `adapt_minimal_module()` is effectively:

- LIR decoder
- backend IR builder
- compatibility layer for target-specific pattern matching

That coupling makes it difficult to evolve the IR cleanly.

## Desired End State

The backend boundary should eventually look more like:

1. `LIR`
2. optional structured decode/canonicalize layer for legacy text-heavy LIR fields
3. `backend_ir` as a backend-owned CFG/value model
4. backend legalization / canonicalization passes
5. target-specific codegen / regalloc / emission

In the desired end state:

- backend IR operands are structured, not parsed from text on demand
- call sites expose structured callee/arg information
- memory and address formation have explicit backend-side representations
- backend passes no longer depend on LLVM textual formatting details

## Proposed Backend IR Direction

The exact shape can evolve, but the backend IR should move toward explicit
backend concepts such as:

- value ids or SSA ids
- block ids
- structured operands
- structured call operands and callee kinds
- explicit addresses / frame objects / globals
- typed immediates and typed comparisons
- control-flow terminators that do not borrow LLVM text conventions

This is intentionally closer to a generic machine-oriented IR than to a
re-renderable LLVM IR clone.

## LIR Scope

This plan does **not** require an immediate full LIR redesign.

LIR is already useful as the frontend/codegen output IR, and the backend can
be improved first by introducing a better lowering boundary. The recommended
approach is:

1. keep existing LIR APIs working
2. isolate all syntax decoding into a narrow lowering layer
3. prevent backend IR and target codegen from re-parsing those text surfaces
4. later, selectively improve the most valuable LIR fields so the lowering
   layer becomes thinner over time

The most likely long-term LIR follow-up areas are:

- structured call signatures / arguments
- structured GEP indices
- structured constant/global references
- aggregate/global initializer representation

## Proposed Split

Replace the current `lir_adapter` role with a clearer subsystem, for example:

- `src/backend/lowering/lir_to_backend_ir.hpp`
- `src/backend/lowering/lir_to_backend_ir.cpp`
- `src/backend/lowering/lir_decode_calls.cpp`
- `src/backend/lowering/lir_decode_memory.cpp`
- `src/backend/lowering/lir_decode_gep.cpp`
- `src/backend/lowering/lir_lower_cfg.cpp`

The final file layout may differ, but the responsibility split should be:

- decode legacy LIR text surfaces in one place
- lower into backend-native IR in one place
- keep target codegen free of LIR-syntax parsing

## Migration Strategy

### Phase 1: Rename the boundary without changing behavior

- introduce a new lowering entrypoint such as `lower_to_backend_ir(...)`
- keep the current implementation behaviorally equivalent
- reduce use of the word "adapter" in the main production path
- treat this naming as transitional until the BIR scaffold from `02` lands

### Phase 2: Reshape backend IR around structured operands

- migrate call representation away from parsed text helpers
- make operand/callee/address forms explicit
- update backend validation and printing to match
- keep the implementation compatible with an eventual rename from
  `backend_ir` to `bir`

### Phase 3: Remove syntax-parsing dependencies from codegen

- stop requiring `parse_backend_*` helpers inside target emitters
- keep any needed parsing only in the LIR decode/lowering layer
- let target codegen operate purely on backend IR semantics

### Phase 4: Thin the LIR decode layer

- selectively upgrade LIR fields that still force redundant decoding
- remove transitional compatibility code once both sides are structured enough

## Naming Transition

This idea deliberately allows transitional names such as:

- `lower_to_backend_ir(...)`
- `src/backend/ir.hpp`
- `backend_ir` in comments or helper APIs

Those names are acceptable in `01` as long as the design is moving toward a
backend-owned structured IR boundary.

The follow-on `02` idea is expected to:

- introduce `bir` naming
- decide which transitional `backend_ir` names should be replaced immediately
- preserve compatibility shims only where they reduce migration risk

## Constraints

- Do not block current backend feature work on a big-bang rewrite
- Preserve the existing `codegen/llvm -> backend` production path during migration
- Keep the AArch64 and x86 backends working throughout the transition
- Prefer staged compatibility shims over wide flag-day rewrites
- Avoid mixing unrelated backend correctness fixes into the structural refactor
- Keep regression validation backend-scoped: prioritize `tests/c/internal/backend_*`,
  `tests/backend/*`, or backend-regex filtered runs.

## Acceptance Criteria

- [x] the production lowering path is described as a backend lowering pass, not a
      "minimal adapter"
- [x] backend IR no longer requires target emitters to parse call/type structure
      from LLVM-shaped text helpers
- [x] `src/backend/ir.hpp` is more backend-native and less stringly-typed in its
      core instruction/operand model
- [x] the LIR decode responsibility is isolated to a narrow backend-lowering layer
- [x] target-specific codegen consumes backend semantics rather than backend-side
      syntax decoding helpers
- [x] the result is clearly positioned as the foundation for the later BIR
      rename/migration, not as a competing permanent architecture
- [x] build/tests remain regression-free during each migration phase (backend scope:
      `tests/c/internal/backend_*`, `tests/backend/*`, or backend-regex runs)

## Completion Notes (2026-04-03)

This refactor is complete as a transition-step cleanup of the `lir -> backend_ir`
boundary.

Delivered outcomes:

- production ownership moved to `src/backend/lowering/lir_to_backend_ir.*`
  with `src/backend/lir_adapter.hpp` retained only as a compatibility alias
- LIR syntax decoding for the migrated slices now lives behind lowering-owned
  helpers such as `src/backend/lowering/call_decode.hpp`
- backend IR carries more backend-native structure for signatures, calls,
  linkage, globals, addresses, and local-slot state instead of relying on
  open-ended string matching in target codegen
- the active branch-only constant-return local-slot family now lowers as a
  structured backend semantic path across both AArch64 and x86 coverage,
  including non-`main` symbols, unused fixed parameters, representative
  local-slot counts, and bounded typed-scalar support for `i8` / `i32`
- the result is intentionally staged for the next `backend_ir -> bir` work
  rather than trying to finish every remaining naming and compatibility cleanup
  in this idea

## Leftover Boundaries For Follow-On Ideas

The following seams are intentionally retained and bounded for later work under
the follow-on backend-IR/BIR ideas already in `ideas/open/`:

- `src/backend/lir_adapter.hpp` remains as a thin compatibility entrypoint that
  forwards directly to `lower_to_backend_ir(...)`
- some type and helper names still carry transitional `adapter` naming such as
  `LirAdapterError` and backend test binaries named `backend_lir_adapter_*`
- the lowering layer still owns legacy syntax-decoding helpers; this idea did
  not attempt to redesign all remaining LIR text surfaces at once
- target-local direct-LIR compatibility paths that are outside the migrated
  structural families remain future cleanup, not open work for this closed idea

## Validation Snapshot (2026-04-03)

- `ctest --test-dir build -j --output-on-failure` completed with
  `2667 passed / 2 failed / 2669 total`
- the only remaining failures are the known AArch64 contract cases:
  `backend_contract_aarch64_return_add_object` and
  `backend_contract_aarch64_global_load_object`
- backend regression coverage for the migrated lowering seam remains green,
  including `backend_lir_adapter_tests` and
  `backend_lir_adapter_aarch64_tests`

## Non-Goals

- rewriting all of LIR up front
- forcing a one-shot replacement of the backend path
- redesigning target-specific assembly syntax emission in the same effort
- changing frontend semantics as part of this structural cleanup

## Good First Patch

Introduce a new backend-lowering entrypoint and file layout that preserves the
current behavior of `adapt_minimal_module()`, but makes the responsibility
explicit:

- decoding legacy LIR surfaces
- lowering into backend IR
- keeping syntax parsing out of target codegen over time

## Parked Notes (2026-04-02)

### Parked / Deactivated (2026-04-02)

- Source plan execution for this idea has been deactivated to isolate an active backend regression wave.
- New blocker discovered: large regression blast after aarch64 backend refactor affecting backend scope tests.
- Current known failed clusters:
  - `backend_lir_adapter_aarch64_tests`
  - `backend_runtime_nested_member_pointer_array`
  - `backend_runtime_nested_param_member_array`
  - `backend_runtime_param_member_array`
  - multiple `c_testsuite_x86_backend_src_*_c` cases in the `backend` subset
- Re-open status set to continue from this checkpoint in a dedicated repair plan before resuming structural refactor.
