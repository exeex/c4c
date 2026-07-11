# Incremental Migration Plan For MIR Consumers

This document answers Step 4's migration question: how MIR targets should move
from unrestricted `PreparedBirModule` access to `PreparedMirView` access with
the lowest risk.

The migration should be adapter-first. The first implementation should not
rewrite BIR, delete prepared fields, or move target behavior. It should create
read-only view wrappers over the current `prepare::PreparedBirModule`, migrate
one target surface to consume those wrappers, then ratchet the boundary closed.

## Migration Principles

- Start with reference-only views. The first `PreparedMirViewAdapter` should
  hold references or pointers into the existing prepared module and own only
  derived lookup caches whose lifetime is bound to the lowering call.
- Keep public backend behavior stable. Existing public entry points such as
  `x86::api::emit_prepared_module(const PreparedBirModule&)` can remain as
  compatibility wrappers while they construct the view and call the new
  internal surface.
- Move one target surface at a time. x86 should lead because Step 1 shows the
  smallest live dependency set, while RV64 and AArch64 currently consume wider
  feature families.
- Fail closed when a view does not expose a fact. A migrated target must not
  fall back to raw `PreparedBirModule` to recover missing authority.
- Treat diagnostics, route summaries, notes, and completed phases as
  observational. The Step 3 diagnostic-boundary rules apply to every phase
  below.

## Phase Table

| Phase | Main change | Rollback point | Proof category |
| --- | --- | --- | --- |
| 0. Baseline inventory lock | Keep docs 01-05 as the field inventory and reject broad implementation work until the first adapter exists. | Revert only the documentation or checklist update; no code state changes. | Documentation whitespace plus review against the Step 1 dependency table. |
| 1. Add reference-only core adapter | Introduce `PreparedMirCoreView` and `PreparedMirFunctionView` backed by `PreparedBirModule` references, exposing target identity, BIR traversal, names, control flow, value locations, stack layout, addressing, and view-owned prepared lookups. | Remove the adapter and restore old signatures because no target behavior should depend on new ownership. | Compile proof, adapter unit tests for lifetime and lookup agreement, and a canonical view dump for one x86 module. |
| 2. Migrate first x86 entry surface | Keep `x86::api::emit_prepared_module(const PreparedBirModule&)` as a wrapper, but route `x86::module::emit()` through a view-taking internal overload. | Switch the wrapper back to the old `module::emit(const PreparedBirModule&)` path. | x86 backend subset, assembly snapshot comparison for existing supported x86 cases, and no direct-emission behavior delta. |
| 3. Migrate x86 per-function consumption | Replace `consume_plans(const PreparedBirModule&, ...)` with a view/function-view consumer. Move `make_prepared_function_lookups()` ownership behind the view instead of target-local recomputation. | Restore `consume_plans()` from the old module while leaving the adapter compiled but unused. | x86 call, branch, memory, data, and contract-first stub subsets; structural comparison of old consumed plans versus view-derived consumed plans. |
| 4. Add feature-view admission gates | Add named optional views for calls, dynamic stack, storage/regalloc, variadic entry, special carriers, atomics, intrinsics, inline asm, object data, and runtime helpers. x86 should request only the features it actually lowers. | Disable each feature view independently and fall back to fail-closed unsupported diagnostics rather than raw module access. | Feature-specific admission tests, missing-feature fail-closed tests, and reviewer check that no feature silently reads raw prepared fields. |
| 5. Block new raw dependencies in MIR | Add compile-time or review-time checks that make new direct target access to `PreparedBirModule` visible and rejectable. | Temporarily allowlist a specific legacy file with an owner and deletion condition. | Static grep or lint gate plus build proof. |
| 6. Migrate RV64 text path | Wrap `riscv::codegen::emit_prepared_module_text()` and its per-function prepared lookups behind the same core and feature views. | Keep the existing `emit_prepared_module_text(const PreparedBirModule&)` wrapper as the old route. | RV64 backend text subset, prepared global storage/data subset, and comparison of old versus view-derived lookups. |
| 7. Migrate RV64 object/data consumers | Move RV64 object emission and object-data consumers to `PreparedMirObjectDataView` and core module data access. | Fall back to the old RV64 prepared-object route before target lowering if object data is incomplete. | RV64 object tests, relocation/data snapshot checks, and missing object-data fail-closed tests. |
| 8. Migrate AArch64 contexts last | Replace `FunctionLoweringContext::prepared` with `PreparedMirFunctionView` plus named feature views. Move AArch64 derived lookup pointers to view-owned or feature-owned handles. | Keep the current raw pointer context until each feature family has a view and a green structural comparison. | AArch64 compile proof, per-feature lowerer subsets, structural view comparison, and no regression in object-consumer diagnostics. |
| 9. Turn on old/new producer comparison | Once all target consumers can lower from the view, add the old/new comparison layers from doc 05. | Route all units through the old producer view when the comparator reports unsupported or blocking differences. | Schema, canonical dump, structural comparator, verifier comparator, target dry-run admission, and selected MIR/object snapshots. |

Phase 5 should not wait until every target is migrated. It should land as soon
as the first x86 surface demonstrates that ordinary lowering can be expressed
through the view.

## First X86 Surface

The first x86 surface to migrate should be the internal module-emission entry,
not the public API wrapper.

Current evidence:

- `src/backend/mir/x86/api/api.cpp:56` exposes
  `emit_prepared_module(const PreparedBirModule&)` and forwards directly to
  `x86::module::emit()`.
- `src/backend/mir/x86/module/module.cpp:6452` resolves the target profile and
  triple, builds module data, iterates `module.module.functions`, dispatches
  supported scalar routes, and emits the contract-first stub.
- `src/backend/mir/x86/x86.hpp:162` builds `ConsumedPlans` from
  `PreparedBirModule` field families and derived Route 6/source lookup state.

Recommended first cut:

```text
x86::api::emit_prepared_module(const PreparedBirModule&)
  -> PreparedMirViewAdapter old_view(prepared)
  -> x86::module::emit(const PreparedMirCoreView&)
```

The public wrapper stays stable for callers. The internal view-taking overload
should first cover target identity, target triple, data emission inputs,
defined-function iteration, `function_view()`, and prepared names. After that
is green, `consume_plans()` should become the first per-function lowering
helper migrated from raw module access to `PreparedMirFunctionView` plus named
feature views.

This sequence narrows the outer x86 entry before touching the many route
helpers. It also gives review tooling a simple rule: new x86 lowering helpers
should accept the view or a named feature view, not `PreparedBirModule`.

## Direct Dependency Gates

The migration needs both mechanical checks and review rules. A build-only
change is not enough because legacy target files will remain during the
transition.

Recommended compile-time gates:

- Put `PreparedMirView` declarations in a MIR-facing header that does not
  include `prealloc/module.hpp`.
- Move target lowering APIs toward signatures that accept
  `PreparedMirCoreView`, `PreparedMirFunctionView`, or a named feature view.
  A helper that needs `PreparedBirModule` should be visibly legacy.
- Keep the adapter implementation in a small bridge translation unit that is
  allowed to include `prealloc/module.hpp`; ordinary target lowering files
  should not need that include.
- Add static assertions or deleted overloads only after the target surface has
  been migrated enough that they will not block existing legacy code.

Recommended review-time gates:

```text
rg -n "PreparedBirModule|prealloc/prealloc.hpp|prealloc/module.hpp" \
  src/backend/mir/x86 src/backend/mir/riscv src/backend/mir/aarch64
```

During early phases this command will have legacy hits. Review should require
every hit to fall into one of these buckets:

- adapter bridge code;
- public compatibility wrapper;
- an explicitly tracked legacy target surface not yet migrated;
- diagnostic-only rendering that receives a diagnostic view rather than
  lowering authority.

New raw-module hits in migrated files should be rejected unless the patch also
updates the allowlist with an owner, deletion condition, and proof that the
fact cannot yet be expressed through a view. New diagnostic or route-text
dependencies should also be rejected under the Step 3 reviewer rules.

## RV64 Sequence

RV64 should follow x86 after the core adapter and x86 per-function migration
are stable.

Current evidence:

- `src/backend/mir/riscv/codegen/emit.cpp:62` forwards
  `emit_prepared_module()` to `emit_prepared_module_text()`.
- `src/backend/mir/riscv/codegen/prepared_module_emit.cpp:253` iterates
  `module.control_flow.functions`, builds prepared lookups, resolves prepared
  names, finds defined BIR functions in `module.module.functions`, and emits
  available edge-publication moves.

Recommended order:

1. Add an RV64 wrapper that constructs a `PreparedMirViewAdapter` and calls a
   view-taking text emitter.
2. Migrate function admission to `PreparedMirFunctionView`: control-flow
   iteration, prepared function names, defined BIR function lookup, and
   view-owned prepared lookups.
3. Move global/data emission through core globals/string constants first, then
   through `PreparedMirObjectDataView` when RV64 object emission needs
   relocation or section facts.
4. Require call/publication feature views only for RV64 routes that currently
   lower those records. Do not make RV64 object data or inline asm part of the
   core view just because RV64 consumes them.

Rollback should stay at the public wrapper boundary until the per-function
text path and object/data path both have green proof.

## AArch64 Sequence

AArch64 should be last because it currently carries the broadest raw prepared
surface inside target contexts.

Current evidence:

- `src/backend/mir/aarch64/codegen/module_compile.cpp:73` validates the
  prepared handoff and builds a module shell containing a raw prepared pointer.
- `src/backend/mir/aarch64/codegen/traversal.cpp:59` creates
  `FunctionLoweringContext` from `PreparedBirModule`, selected per-function
  plan pointers, and derived lookup objects.
- `src/backend/mir/aarch64/module/module.hpp:80` stores
  `const PreparedBirModule* prepared` in `FunctionLoweringContext`.

Recommended order:

1. Introduce a view-backed `FunctionLoweringContext` sibling that contains
   `PreparedMirFunctionView`, `PreparedMirFeatureSet`, and typed lookup handles.
2. Migrate pure core reads first: function identity, BIR function binding,
   control flow, value locations, stack layout, addressing, and target profile.
3. Migrate calls, storage/regalloc, dynamic stack, i128/f128, atomics,
   intrinsics, inline asm, runtime helpers, and object data as separate
   feature-view slices. Each slice should delete one raw pointer route from
   the context or make it legacy-only.
4. Keep object-consumer diagnostics as verifier/diagnostic facts. Do not let
   diagnostic messages or prepared consumer categories become lowering
   authority.
5. Remove `FunctionLoweringContext::prepared` only after all remaining users
   are either feature-view consumers or diagnostic-view renderers.

Rollback for AArch64 should be per feature family. The raw pointer should not
be removed until no accepted lowering path requires it.

## Proof Categories By Phase

Implementation packets should use the smallest proof that matches the phase,
then escalate when a phase changes shared target entry or feature admission.

| Phase family | Minimum proof category | Broader proof trigger |
| --- | --- | --- |
| Documentation and planning | `git diff --check` on changed docs and `todo.md`. | None unless the docs claim source evidence that cannot be found. |
| Adapter-only code | Compile proof plus adapter unit tests. | If adapter exposes derived lookups, add structural comparison against current helper output. |
| x86 entry migration | Build plus x86 backend subset and assembly snapshot comparison. | Escalate when `module::emit()` signature changes or data emission moves. |
| x86 per-function/helper migration | x86 call/branch/memory/data subsets and consumed-plan equivalence. | Escalate when Route 6, publication, or call-boundary authority changes. |
| Feature-view admission | Feature-specific fail-closed tests, including absent and incomplete views. | Escalate when a required feature becomes target-independent or shared across targets. |
| RV64 migration | RV64 text/object subset plus old/view lookup comparison. | Escalate when object data, relocations, or edge-publication moves change. |
| AArch64 migration | AArch64 build/subset plus diagnostic stability checks. | Escalate for context-wide raw-pointer removal or multiple feature families at once. |
| Old/new producer comparison | Schema, canonical dump, structural comparator, verifier comparator, and dry-run admission. | Escalate to MIR/object snapshots only after view equivalence is green. |

Example command shapes should be selected by the supervisor for the exact
slice, but the expected families are:

```text
cmake --build --preset default
ctest --test-dir build -j --output-on-failure -R '^backend_'
git diff --check -- <changed files>
```

The key acceptance rule is semantic: a green narrow test is not sufficient if
the diff adds a new raw `PreparedBirModule` dependency in migrated MIR code or
derives emission authority from diagnostics.

## Rollback Rules

Rollback should never require deleting source prepared facts or weakening
tests. The intended rollback points are:

- adapter construction can be bypassed by returning to the old public wrapper;
- a target internal overload can be disabled while the compatibility wrapper
  remains;
- a per-function helper can temporarily use the old `consume_plans()` if its
  view-derived equivalent is not structurally equal;
- an optional feature view can fail closed or route the whole unit through the
  old prepared consumer before MIR lowering;
- a new producer can be discarded before MIR lowering if view comparison is
  unsupported or blocking.

Do not roll back by making the view expose the entire `PreparedBirModule`, by
parsing diagnostic text, by weakening unsupported markers, or by changing
expectations to hide a missing view fact.
