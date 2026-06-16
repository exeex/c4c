# Plan: AArch64 LLVM-path fp128/vararg codegen crash triage

Status: Active
Source Idea: ideas/open/285_aarch64_llvm_path_fp128_vararg_codegen_crash_triage.md

## Purpose

Triage and repair, or isolate behind an explicit support boundary, the
AArch64 LLVM-path IR that currently crashes Debian clang 19 during object
generation for fp128/long-double and vararg-heavy C torture cases.

## Goal

Classify each known crashing AArch64 LLVM-path case as repaired, reduced to a
concrete C4CLL IR bug, or parked behind a documented external-backend support
boundary with focused evidence.

## Core Rule

Do not mask clang backend crashes through skips, stderr suppression, missing
binary handling, or named-test rewrites. Prefer reduced evidence and semantic
IR-generation repair.

## Read First

- `ideas/open/285_aarch64_llvm_path_fp128_vararg_codegen_crash_triage.md`
- The three crashing C torture sources:
  - `c_testsuite_src_00204_c`
  - `llvm_gcc_c_torture_src_920625_1_c`
  - `llvm_gcc_c_torture_src_pr44575_c`
- The LLVM-path C lowering, vararg, long-double/fp128, and AArch64 target
  code involved in producing the crashing IR.
- The runtime test harness path that reports clang IR compile failures as
  `[BACKEND_FAIL]`.

## Current Targets

- `c_testsuite_src_00204_c`: clang 19 crashes in AArch64 instruction
  selection for `@myprintf` with fp128/long-double HFA or vararg-heavy IR.
- `llvm_gcc_c_torture_src_920625_1_c`: clang 19 crashes in `RegBankSelect`
  for `@va1`.
- `llvm_gcc_c_torture_src_pr44575_c`: clang 19 crashes in `RegBankSelect`
  for `@check`.

## Non-Goals

- Do not downgrade the tests to unsupported without explicit reduced evidence
  and supervisor approval.
- Do not suppress stderr, hide clang crashes, or reinterpret missing c2ll
  binaries as runtime no-file failures.
- Do not fold in C++ dependent casts or C aggregate function-pointer ABI work.
- Do not start broad AArch64 ABI rewrites unless reduction proves they are the
  direct cause.
- Do not add filename, symbol, or known-case matching for the three failures.

## Working Model

The c2ll binary is never produced for these failures. C4CLL emits IR that
`clang -x ir` accepts far enough to enter the AArch64 backend, then Debian
clang 19 crashes during object generation. The shared region may be invalid
LLVM IR, target-incompatible IR for AArch64, or avoidably hostile IR around
fp128/long-double, HFA, and vararg lowering.

## Execution Rules

- Reduce before repairing: first identify whether the three cases share one
  unsupported construct or split into separate failure families.
- Preserve harness visibility: clang IR compile failures must continue to
  report `[BACKEND_FAIL]` with useful stderr.
- Prefer semantic IR generation fixes over target-specific expectation churn.
- Any external-backend boundary must be narrow, evidence-backed, and explicit.
- For code-changing steps, run fresh build proof before focused CTest proof.
- Treat expectation downgrades, unsupported broadening, and stderr suppression
  as blockers, not progress.

## Ordered Steps

### Step 1: Reproduce And Reduce The Three Crash Families

Goal: capture focused evidence for each crashing case and determine whether
they share one unsupported IR construct or form separate failure families.

Primary targets:

- `c_testsuite_src_00204_c`
- `llvm_gcc_c_torture_src_920625_1_c`
- `llvm_gcc_c_torture_src_pr44575_c`
- generated LLVM IR and clang 19 stderr for each target

Actions:

- Reproduce each focused CTest target on the AArch64 LLVM path.
- Save or quote the relevant crashing IR shape, function name, and clang
  backend crash point in `todo.md`.
- Reduce the IR enough to identify the construct that triggers each backend
  crash.
- Decide whether the cases share a common fp128/long-double vararg/HFA issue
  or need separate repair/boundary paths.

Completion check:

- `todo.md` records the focused proof command, crash evidence, reduced
  construct, and initial family classification for all three targets.

### Step 2: Classify IR Validity And Ownership

Goal: decide whether each reduced crash is caused by invalid C4CLL IR,
AArch64-target-incompatible IR, or an external clang 19 backend limitation.

Primary targets:

- LLVM verifier or clang IR diagnostics available before backend codegen.
- C4CLL lowering code that emits the reduced fp128/long-double, HFA, and
  vararg IR constructs.
- Any existing target support boundary or harness classification mechanism.

Actions:

- Check whether the reduced IR violates LLVM IR rules or target ABI
  expectations.
- Identify the C4CLL lowering helper that owns each avoidable or invalid IR
  shape.
- If the evidence points to a clang backend limitation, define the narrowest
  support boundary and the exact condition that triggers it.
- Record the classification and proposed next action for each target in
  `todo.md`.

Completion check:

- Each target has a documented owner: C4CLL repair surface, target support
  boundary, or explicitly unresolved blocker with evidence.

### Step 3: Repair Avoidable Or Invalid IR Shapes

Goal: repair C4CLL IR generation for any reduced construct that is invalid or
avoidably hostile while preserving nearby supported behavior.

Primary targets:

- The lowering helper or helpers identified in Step 2.
- Existing fp128/long-double, vararg, HFA, and AArch64 ABI utilities.

Actions:

- Reuse structured type, ABI, and target metadata rather than textual IR or
  testcase matching.
- Adjust only the semantic lowering path required by the reduced evidence.
- Keep harness behavior unchanged and keep clang backend stderr visible.
- Run a fresh build before focused CTest proof.

Completion check:

- Any repaired target advances past clang object generation or has a different
  documented failure.
- Focused CTest proof for the repaired target is recorded in `todo.md`.
- No test expectation, unsupported classification, or stderr behavior was
  weakened.

### Step 4: Add Or Confirm A Narrow External-Backend Boundary

Goal: when reduction proves a clang 19 AArch64 backend limitation rather than
a C4CLL IR bug, represent that limitation explicitly and narrowly.

Actions:

- Encode the boundary only for the reduced unsupported construct, not for all
  fp128, long-double, vararg, or AArch64 tests.
- Preserve `[BACKEND_FAIL]` reporting for ordinary clang IR compile failures.
- Document the evidence that distinguishes an external backend limitation from
  invalid C4CLL IR.
- Get supervisor approval before treating unsupported classification as
  acceptance progress.

Completion check:

- Any parked case has a documented, narrow support boundary with reduction
  evidence.
- The boundary does not hide unrelated clang crashes or missing binaries.

### Step 5: Prove Focused And Harness Behavior

Goal: verify repaired or explicitly bounded outcomes for the three source
targets and preserve backend failure reporting.

Actions:

- Run focused CTest commands for all three targets.
- Run any nearby fp128/long-double, vararg, HFA, and AArch64 LLVM-path tests
  selected by the supervisor.
- Confirm harness output still reports clang IR compile failures as
  `[BACKEND_FAIL]` with useful stderr.
- Record commands, results, residual risks, and any external-boundary evidence
  in `todo.md`.

Completion check:

- Each source target is repaired, reduced to a concrete C4CLL IR bug for a
  follow-up route, or parked behind an approved documented external-backend
  support boundary.
- No reviewer reject signal from the source idea applies.
- The supervisor has enough proof to decide broader validation and closure.
