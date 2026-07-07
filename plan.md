# RV64 20000819 Runtime Mismatch After Pointer Publication

Status: Active
Source Idea: ideas/open/584_rv64_20000819_runtime_mismatch_after_pointer_publication.md

## Purpose

Diagnose and repair the downstream RV64 runtime mismatch for
`tests/c/external/gcc_torture/src/20000819-1.c` after pointer-valued add/sub
publication no longer blocks object emission.

## Goal

Make the representative stop failing with the same
`[RV64_BACKEND_RUNTIME_MISMATCH]` abort cause, or isolate a narrower downstream
owner with enough evidence for a separate source idea.

## Core Rule

Find the first semantic divergence or abort owner before changing lowering,
emission, or harness behavior. Do not treat expectation rewrites, unsupported
classification, or named-case matching as progress.

## Read First

- `ideas/open/584_rv64_20000819_runtime_mismatch_after_pointer_publication.md`
- `ideas/closed/583_rv64_pointer_arithmetic_result_publication.md`
- `build/agent_state/583_rv64_pointer_arithmetic_result_publication/step4/src_20000819-1.c/object-route.log`
- `build/agent_state/583_rv64_pointer_arithmetic_result_publication/step4/src_20000819-1.c/dump-prepared-bir.txt`

## Current Targets

- Primary representative:
  `tests/c/external/gcc_torture/src/20000819-1.c`
- Primary failure mode:
  `[RV64_BACKEND_RUNTIME_MISMATCH]` with `clang_exit=0` and
  `c4c_exit=Subprocess aborted`
- Primary route:
  prepared BIR -> RV64 object emission -> linked runtime comparison

## Non-Goals

- Do not reopen pointer-valued add/sub publication unless fresh logs prove the
  published pointer value is still semantically wrong.
- Do not mark `20000819-1.c` unsupported or weaken runtime comparison checks.
- Do not match on filename, function name, block name, value name, or exact
  diagnostic text as the repair.
- Do not broadly rewrite unrelated RV64 call, select, local-memory, or integer
  ALU lowering before the first runtime owner is identified.

## Working Model

The 583 repair advanced the route from compile-time pointer arithmetic rejection
to runtime execution. This plan starts from the runtime abort, compares clang
and C4C behavior, identifies the first concrete owner, and only then applies a
focused semantic repair or records a narrower follow-up.

## Execution Rules

- Keep investigation artifacts under
  `build/agent_state/584_rv64_20000819_runtime_mismatch_after_pointer_publication/`.
- Record packet progress and proof in `todo.md`.
- Prefer focused backend or route tests before broader validation.
- If the first owner is outside this idea's scope, stop after recording the
  owner and create a separate follow-up idea through lifecycle.

## Ordered Steps

### Step 1: Reproduce And Capture Runtime Abort

Goal: Reproduce the current runtime mismatch with fresh logs and preserve the
exact route context.

Actions:

- Build the required frontend/backend target for the route.
- Rerun the RV64 object/runtime comparison for
  `tests/c/external/gcc_torture/src/20000819-1.c`.
- Save merged command output, return codes, object/link/run artifacts when
  available, and prepared BIR dumps under the Step 1 agent-state directory.
- Confirm the old `unsupported_pointer_arithmetic` owner is not present.

Completion Check:

- `todo.md` names the current runtime failure mode, command return codes, and
  artifact paths.

### Step 2: Isolate First Divergence Or Abort Owner

Goal: Identify the first concrete semantic owner responsible for the abort or
runtime mismatch.

Actions:

- Compare clang and C4C execution behavior for the representative at the
  smallest practical level available in this repo.
- Inspect generated RV64 object output, relevant prepared BIR, and runtime
  harness evidence.
- Determine whether the first owner is lowering, object emission, runtime
  harness, prepared-data publication, or a narrower downstream capability.
- Avoid speculative broad fixes until the owner is named.

Completion Check:

- `todo.md` records the first owner with enough function/block/instruction,
  symbol, register, memory, or harness context for an executor to repair or
  split it.

### Step 3: Add Focused Coverage For The Owner

Goal: Lock the identified behavior with a focused test that is not only the
  representative.

Actions:

- Add or adjust the narrowest relevant backend/runtime test for the isolated
  owner.
- Keep the test semantic: it should prove the lowering, emission, runtime, or
  prepared-data behavior directly.
- Preserve the representative as route proof, not as the only assertion.

Completion Check:

- The focused test fails before the repair or captures the missing behavior
  clearly enough to guard the fix.

### Step 4: Repair The Runtime Cause

Goal: Implement the focused semantic repair for the isolated owner.

Actions:

- Modify only the responsible lowering, object emission, runtime harness, or
  prepared-data publication surface.
- Keep the repair general for the semantic shape; do not special-case
  `20000819-1.c` or its internal names.
- Run the focused test and representative route proof.

Completion Check:

- Focused proof passes.
- The representative no longer fails with the same runtime abort cause.

### Step 5: Backend Validation And Closure Decision

Goal: Prove the repair did not regress the relevant backend surface and decide
whether the source idea is complete.

Actions:

- Run the supervisor-selected backend validation subset, normally
  `ctest --test-dir build -j --output-on-failure -R '^backend_'` after a fresh
  build unless the supervisor selects a stronger command.
- Record validation in `test_after.log` and summarize it in `todo.md`.
- If the representative advances to a different separately scoped owner, record
  it for lifecycle split instead of stretching this plan.

Completion Check:

- Backend validation passes, and `todo.md` states whether the source idea is
  ready to close or needs a narrower follow-up.
