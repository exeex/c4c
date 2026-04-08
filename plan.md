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

Sub-milestones:

1. eliminate any remaining app-layer/backend fallback callers so backend asm
   either emits natively or fails explicitly
2. remove live production ownership of `lir_to_backend_ir.*` from x86/aarch64
   emitters and backend route selectors
3. delete the dead legacy backend IR files, tests, and build wiring once the
   surviving paths are BIR-only

Actions:

- keep unsupported backend asm behavior explicit and non-fallbacking on stdout
  and file output throughout the cutover
- batch work by live production seam, not by single testcase
- when a seam is proven dead, delete the matching production caller, then
  delete its tests/build wiring in the same slice
- keep `lir_to_backend_ir.*` and `ir.*` deletion grouped into explicit
  removal batches rather than letting them linger behind test-only progress

Current next slice for Step 4:

- first remove the remaining production-side `main` fixture anchors from
  emitter and shared parser code, starting with
  [`src/backend/x86/codegen/emit.cpp`](/workspaces/c4c/src/backend/x86/codegen/emit.cpp),
  [`src/backend/aarch64/codegen/emit.cpp`](/workspaces/c4c/src/backend/aarch64/codegen/emit.cpp),
  and
  [`src/backend/lowering/call_decode.hpp`](/workspaces/c4c/src/backend/lowering/call_decode.hpp)
- treat any assumption that "`main` identifies the top-level caller for a
  bounded fixture shape" as test-only convenience, not production semantics
- move any remaining fixture convenience for manufacturing or locating `main`
  into backend test helpers/fixtures instead of leaving it in emitter/parser
  code
- after the production `main` anchors are gone, continue deleting the next
  live emitter-local `lower_lir_to_backend_module(...)` seam in the same batch
- prove the batch with focused backend tests plus `ctest -R backend`

Batch completion check:

- no emitter/parser production path depends on `function.name == "main"` just
  to recognize a bounded fixture shape
- any helper logic that still needs to manufacture or locate `main` for tests
  lives under backend test helper/fixture ownership instead of production code
- at least one live production legacy seam disappears
- the batch also shrinks the matching test/build assumptions
- the slice leaves the repo with fewer `lir_to_backend_ir` owners than before

Step 4 commit quality bar:

- do not treat test-only or probe-only commits as meaningful Step 4 progress
- every Step 4 implementation commit should remove or tighten at least one
  live production legacy path, fallback branch, or build-wired legacy caller
- when tests are added or migrated in Step 4, they should accompany the
  production deletion in the same bounded batch rather than stand alone as the
  whole slice
- if a slice cannot yet delete a production seam, keep it framed as audit or
  preparation work rather than claiming Step 4 removal progress

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
