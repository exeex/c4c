# Shared BIR Family B Recursive Global Pointer Routes After x86 `00040.c`

Status: Open
Last Updated: 2026-04-10

## Why This Idea

While executing idea 44's bounded x86 source-backed lane after the `00039.c`
and `00041.c` recoveries, the next earliest red case `00040.c` did not fit the
current single-function constant-return seams.

The real `00040.c` module carries a different shape:

- two writable globals (`@N` and `@t`)
- a declared libc-facing `calloc`
- a helper function `chk`
- a recursive `go` function
- pointer-indexed global memory loads and stores through the heap-backed board

That is a broader shared-BIR ownership problem than the bounded single-function
loop and local-slot folds that idea 44 is currently using to recover adjacent
x86 cases.

## Source Context

- `ideas/open/44_x86_64_backend_bringup_after_bir_cutover.md`
- `plan.md`
- `todo.md`
- `tests/c/external/c-testsuite/src/00040.c`
- `src/backend/lowering/lir_to_bir.cpp`
- `src/backend/lowering/lir_to_bir/memory.cpp`
- `src/backend/lowering/lir_to_bir/calls.cpp`

## Current Snapshot

Focused inspection on 2026-04-10 shows:

- `c_testsuite_x86_backend_src_00040_c` still fails at the unsupported x86
  direct-LIR boundary
- `--dump-hir-summary` reports `functions=113 globals=2 blocks=29`
- `--codegen llvm` reveals the real ownership surface is not a one-function
  local-slot fold:
  - `chk(i32, i32)` performs repeated indexed loads through global pointer `@t`
  - `go(i32, i32, i32)` performs recursion plus global increments/decrements
  - `main()` calls declared `calloc`, stores the returned pointer into `@t`,
    calls `go`, then checks `@N != 92`

This is a Family B shared-BIR/global-call route, not just another neighboring
`0003x/0004x` local arithmetic seam.

## Goal

Recover a bounded shared-BIR route for the smallest useful subset of
multi-function/global-pointer/call-backed x86 source modules represented by
`00040.c`, without reviving legacy backend IR or LLVM asm rescue.

## Non-Goals

- do not silently absorb this work into idea 44's simpler local-slot lane
- do not revive deleted fallback behavior
- do not broaden this into all remaining global-rich x86 failures at once
- do not mix this with the parked shared-BIR select regression from idea 47

## Proposed Plan

### Step 1: Isolate The First Owning Boundary

- confirm whether the first blocker is:
  - global-pointer memory lowering
  - declared-call lowering
  - multi-function ownership
  - recursive call support
- capture the smallest source-backed reduction that still reproduces the same
  unsupported boundary

### Step 2: Add Focused Shared-Lowering Coverage

- pin the smallest representative route in shared BIR tests before changing
  lowering behavior
- keep the regression focused on the owned global/call seam instead of
  hard-coding the whole search program

### Step 3: Recover The Smallest Shared Route

- implement only the shared-BIR ownership needed for the chosen reduction
- keep explicit unsupported diagnostics for larger unowned Family B shapes

### Step 4: Revalidate The x86 Source Route

- rerun the focused x86 route test(s) plus the source-backed `00040.c` case
- refresh the broad-suite after-log and compare it against the parked baseline
