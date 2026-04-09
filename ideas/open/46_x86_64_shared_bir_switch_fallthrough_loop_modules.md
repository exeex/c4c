# x86_64 Shared BIR Switch Fallthrough Loop Modules

Status: Open
Last Updated: 2026-04-09

## Why This Idea

Idea 44's Step 2 family-recovery lane originally kept `00143.c` grouped with
the earlier alloca-backed local-slot/control-flow survivors. Re-checking the
current tree after the `00057.c`, `00086.c`, and `00138.c` slices showed that
`00143.c` is materially different:

- it enters the hot path through a `switch`
- each case falls through a repeated copy chain
- the `case 1` tail closes with a do-while backedge
- the copy loop is followed by a separate verification loop over local arrays

That is no longer the same ownership point as the constant/immediate folds
already recovered in idea 44.

## Source Context

- `ideas/open/44_x86_64_backend_bringup_after_bir_cutover.md`
- `tests/c/external/c-testsuite/src/00143.c`
- current-tree repro:
  `./build/c4cll --target x86_64-unknown-linux-gnu --codegen asm tests/c/external/c-testsuite/src/00143.c`

## Current Failure Shape

Representative failing case:

- `c_testsuite_x86_backend_src_00143_c`

Current x86 backend result on 2026-04-09:

- `--codegen asm` fails with
  `x86 backend emitter does not support this direct LIR module; only direct-LIR slices that lower natively or through direct BIR are currently supported`

Representative LIR structure from the current tree:

- local arrays `a[39]` and `b[39]`
- local pointers `from` and `to`
- initialization loop over both arrays
- `switch (count % 8)` that jumps into a fallthrough sequence of copy blocks
- a do-while decrement/backedge over `n`
- a second loop that verifies `a[n] == b[n]`

## Goal

Recover the smallest shared `lir_to_bir` seam that can own switch-entry
fallthrough loop modules like `00143.c` on x86_64 without reviving any deleted
legacy fallback path.

## Non-Goals

- do not widen this into the global-rich Family B lane from idea 44
- do not revive legacy backend IR or LLVM asm rescue
- do not silently absorb unrelated multi-function or libc-heavy cases

## Proposed Plan

### Step 1: Add A Narrow Route Regression

- add an internal x86 backend route regression around `00143.c`
- keep the assertion narrow: native asm output must be produced and LLVM text
  fallback must stay forbidden

### Step 2: Capture The Minimal Owning Contract

- identify the smallest shared representation that can own:
  - switch-entry control flow
  - structured fallthrough copy chains
  - the bounded do-while backedge
- decide whether the verification loop can stay in the same slice or should be
  split again

### Step 3: Land The Smallest Shared Lowering Expansion

- expand `lir_to_bir` only enough to cover the chosen contract
- add focused lowering and x86 backend pipeline coverage
- preserve explicit unsupported diagnostics for larger switch/loop shapes that
  still remain unowned

## Acceptance Criteria

- `c_testsuite_x86_backend_src_00143_c` no longer stops at the unsupported
  direct-LIR boundary on x86_64
- the new shared seam is pinned by targeted regression coverage
- no legacy fallback behavior is restored
