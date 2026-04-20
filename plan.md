# X86 Backend Runtime Correctness Regressions

Status: Active
Source Idea: ideas/open/63_x86_backend_runtime_correctness_regressions.md
Supersedes: ideas/open/61_call_bundle_and_multi_function_prepared_module_consumption.md

## Purpose

Turn idea 63 into an execution runbook for x86 backend cases that already
compile and emit asm but still crash or return the wrong result at runtime.

## Goal

Repair the active runtime-correctness failures, starting with the newly
graduated `c_testsuite_x86_backend_src_00040_c`, without hiding wrong-code or
runtime crashes behind unsupported-capability fallbacks.

## Core Rule

Do not route runtime failures back into unsupported or emitter-rejection
buckets. Fix the concrete correctness seam that produces bad asm or bad
execution, then keep the proof on real running cases.

## Read First

- `ideas/open/63_x86_backend_runtime_correctness_regressions.md`
- `ideas/open/61_call_bundle_and_multi_function_prepared_module_consumption.md`
- `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`
- `src/backend/mir/x86/codegen/x86_codegen.hpp`
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/regalloc.cpp`
- `tests/c/external/c-testsuite/src/00040.c`
- `build/c_testsuite_x86_backend/src/00040.c.s`

## Scope

- runtime crashes after the backend already emits asm
- wrong-result executions after the backend already emits asm
- prepared pointer, local-slot, addressing, or call/result rendering defects
  that manifest as runtime failure
- proof on owned runtime cases plus nearby backend coverage when a packet
  changes shared x86/prepared logic

## Non-Goals

- reopening idea-61 ownership for prepared-module traversal or authoritative
  call-bundle publication once those gates are crossed
- downgrading runtime failures into unsupported diagnostics
- broad semantic-lowering or frontend work unrelated to the emitted runtime bug
- hiding wrong-code with testcase-specific x86 shortcuts

## Working Model

- `c_testsuite_x86_backend_src_00040_c` now passes the old prepared-module and
  call-bundle gates and fails at runtime with a segmentation fault
- the current emitted `chk` asm repeatedly dereferences
  `QWORD PTR [rsp + 1560]` instead of using the initialized global-backed `t`
- the most likely first seam is prepared pointer/local-slot rendering in
  `prepared_local_slot_render.cpp`, not a return to local ABI inference
- other runtime cases may reveal different seams, so keep each packet bounded
  to one correctness root cause at a time

## Execution Rules

- prefer one runtime root cause per packet
- update `todo.md`, not this file, for routine packet progress
- use `build -> narrow runtime proof` for every accepted code slice
- when a packet touches shared prepared or x86 helpers, keep at least one real
  runtime case in proof and add nearby backend coverage if it protects the seam
- reject slices that convert runtime failures into unsupported behavior or
  testcase-shaped special cases

## Step 1: Audit Runtime Failure Surface

Goal: confirm the first active runtime failure and isolate the smallest codegen
or prepared-contract seam responsible for it.

Primary targets:

- `tests/c/external/c-testsuite/src/00040.c`
- `build/c_testsuite_x86_backend/src/00040.c.s`
- `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`

Actions:

- confirm the current `00040` runtime symptom from the emitted asm and runtime
  proof log
- map the bad pointer/local-slot behavior back to the prepared addressing or
  value-home path that produced it
- decide whether the first repair belongs in x86 rendering, shared prepared
  contract publication, or another runtime-owned seam

Completion check:

- the first implementation packet is narrowed to one concrete runtime seam with
  proof targets that include `00040` and any nearest protective backend test

## Step 2: Repair The First Runtime Seam

Goal: fix the concrete emitted-asm or prepared-consumption bug behind the
active runtime failure without reopening unsupported-capability work.

Primary targets:

- `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`
- shared prepared helpers only if the runtime bug exposes a real missing fact

Actions:

- repair the pointer/local-slot or related runtime logic identified in Step 1
- keep shared contract changes target-independent when they express durable
  prepared meaning
- avoid x86-only bounded-case fallbacks that merely hide the symptom

Completion check:

- `c_testsuite_x86_backend_src_00040_c` advances past the current runtime
  crash for the repaired seam, with no regression in the protective subset

## Step 3: Extend Proof Across Owned Runtime Cases

Goal: show the accepted slice improves real runtime correctness rather than one
named case.

Actions:

- require a fresh build for each accepted code slice
- prove the repaired seam with `00040` plus the nearest backend coverage that
  protects the changed rendering path
- broaden to other owned runtime cases when they plausibly share the repaired
  seam

Completion check:

- accepted slices have fresh proof and demonstrate real runtime-correctness
  progress without changing failure classification back to unsupported
