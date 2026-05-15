# AArch64 Barrier Cache Hint Builtin Intrinsic Carriers Runbook

Status: Active
Source Idea: ideas/open/242_aarch64_barrier_cache_hint_builtin_intrinsic_carriers.md
Activated From: ideas/open/242_aarch64_barrier_cache_hint_builtin_intrinsic_carriers.md

## Purpose

Define the missing structured semantic and prepared carrier authority for
AArch64 barrier, cache-maintenance, pause/hint, and builtin-address intrinsic
families before any selected-machine support can consume them.

## Goal

Accepted representatives for the owned intrinsic families expose explicit BIR
or equivalent semantic facts plus complete prepared intrinsic carriers, while
unsupported, malformed, target-mismatched, or incomplete calls remain
diagnostic-only.

## Core Rule

Carrier support must be proven before machine-node selection or printer output.
Do not infer these families from intrinsic names, assembly text, generic call
plans, atomic fence carriers, address-materialization helpers, or archived
scratch-register conventions.

## Read First

- `ideas/open/242_aarch64_barrier_cache_hint_builtin_intrinsic_carriers.md`
- `ideas/closed/239_aarch64_intrinsic_machine_nodes.md`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/lir_to_bir.cpp`
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/prepared_printer.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `tests/backend/bir/backend_lir_to_bir_notes_test.cpp`
- `tests/backend/bir/backend_prepared_printer_test.cpp`
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`

## Current Scope

- Barrier intrinsic semantic and prepared carrier facts.
- Cache-maintenance intrinsic semantic and prepared carrier facts.
- Pause/hint intrinsic semantic and prepared carrier facts.
- Builtin-address intrinsic semantic and prepared carrier facts.
- Diagnostics and tests that prove complete carriers and fail-closed neighbors.

## Non-Goals

- Do not select or print AArch64 machine records for these families.
- Do not replace atomic fence, inline asm, ordinary call, frame lowering,
  binary128, or address-materialization routes.
- Do not treat x86-only intrinsic spellings as AArch64 support.
- Do not claim progress through expectation rewrites, helper renames, or
  classification-only changes.
- Do not recover backend semantics from rendered assembly text or generic call
  metadata.

## Working Model

- BIR owns accepted intrinsic family, operation, target feature, operands,
  result, side-effect, memory-ordering, address-space, and immediate facts.
- Prepared intrinsic carriers own completeness and register/home authority.
- AArch64 selected-machine dispatch must remain a consumer only after carrier
  authority exists.
- Missing or malformed authority is a diagnostic surface, not permission to
  fabricate registers or selected records.

## Execution Rules

- Keep each step small enough to prove with `cmake --build --preset default`
  plus the supervisor-delegated backend subset.
- Add fail-closed tests next to every accepted representative.
- Prefer structured enums and carrier fields over intrinsic-name matching.
- If a family needs a separate upstream route, record that in `todo.md` and ask
  for lifecycle split instead of broadening this runbook.
- Treat selected-machine support, expectation downgrades, and single-fixture
  shortcuts as route drift.

## Step 1: Inventory Current Carrier Gaps

Goal: identify the exact semantic and prepared carrier fields missing for each
owned intrinsic family.

Primary targets:

- `ideas/open/242_aarch64_barrier_cache_hint_builtin_intrinsic_carriers.md`
- `ideas/closed/239_aarch64_intrinsic_machine_nodes.md`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/lir_to_bir.cpp`
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/prepared_printer.cpp`
- `tests/backend/bir/backend_lir_to_bir_notes_test.cpp`
- `tests/backend/bir/backend_prepared_printer_test.cpp`

Actions:

- Inspect existing BIR intrinsic family and prepared intrinsic carrier shapes.
- List the missing operation, feature, side-effect, memory-ordering,
  address-space, immediate, operand, result, and home fields for barrier,
  cache-maintenance, pause/hint, and builtin-address representatives.
- Record in `todo.md` which representatives can be completed in this runbook
  and any dependency blockers that need separate lifecycle handling.
- Confirm existing AArch64 selected-machine intrinsic tests still treat these
  families as unsupported or non-selected.

Completion check:

- `todo.md` contains a per-family readiness inventory and identifies the first
  implementable carrier packet.
- No BIR, prepared carrier, dispatch, or printer behavior changes are required
  in this step.
- The delegated inventory proof passes.

## Step 2: Define Semantic Intrinsic Facts

Goal: add BIR or equivalent semantic facts for the accepted representatives
without preparing homes or selecting machine records.

Primary targets:

- `src/backend/bir/bir.hpp`
- `src/backend/bir/lir_to_bir.cpp`
- `tests/backend/bir/backend_lir_to_bir_notes_test.cpp`

Actions:

- Add structured family and operation facts for the selected barrier,
  cache-maintenance, pause/hint, and builtin-address representatives.
- Preserve target feature, operand, result, side-effect, memory-ordering,
  address-space, and immediate facts required by later prepared carriers.
- Keep x86-only, target-mismatched, malformed, and unsupported calls
  diagnostic-only.
- Avoid changing AArch64 machine dispatch or printer support.

Completion check:

- BIR tests prove at least one accepted representative per completed family.
- Nearby malformed or unsupported cases remain missing or diagnostic-only.
- No selected AArch64 machine records are produced for these families.

## Step 3: Publish Prepared Intrinsic Carriers

Goal: expose complete prepared intrinsic carriers for the semantic facts added
in Step 2.

Primary targets:

- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/prealloc.cpp`
- `src/backend/prealloc/prepared_printer.cpp`
- `tests/backend/bir/backend_prepared_printer_test.cpp`

Actions:

- Add prepared carrier fields for feature, operation, side effects,
  memory-ordering, address-space, immediate, operands, result, and register or
  stack homes.
- Distinguish complete carriers from incomplete semantic facts, missing homes,
  malformed immediates, unsupported targets, and non-AArch64 calls.
- Print prepared diagnostics that expose carrier facts without implying
  selected-machine support.
- Preserve existing scalar, CRC, and vector prepared intrinsic behavior.

Completion check:

- Prepared-printer tests prove complete carrier facts for at least one accepted
  representative per completed family.
- Fail-closed tests cover incomplete, malformed, unsupported, and
  target-mismatched neighbors.
- Existing prepared intrinsic tests for scalar, CRC, and vector families still
  pass.

## Step 4: Guard AArch64 Machine Dispatch Boundaries

Goal: prove the new carriers are not accidentally selected or printed before a
separate machine-node route consumes them.

Primary targets:

- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`
- `tests/backend/mir/backend_aarch64_machine_printer_test.cpp`

Actions:

- Add or preserve tests showing barrier, cache-maintenance, pause/hint, and
  builtin-address carriers are non-selected unless a later plan adds selected
  machine records.
- Keep diagnostics explicit for complete-but-not-selected carriers.
- Do not add instruction records, dispatch cases, or printer spelling for these
  families in this runbook.

Completion check:

- AArch64 dispatch and printer tests prove the selected-machine boundary stays
  closed for these carriers.
- Existing scalar, CRC, and vector selected-machine behavior still passes.

## Step 5: Broader Backend Validation And Closure Readiness

Goal: prove the carrier route is coherent and decide whether the source idea is
complete or needs a follow-up split.

Primary targets:

- `todo.md`
- `test_before.log`
- `test_after.log`

Actions:

- Run the supervisor-delegated broader backend validation after the carrier
  packets land.
- Record any remaining family that lacks source authority in `todo.md` and ask
  for lifecycle split if it is outside this idea's executable scope.
- Confirm that all accepted representatives satisfy the source idea acceptance
  criteria without selected-machine support.

Completion check:

- Build, narrow backend tests, and the supervisor-selected broader backend
  check pass.
- `todo.md` identifies whether closure is ready or which dependency remains.
- The source idea can be closed only if carrier facts and fail-closed proof
  exist for the completed families required by the idea.
