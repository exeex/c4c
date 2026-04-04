# BIR Full Coverage and Legacy IR Removal

Status: Active
Source Idea: ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md
Activated from: ideas/closed/40_target_profile_and_execution_domain_foundation.md

## Purpose

Resume the BIR migration now that the minimal target/execution-domain
foundation is in place, and remove both legacy backend IR and backend-facing
LLVM rescue behavior without letting test coverage collapse back onto the
`riscv64` passthrough oracle.

## Goal

Make BIR the only backend IR, migrate x86/aarch64 emitters to consume it
directly, and delete legacy backend-IR plus LLVM rescue paths.

## Read First

- [ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md](/workspaces/c4c/ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md)
- [src/backend/backend.cpp](/workspaces/c4c/src/backend/backend.cpp)
- [src/backend/backend.hpp](/workspaces/c4c/src/backend/backend.hpp)
- [src/backend/bir.hpp](/workspaces/c4c/src/backend/bir.hpp)
- [src/backend/lowering/lir_to_bir.cpp](/workspaces/c4c/src/backend/lowering/lir_to_bir.cpp)
- [src/backend/lowering/lir_to_backend_ir.cpp](/workspaces/c4c/src/backend/lowering/lir_to_backend_ir.cpp)
- [src/apps/c4cll.cpp](/workspaces/c4c/src/apps/c4cll.cpp)

## Execution Rules

- do not use `riscv64` passthrough output as the default proof that BIR stayed
  off fallback paths
- prefer target-neutral route/lowering checks when the question is about
  `lir_to_bir` or fallback selection
- use `x86_64` / `aarch64` tests when the question is really about emitter
  behavior
- for this plan's routine validation, use `ctest -R backend` as the required
  regression scope instead of the full `ctest` suite unless a change clearly
  escapes backend ownership
- when a lowering or route surface has multiple low-coupling fixture variants
  in the same family, prefer landing that family as one bounded batch instead
  of one test at a time
- keep transitional legacy-bucket tests clearly marked and delete them once
  their coverage is either migrated or made irrelevant

## Step 1. Re-audit legacy boundaries

Goal: refresh the exact production and test seams that still depend on
`BackendModule(ir.*)` or backend-facing LLVM rescue.

Actions:

- refresh the code boundary inventory in `backend.*`, lowering, emitters, and
  `c4cll`
- refresh the legacy-coupled test inventory, including temporary extracted
  buckets
- identify which seams need structured route/fallback observation instead of
  `riscv64` text inference

Completion Check:

- the current removal map is concrete enough to drive implementation without
  leaning on stale assumptions

## Step 2. Expand BIR coverage without RV64 overfitting

Goal: keep porting `lir_to_backend_ir` behavior into `lir_to_bir`, but validate
through target-neutral seams first and use per-target emitters second.

Actions:

- port missing lowering clusters one bounded slice at a time
- add target-neutral lowering/route tests where possible
- prefer batching same-shape fixture families together when they share the same
  route/lowering seam and are unlikely to interfere with each other
- keep any `riscv64` route tests explicitly temporary and bounded
- extend native x86/aarch64 emitter coverage for slices that are now BIR-owned

Completion Check:

- new BIR coverage no longer depends primarily on `riscv64` passthrough tests

## Step 3. Migrate emitter-facing contracts

Goal: move x86/aarch64 emission off `ir.*` and onto BIR-native structures.

Actions:

- port printer/validator/data-structure parity needed by emitters
- switch emitter includes and call sites from `ir.*` to BIR
- remove `bir_to_backend_ir` usage as emitter-side crutch

Completion Check:

- x86/aarch64 emission works from BIR-native ownership

## Step 4. Delete legacy/backend LLVM rescue paths

Goal: remove the now-obsolete backend IR and app-layer LLVM rescue behavior.

Actions:

- delete `lir_to_backend_ir.*`, `bir_to_backend_ir.*`, `ir.*`
- remove legacy routing from `backend.cpp`
- remove backend-facing LLVM rescue helpers from `c4cll`
- convert unsupported backend asm into explicit backend errors

Completion Check:

- there is only one backend IR path and no backend-to-LLVM rescue path left

## Step 5. Final test cleanup

Goal: remove transitional test buckets and align test families with the new
BIR-only architecture.

Actions:

- delete temporary extracted legacy test buckets once coverage is migrated
- retire legacy adapter/shared-util tests that only existed for `BackendModule`
- keep platform-specific test files only where they still assert real
  platform-specific behavior

Completion Check:

- test layout reflects the new architecture instead of the migration scaffolding
