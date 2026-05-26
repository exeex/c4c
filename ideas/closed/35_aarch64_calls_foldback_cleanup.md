# AArch64 Calls Fold-Back Cleanup

## Goal

Fold target-local AArch64 calls helper files back into the `calls.cpp` owner
family without changing shared call-preparation authority.

## Why This Exists

The AArch64 layout classification marked the extra calls helpers as
mechanical layout debt. These files consume prepared call plans, prepared
byval source selections, ABI placement facts, and preservation publication
records, then emit AArch64 call-boundary moves and operands. That residue
belongs with the reference-style `calls` owner instead of separate
AArch64-only translation units.

## In Scope

- Consolidate these AArch64 target-local helper families into the calls owner:
  - `src/backend/mir/aarch64/codegen/calls_byval_aggregates.cpp`
  - `src/backend/mir/aarch64/codegen/calls_common.cpp`
  - `src/backend/mir/aarch64/codegen/calls_dispatch_bridge.cpp`
  - `src/backend/mir/aarch64/codegen/calls_dispatch_bridge.hpp`
  - `src/backend/mir/aarch64/codegen/calls_moves.cpp`
- Preserve `calls.cpp` and `calls.hpp` as the AArch64 calls/ABI lowering entry
  points.
- Keep prepared call-plan, byval lane source, preservation, and return
  publication authority outside the mechanical fold-back.
- Update build metadata only as needed to remove folded-back translation
  units.
- Keep existing call-boundary diagnostics and ABI behavior intact.

## Out Of Scope

- Changing call ABI semantics, register assignment, stack-argument layout, or
  prepared call planning.
- Moving shared call preparation into AArch64 codegen.
- Reopening byval source-selection authority already handled by earlier ideas.
- Folding unrelated `returns`, `prologue`, or `memory` helpers.
- Broad dispatch rewrites beyond replacing calls bridge includes/calls with
  the folded calls owner API.

## Acceptance Criteria

- The listed calls helper implementation is owned by the calls family instead
  of separate extra AArch64 calls translation units.
- Call-boundary moves, byval aggregate handling, dynamic-stack helper calls,
  call result recording, and diagnostics keep their previous behavior.
- Build metadata no longer depends on removed calls helper translation units.
- Validation covers focused AArch64 call lowering plus an appropriate backend
  build or test bucket.

## Reviewer Reject Signals

- The patch changes prepared call-plan, byval source, preservation, or return
  publication semantics while presenting the work as mechanical fold-back.
- The patch fixes one named call fixture by special-casing a function name,
  value id, aggregate shape, ABI register, or stack offset.
- The patch weakens unsupported diagnostics, removes coverage, or rewrites
  expectations to hide a call-boundary regression.
- The patch leaves the same split helper ownership under new filenames or a
  new bridge layer outside the calls owner.
- The patch folds unrelated memory, prologue, comparison, or dispatch cleanup
  into this calls slice.

## Closure

Closed after the listed AArch64 calls helper implementation was folded into
the `calls.cpp` / `calls.hpp` owner surface and obsolete helper files were
removed from build metadata.

Evidence:
- `calls_byval_aggregates.cpp`, `calls_common.cpp`, `calls_moves.cpp`, and
  `calls_dispatch_bridge.*` are no longer referenced by source, build metadata,
  or tests.
- Reviewer scrutiny judged the route aligned with the source idea and found no
  behavioral blocker; the stale ownership wording finding was resolved by
  commit `831ef0f13`.
- Focused AArch64 call proof passed during execution.
- Backend regression guard passed with matching before/after backend logs:
  163/163 before and 163/163 after, no new failures.
- Full-suite baseline at commit `2d630797a` recorded 3411/3411 passing.
