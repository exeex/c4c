# AArch64 Cache Hint Builtin Intrinsic Carriers Runbook

Status: Active
Source Idea: ideas/open/243_aarch64_cache_hint_builtin_intrinsic_carriers.md
Activated From: ideas/open/243_aarch64_cache_hint_builtin_intrinsic_carriers.md

## Purpose

Define structured semantic and prepared carrier authority for the remaining
AArch64 cache-maintenance, pause/hint, and builtin-address intrinsic families
before any selected-machine route can consume them.

## Goal

Accepted representatives expose explicit BIR or equivalent semantic facts plus
complete prepared intrinsic carriers, while incomplete, malformed,
target-mismatched, unsupported, or separately owned calls remain diagnostic-only
or on their existing routes.

## Core Rule

Carrier authority must exist before machine-node selection or printer output.
Do not infer support from intrinsic names, rendered assembly text, generic call
plans, archived scratch-register conventions, TLS metadata, or
address-materialization helpers.

## Read First

- `ideas/open/243_aarch64_cache_hint_builtin_intrinsic_carriers.md`
- `ideas/closed/242_aarch64_barrier_cache_hint_builtin_intrinsic_carriers.md`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/lir_to_bir.cpp`
- `src/backend/bir/lir_to_bir/calling.cpp`
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/prealloc.cpp`
- `src/backend/prealloc/prepared_printer.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- `tests/backend/bir/backend_lir_to_bir_notes_test.cpp`
- `tests/backend/bir/backend_prepared_printer_test.cpp`
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`
- `tests/backend/mir/backend_aarch64_machine_printer_test.cpp`

## Current Scope

- Cache-maintenance intrinsic semantic and prepared carrier facts.
- Pause/hint intrinsic semantic and prepared carrier facts.
- Builtin-address intrinsic semantic and prepared carrier facts where this
  route owns the carrier boundary.
- Diagnostics and tests that prove complete carrier fields without implying
  selected-machine support.
- Explicit handoff notes for builtin-address cases that remain owned by
  address-materialization or TLS routes.

## Non-Goals

- Do not select or print AArch64 machine records for these families.
- Do not reopen DMB barrier carrier support unless a regression is found.
- Do not replace atomic fence, inline asm, ordinary call, frame lowering,
  binary128, TLS, or address-materialization routes.
- Do not treat x86-only intrinsic spellings as AArch64 support.
- Do not claim progress through expectation rewrites, helper renames, or
  classification-only changes.
- Do not recover backend semantics from rendered assembly text or generic call
  metadata.

## Working Model

- BIR owns accepted intrinsic family, operation, target feature, operands,
  result, side-effect, memory-ordering, address-space, and immediate facts.
- Prepared intrinsic carriers own completeness plus register, immediate, and
  home authority needed by later AArch64 selection.
- Builtin-address support must explicitly separate intrinsic-carrier authority
  from existing address-materialization and TLS ownership.
- AArch64 selected-machine dispatch stays closed until a later machine-node
  route consumes these carriers.
- Missing or malformed authority is a diagnostic surface, not permission to
  fabricate registers or selected records.

## Execution Rules

- Keep each step small enough to prove with `cmake --build --preset default`
  plus the supervisor-delegated backend subset.
- Add fail-closed tests next to every accepted representative.
- Prefer structured enums and carrier fields over intrinsic-name matching.
- Record family-specific blockers in `todo.md` and ask for lifecycle split
  instead of broadening this runbook into unrelated lowering routes.
- Treat selected-machine support, expectation downgrades, and single-fixture
  shortcuts as route drift.

## Step 1: Inventory Remaining Carrier Gaps

Goal: identify exact semantic and prepared carrier fields missing for the
remaining cache-maintenance, pause/hint, and builtin-address families.

Primary targets:

- `ideas/open/243_aarch64_cache_hint_builtin_intrinsic_carriers.md`
- `ideas/closed/242_aarch64_barrier_cache_hint_builtin_intrinsic_carriers.md`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/lir_to_bir.cpp`
- `src/backend/bir/lir_to_bir/calling.cpp`
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/prealloc.cpp`
- `src/backend/prealloc/prepared_printer.cpp`
- `tests/backend/bir/backend_lir_to_bir_notes_test.cpp`
- `tests/backend/bir/backend_prepared_printer_test.cpp`

Actions:

- Inspect existing BIR intrinsic family and prepared intrinsic carrier shapes.
- List the missing operation, feature, side-effect, memory-ordering,
  address-space, immediate, operand, result, and home fields for each
  remaining family.
- Identify which representatives can be completed as intrinsic carriers in
  this route and which builtin-address cases must stay with
  address-materialization or TLS ownership.
- Confirm existing AArch64 selected-machine intrinsic tests still treat these
  families as unsupported or non-selected.

Completion check:

- `todo.md` contains a per-family readiness inventory and identifies the first
  implementable carrier packet.
- Builtin-address ownership boundaries are explicit before implementation.
- No BIR, prepared carrier, dispatch, or printer behavior changes are required
  in this step.

## Step 2: Define Cache-Maintenance Carrier Facts

Goal: add semantic and prepared carrier authority for accepted AArch64
cache-maintenance representatives without selecting machine records.

Primary targets:

- `src/backend/bir/bir.hpp`
- `src/backend/bir/lir_to_bir.cpp`
- `src/backend/bir/lir_to_bir/calling.cpp`
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/prealloc.cpp`
- `src/backend/prealloc/prepared_printer.cpp`
- `tests/backend/bir/backend_lir_to_bir_notes_test.cpp`
- `tests/backend/bir/backend_prepared_printer_test.cpp`

Actions:

- Add structured family and operation facts for the accepted
  cache-maintenance representative.
- Preserve target feature, side effects, memory behavior, address-space or
  address operand facts, immediates, and homes required by later selection.
- Publish prepared diagnostics that expose complete cache carrier fields
  without implying selected-machine support.
- Keep malformed, target-mismatched, unsupported, and missing-authority calls
  diagnostic-only or absent.

Completion check:

- BIR and prepared-printer tests prove at least one complete cache-maintenance
  carrier and nearby fail-closed cases.
- Existing DMB, scalar, CRC, and vector carrier tests still pass.
- AArch64 dispatch remains non-selecting for cache-maintenance carriers.

## Step 3: Define Pause/Hint Carrier Facts

Goal: add semantic and prepared carrier authority for accepted AArch64
pause/hint representatives without selected-machine support.

Primary targets:

- `src/backend/bir/bir.hpp`
- `src/backend/bir/lir_to_bir.cpp`
- `src/backend/bir/lir_to_bir/calling.cpp`
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/prealloc.cpp`
- `src/backend/prealloc/prepared_printer.cpp`
- `tests/backend/bir/backend_lir_to_bir_notes_test.cpp`
- `tests/backend/bir/backend_prepared_printer_test.cpp`

Actions:

- Add structured family and operation facts for the accepted pause/hint
  representative.
- Preserve target feature, no-result behavior, side effects, immediates, and
  any ordering facts required by later AArch64 selection.
- Publish prepared diagnostics that expose complete pause/hint carrier fields.
- Keep unsupported hint names, target mismatches, malformed immediates, and
  missing-authority cases diagnostic-only.

Completion check:

- BIR and prepared-printer tests prove at least one complete pause/hint carrier
  and nearby fail-closed cases.
- Existing DMB, cache, scalar, CRC, and vector carrier tests still pass.
- AArch64 dispatch remains non-selecting for pause/hint carriers.

## Step 4: Define Builtin-Address Ownership And Carriers

Goal: establish which builtin-address representatives are intrinsic carriers
and which remain owned by address-materialization or TLS routes.

Primary targets:

- `src/backend/bir/bir.hpp`
- `src/backend/bir/lir_to_bir.cpp`
- `src/backend/bir/lir_to_bir/calling.cpp`
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/prealloc.cpp`
- `src/backend/prealloc/prepared_printer.cpp`
- `tests/backend/bir/backend_lir_to_bir_notes_test.cpp`
- `tests/backend/bir/backend_prepared_printer_test.cpp`

Actions:

- Define the builtin-address representatives that this intrinsic-carrier route
  owns, if any, with explicit semantic family and operation facts.
- Preserve result, address-space, operand, side-effect, target feature, and
  home authority needed by later selection.
- Record in `todo.md` any builtin-address case that must remain outside this
  route because address-materialization or TLS owns it.
- Keep existing address-materialization and TLS behavior intact.

Completion check:

- Tests prove accepted builtin-address carrier facts or document a justified
  lifecycle handoff when no representative belongs in this route.
- Fail-closed tests cover target-mismatched, unsupported, and missing-authority
  neighbors.
- No address-materialization, TLS, dispatch, or printer route is broadly
  rewritten.

## Step 5: Guard AArch64 Machine Dispatch Boundaries

Goal: prove the new carriers are not accidentally selected or printed before a
separate machine-node route consumes them.

Primary targets:

- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`
- `tests/backend/mir/backend_aarch64_machine_printer_test.cpp`

Actions:

- Add or preserve tests showing complete cache-maintenance, pause/hint, and
  builtin-address carriers are non-selected unless a later plan adds selected
  machine records.
- Keep diagnostics explicit for complete-but-not-selected carriers.
- Do not add instruction records, dispatch cases, or printer spelling for
  these families in this runbook.

Completion check:

- AArch64 dispatch and printer tests prove the selected-machine boundary stays
  closed for these carriers.
- Existing scalar, CRC, vector, and DMB boundary behavior still passes.

## Step 6: Broader Backend Validation And Closure Readiness

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
- Confirm that completed representatives satisfy the source idea acceptance
  criteria without selected-machine support.

Completion check:

- Build, narrow backend tests, and the supervisor-selected broader backend
  check pass.
- `todo.md` identifies whether closure is ready or which dependency remains.
- The source idea can be closed only if carrier facts and fail-closed proof
  exist for the completed families required by the idea, or if any remainder is
  explicitly split into follow-up source intent.
