# Backend X86 C-Testsuite Backend Convergence Plan

## Status

Open as of 2026-03-29.

## Relationship To Roadmap

Umbrella source: `ideas/open/__backend_port_plan.md`

Depends on:

- `ideas/closed/31_backend_x86_runtime_case_convergence_plan.md`
- `ideas/closed/33_backend_x86_external_call_object_plan.md`
- `ideas/open/35_backend_ready_ir_contract_plan.md`
- `ideas/open/36_lir_to_backend_ready_ir_lowering_plan.md`
- `ideas/open/37_backend_emitter_backend_ir_migration_plan.md`

Should precede:

- any claim that the x86 backend is ready for broader external C corpus
  execution beyond the currently hand-picked internal runtime cases
- any activation of broader x86 assembler/linker parity work based on
  c-testsuite-style evidence

## Goal

Promote the vendored external C testsuite from "frontend plus clang backend"
coverage into explicit x86 backend-owned assembly coverage, one bounded failure
family at a time, until the new `c_testsuite_x86_backend_*` tests stop failing
because of LLVM IR fallback.

## Why This Slice Exists

The repo now has two distinct c-testsuite surfaces:

- `c_testsuite_*`:
  preserves the original behavior where `c4cll` emits LLVM IR and Clang handles
  code generation
- `c_testsuite_x86_backend_*`:
  requires `c4cll --codegen lir --target x86_64-unknown-linux-gnu` to produce
  real x86 assembly, and treats fallback-to-IR as a hard failure

Focused validation on 2026-03-29 shows:

- all 220 legacy `c_testsuite_*` cases still pass
- only 20 of 220 `c_testsuite_x86_backend_*` cases currently pass
- the remaining 200 fail with `[BACKEND_FALLBACK_IR]`, not with frontend,
  assembler, or runtime mismatches

That means the immediate problem is not broad correctness regression in the
existing frontend path. The problem is that the x86 backend still recognizes too
few real external-C lowering shapes and falls back to LLVM text for most of this
corpus.

Work executed under the previously active convergence runbook did improve the
backend-owned pass count from 20/220 to 31/220 by teaching
`src/backend/lir_adapter.cpp` additional bounded normalization cases. That
progress also made the next blocker clearer: further convergence is now coupled
too tightly to growing the minimal adapter, which is not the right long-term
ingestion layer for multi-block and memory-using external-C inputs.

This idea should therefore remain parked behind the backend-ready IR work in
`35`, `36`, and `37` rather than continuing to expand the adapter-driven path
as if it were the final backend architecture.

## Primary Scope

- `tests/c/external/c-testsuite/RunCase.cmake`
- `tests/TestEntry.cmake`
- `tests/c/external/c-testsuite/src/*.c` only as probe inputs for bounded
  backend mechanism discovery
- `src/backend/x86/codegen/emit.cpp`
- closely adjacent x86 backend adapter/emitter files only when required by the
  newly promoted testcase family

## Current Verified Baseline

As of 2026-03-29:

- `ctest --test-dir build -R '^c_testsuite_src_00002_c$' --output-on-failure`
  passes on the legacy frontend surface
- `ctest --test-dir build -R '^c_testsuite_x86_backend_src_00001_c$' --output-on-failure`
  passes on the backend-owned x86 asm surface
- `ctest --test-dir build -R '^c_testsuite_x86_backend_src_00002_c$' --output-on-failure`
  fails with `[BACKEND_FALLBACK_IR]`
- `ctest --test-dir build -L x86_backend --output-on-failure` reports 20/220
  passing and 200/220 failing, with the failing population currently dominated
  by explicit fallback-to-IR rejection

## Required Behavior

For any c-testsuite case explicitly brought into scope by this plan:

- backend output must be native x86 assembly, not LLVM IR
- fallback-to-IR must remain treated as a bug, not a tolerated alternate path
- assembled binaries must still match the existing expected-output contract used
  by the legacy `c_testsuite_*` runner

## Non-Goals

- no silent weakening of the new `c_testsuite_x86_backend_*` rule back into
  accepting LLVM IR
- no deletion or relaxation of the original `c_testsuite_*` frontend coverage
- no attempt to solve the entire 200-case failing population in one patch
- no broad x86 linker or optimizer expansion unless a bounded testcase family
  proves it is strictly required

## Suggested Execution Order

1. classify the 200 current failures into a few dominant backend mechanism
   families instead of treating them as 200 unrelated cases
2. choose the smallest high-leverage family whose cases all fail for the same
   structural reason
3. promote only that family onto the backend-owned asm path
4. add the narrowest backend adapter/emitter assertions needed to make the new
   coverage durable
5. rerun the `x86_backend` label and record the pass-count improvement before
   moving to the next family

## Validation

- keep `c_testsuite_*` green as the unchanged frontend baseline
- require promoted `c_testsuite_x86_backend_*` cases to pass without IR fallback
- use targeted `ctest --test-dir build -R '^c_testsuite_x86_backend_...'`
  checks while iterating
- rerun `ctest --test-dir build -L x86_backend --output-on-failure` to measure
  monotonic coverage improvement
- if a patch claims general improvement, also rerun
  `ctest --test-dir build -L backend --output-on-failure`

## Parked State

Execution knowledge preserved from the deactivated runbook:

- the adapter-driven path improved the `x86_backend` label from
  20 passed / 200 failed / 220 total to
  31 passed / 189 failed / 220 total
- the next excluded seed remained `tests/c/external/c-testsuite/src/00007.c`,
  which combines two countdown-loop regions with a mid-function conditional
  early return
- that next slice is no longer the preferred immediate target because it would
  require widening `src/backend/lir_adapter.cpp` further instead of establishing
  a backend-owned IR boundary first

## Good First Patch

Bucket the current 200 `[BACKEND_FALLBACK_IR]` failures by shared lowering
shape, then fully convert the smallest repeatable family rather than chasing
single cases one by one.

## Success Condition

This idea is complete only when the c-testsuite x86 backend surface is no longer
blocked primarily by backend fallback for the in-scope bounded families, and the
remaining gaps, if any, have been split into narrower follow-on ideas instead of
left as one large opaque backlog.
