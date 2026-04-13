# X86 Backend Failure Recovery

Status: Active
Source Idea: ideas/open/45_fix_x86_backend_fails.md
Activated from: fresh activation on 2026-04-13; route checkpoint rewritten on 2026-04-13 after deleting the x86 `try_emit_*` / `direct_dispatch` special-case route and resetting the runbook onto the translated backend path

## Purpose

Continue the broad x86 backend recovery without relying on testcase-shaped
`try_emit_*` helpers, direct-dispatch ladders, or hand-written asm templates
that only make narrow c-testsuite families go green.

## Goal

Re-establish a truthful minimal x86 execution path through shared lowering and
the translated x86 owners so that small modules no longer immediately fail with
the unsupported direct-LIR diagnostic after the special-case route removal.

## Core Rule

Do not reintroduce `try_emit_minimal_*`, `try_emit_direct_*`,
`direct_dispatch.cpp`, or equivalent testcase-shaped routing under a different
name. All recovery work in this runbook must land in general lowering or the
translated x86 owners.

## Read First

- `ideas/open/45_fix_x86_backend_fails.md`
- `src/backend/backend.cpp`
- `src/backend/lowering/lir_to_bir.cpp`
- `src/backend/x86/codegen/mod.cpp`
- `src/backend/x86/codegen/calls.cpp`
- `src/backend/x86/codegen/globals.cpp`
- `src/backend/x86/codegen/returns.cpp`
- `src/backend/x86/codegen/shared_call_support.cpp`
- `src/backend/x86/codegen/x86_codegen.hpp`
- `tests/c/external/c-testsuite/src/00030.c`
- `tests/c/external/c-testsuite/src/00031.c`
- `tests/c/external/c-testsuite/src/00094.c`

## Current Targets

- `src/backend/backend.cpp`
- `src/backend/lowering/lir_to_bir.cpp`
- `src/backend/x86/codegen/mod.cpp`
- `src/backend/x86/codegen/calls.cpp`
- `src/backend/x86/codegen/globals.cpp`
- `src/backend/x86/codegen/returns.cpp`
- `src/backend/x86/codegen/shared_call_support.cpp`
- `src/backend/x86/codegen/x86_codegen.hpp`

## Non-Goals

- do not recreate any deleted `src/backend/x86/codegen/direct_*.cpp` unit
- do not add text-shape matching, printed-IR probing, or named-case-only x86
  emitter shortcuts
- do not claim old `00030.c` through `00094.c` packets remain accepted after
  the route reset; they must be reproved on the translated path
- do not widen into broad x86 backlog closure before the minimal translated
  baseline is back

## Working Model

- the deleted special-case route previously masked real x86 backend gaps with
  testcase-shaped native emitters
- after removing that route, the smallest observed x86 failures now collapse to
  the same unsupported direct-LIR diagnostic
- the first honest recovery step is to restore reachability for a tiny shared
  lane through general lowering and translated x86 codegen, then expand from
  there
- `00030.c`, `00031.c`, and `00094.c` currently fail with the same x86
  unsupported direct-LIR error and are the right proving subset for the reset

## Ordered Steps

### Step 1: Re-establish a translated minimal x86 lane

Goal: make the smallest reset subset reach a truthful translated x86 path
again.

Primary target:

- `src/backend/backend.cpp`
- `src/backend/lowering/lir_to_bir.cpp`
- `src/backend/x86/codegen/mod.cpp`
- `src/backend/x86/codegen/calls.cpp`
- `src/backend/x86/codegen/globals.cpp`
- `src/backend/x86/codegen/returns.cpp`
- `src/backend/x86/codegen/shared_call_support.cpp`
- `src/backend/x86/codegen/x86_codegen.hpp`

Concrete actions:

- inspect why tiny x86 modules still stop at the unsupported direct-LIR
  diagnostic after the special-case route deletion
- restore a general path through shared lowering or translated x86 owners
  instead of reintroducing route-specific emitters
- keep the first repair bounded to the common failure mode exercised by
  `00030.c`, `00031.c`, and `00094.c`

Completion check:

- `c_testsuite_x86_backend_src_00030_c` passes
- `c_testsuite_x86_backend_src_00031_c` passes
- `c_testsuite_x86_backend_src_00094_c` passes

### Step 2: Reprove the previously claimed x86 band on the translated route

Goal: revalidate the nearby x86 band that used to be covered by deleted
special-case packets.

Primary target:

- `todo.md`

Concrete actions:

- keep the next adjacent x86 family explicit after the reset subset lands
- only mark a formerly accepted family as recovered after it passes again on
  the translated route
- record remaining regressions in `todo.md` instead of re-encoding them in
  `plan.md` packet by packet

Completion check:

- the next reproving band after `00030.c` / `00031.c` / `00094.c` is explicit
- no deleted special-case family is still described as accepted without fresh
  proof

### Step 3: Validate the reset route honestly

Goal: keep proof aligned to the new translated route.

Primary target:

- `build/`

Concrete actions:

- run `cmake --build build -j2`
- run `ctest --test-dir build -j --output-on-failure -R '^(c_testsuite_x86_backend_src_00030_c|c_testsuite_x86_backend_src_00031_c|c_testsuite_x86_backend_src_00094_c)$'`
- escalate only after the reset subset passes on the translated path

Completion check:

- fresh build succeeds
- the focused reset subset passes without any x86 special-case route
