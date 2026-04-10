# x86_64 Shared BIR Switch/Case/Goto Entry Modules After `00051.c`

Status: Open
Last Updated: 2026-04-10

## Why This Idea

While executing idea 44's bounded x86 source-backed lane after the `00050.c`
recovery, the next earliest red case `00051.c` did not continue the aggregate
or designated-initializer route.

The real `00051.c` module carries a different shape:

- one writable global selector `@x`
- one function with 24 CFG blocks
- repeated `switch(x)` entry points rather than straight compare/branch chains
- label transfers through `goto next` and `foo:`
- case labels that re-enter nested block structure before the final selector
  switch returns `x` or `1`

That is a different ownership point than the bounded global aggregate folds
already recovered in idea 44.

## Source Context

- `ideas/open/44_x86_64_backend_bringup_after_bir_cutover.md`
- `plan.md`
- `todo.md`
- `tests/c/external/c-testsuite/src/00051.c`
- `src/backend/lowering/lir_to_bir/cfg.cpp`
- `src/backend/lowering/lir_to_bir/phi.cpp`
- `src/backend/lowering/lir_to_bir.cpp`

## Current Snapshot

Focused inspection on 2026-04-10 shows:

- `c_testsuite_x86_backend_src_00051_c` still fails at the unsupported x86
  direct-LIR boundary
- `ctest --test-dir build --output-on-failure -R '^(c_testsuite_x86_backend_src_00051_c)$'`
  reproduces the same boundary in isolation
- `./build/c4cll --dump-hir-summary tests/c/external/c-testsuite/src/00051.c`
  reports `functions=1 globals=1 blocks=24 statements=37 expressions=19`
- `./build/c4cll --codegen llvm tests/c/external/c-testsuite/src/00051.c`
  shows a one-function CFG with:
  - four `switch i32` terminators over loads from `@x`
  - bridge blocks that model fallthrough between case regions
  - label-targeted edges for `next` and `foo`
  - a final three-way switch that returns `%t7`, `1`, or `1`

This is a shared-BIR control-flow route, not another bounded aggregate fold.

## Goal

Recover the smallest shared-BIR seam that can own switch/case/goto-entry
modules like `00051.c` on x86_64 without reviving deleted legacy fallback
behavior.

## Non-Goals

- do not silently absorb this seam into idea 44's aggregate recovery lane
- do not widen this into the broader switch/fallthrough loop work already
  parked in idea 46
- do not revive legacy backend IR or LLVM asm rescue
- do not mix this with the global-rich Family B route parked in idea 48

## Proposed Plan

### Step 1: Isolate The First Shared CFG Contract

- determine the minimal owned feature set among:
  - switch terminator lowering
  - block-to-block fallthrough normalization
  - label/goto edge preservation
  - case/default predecessor mapping for phi and CFG repair
- reduce `00051.c` to the smallest source-backed or synthetic reproduction if
  the full module is larger than the first owning seam

### Step 2: Add Focused Shared-Lowering Coverage

- add shared-BIR regression coverage for the chosen switch/case/goto contract
- add an x86 backend route regression that proves the path reaches native asm
  instead of the unsupported direct-LIR boundary

### Step 3: Recover The Smallest Shared Route

- expand shared lowering only enough to cover the owned control-flow seam
- keep explicit unsupported diagnostics for larger switch-heavy shapes that
  still remain outside ownership

### Step 4: Revalidate The x86 Source Route

- rerun the focused route regression and `c_testsuite_x86_backend_src_00051_c`
- refresh the broad-suite after-log and compare it against the parked baseline
