# Residual Scalar F32/F64 Cast Object Lowering Runbook

Status: Active
Source Idea: ideas/open/517_residual_scalar_f32_f64_cast_object_lowering.md
Activated From: post-425 scalar FPR salvage follow-up

## Purpose

Implement or precisely reject the residual scalar F32/F64 cast object-lowering
boundary identified by idea 425, without replaying `try_gcc_torture` or
absorbing F128/helper ABI work.

## Goal

Lower supported scalar F32/F64 cast directions from explicit prepared facts, or
split/reject rows whose first owner is missing prepared/BIR fact publication,
for the three fresh `unsupported_floating_cast` representatives:
`src/cvt-1.c`, `src/920618-1.c`, and `src/pr66233.c`.

## Core Rule

RV64 object emission may consume only explicit prepared cast facts: cast
opcode, source type, result type, source and destination banks, value homes,
and materialization authority. Do not infer missing bank, type, home, opcode,
or conversion authority from testcase names, raw BIR shapes, `conversion.c`,
or old `try_gcc_torture` behavior.

## Read First

- ideas/open/517_residual_scalar_f32_f64_cast_object_lowering.md
- ideas/closed/425_scalar_fpr_salvage_from_try_gcc_torture.md
- build/agent_state/rv64_gcc_c_torture_backend_summary.full.tsv
- build/rv64_gcc_c_torture_backend/src_cvt-1.c/case.log
- build/rv64_gcc_c_torture_backend/src_920618-1.c/case.log
- build/rv64_gcc_c_torture_backend/src_pr66233.c/case.log
- src/backend/mir/riscv/codegen/object_emission.cpp
- tests/backend/mir/backend_riscv_object_emission_test.cpp

## Current Targets

- Fresh rows:
  - `tests/c/external/gcc_torture/src/cvt-1.c`
  - `tests/c/external/gcc_torture/src/920618-1.c`
  - `tests/c/external/gcc_torture/src/pr66233.c`
- Current bucket: `unsupported_floating_cast`
- Existing accepted paths to preserve:
  - prepared FPR width casts
  - I32/I64-to-F32/F64 integer-to-floating casts
- Branch evidence to mine, not cherry-pick:
  - `9d0f64883`, `b0700c2c3`, `88076c4ec`, `18c41c9c3`,
    `92d77cf2c`, `8bbaf8eb7`, `f95ec37b9`

## Non-Goals

- Do not implement or admit F128/long-double casts, helper calls, helper ABI,
  F128 lane pairs, F128 local-memory work, or `conversion.c` completion.
- Do not repair aggregate/byval, stack-frame, parameter-home, local-memory,
  branch/select, call/return, local-store/reload, or scalar FPR binary
  arithmetic in this route.
- Do not cherry-pick broad `try_gcc_torture` behavior.
- Do not change expectations, allowlists, unsupported markers, runtime
  comparison, scan accounting, or default CTest contracts.
- Do not key behavior to `src/cvt-1.c`, `src/920618-1.c`, `src/pr66233.c`, or
  old testcase names.

## Working Model

- Idea 425 accepted exactly one scalar FPR salvage boundary: residual scalar
  F32/F64 cast object lowering.
- The current RV64 object route reports `unsupported_floating_cast` when a
  scalar cast direction is outside existing prepared FPR width casts and
  I32/I64-to-F32/F64 integer-to-floating paths.
- The first implementation task is not to add lowering blindly; it is to prove
  the three rows carry explicit prepared scalar-cast facts. Missing producer
  facts must become a separate producer idea instead of RV64 inference.

## Execution Rules

- Start by preserving and summarizing the exact row diagnostics and prepared
  facts in `todo.md`.
- Split before implementation if any representative lacks explicit prepared
  cast facts or depends on a quarantined owner.
- Keep accepted lowering semantic by cast direction and prepared facts, not by
  testcase.
- Add focused backend object-emission fixtures for accepted and rejected cast
  directions before treating representative probes as sufficient.
- For code-changing steps, run a fresh build, focused backend proof,
  representative probes, and `ctest --test-dir build -j --output-on-failure -R
  '^backend_'`.

## Step 1: Rebuild Residual Cast Row Evidence

Goal: Determine the exact first owner for each fresh `unsupported_floating_cast`
row before any implementation.

Actions:

- Inspect `build/rv64_gcc_c_torture_backend/src_cvt-1.c/case.log`,
  `build/rv64_gcc_c_torture_backend/src_920618-1.c/case.log`, and
  `build/rv64_gcc_c_torture_backend/src_pr66233.c/case.log`.
- Reproduce focused dumps or probes if needed for the three representatives.
- Record each row's cast opcode, source scalar type, destination scalar type,
  source bank, destination bank, source home, destination home,
  immediate/materialization needs, and current rejection site.
- Classify each row as RV64 object-lowering-owned, producer/prepared-fact
  owned, or quarantined/out-of-scope.

Completion check:

- `todo.md` records the exact row evidence and whether a single RV64 scalar
  cast lowering contract can cover the accepted rows.

## Step 2: Define The Scalar Cast Contract

Goal: Define the minimal accepted scalar F32/F64 cast predicate and adjacent
fail-closed variants.

Actions:

- Trace existing prepared FPR width cast and I32/I64-to-F32/F64 lowering.
- Define supported scalar cast directions only where prepared facts are
  explicit and coherent.
- Define diagnostics for missing cast opcode, missing source/destination bank,
  missing value home, unsupported scalar type, unsupported direction, F128,
  helper-dependent casts, and contradictory facts.
- If any row's first owner is producer publication, record the split boundary
  instead of adding RV64 inference.

Completion check:

- `todo.md` names the accepted/rejected semantic boundary and any source idea
  split required before implementation.

## Step 3: Implement Or Split The Accepted Boundary

Goal: Add the narrow semantic RV64 lowering or create the producer split when
prepared facts are insufficient.

Actions:

- If Step 2 proves coherent RV64 ownership, add the scalar cast lowering path
  in the RV64 object-emission surface.
- If Step 2 proves missing producer authority, create a separate producer idea
  and stop widening RV64 object emission.
- Preserve existing accepted casts and all quarantined/out-of-scope behavior.
- Keep owner-specific unsupported diagnostics for unsupported directions.

Completion check:

- Fresh build passes.
- The old `unsupported_floating_cast` gate is advanced for accepted rows or
  replaced by a more precise owner-specific rejection.
- `todo.md` records implemented behavior or the created producer split.

## Step 4: Add Focused Backend Coverage

Goal: Prove the cast behavior with semantic fixtures instead of named
gcc_torture rows.

Actions:

- Add focused object-emission tests for each accepted scalar cast direction.
- Add fail-closed coverage for unsupported scalar FP cast directions,
  F128/helper-dependent shapes, and missing or contradictory prepared facts
  where practical.
- Preserve existing tests for FPR width casts and I32/I64-to-F32/F64 casts.
- Keep assertions tied to prepared facts and diagnostics.

Completion check:

- Focused backend tests pass.
- Coverage proves adjacent unsupported directions remain fail-closed.
- `git diff --check` passes for touched files.

## Step 5: Validate Representatives And Backend Subset

Goal: Validate the implementation or split decision against focused row probes
and the backend subset.

Actions:

- Run a fresh build.
- Probe `src/cvt-1.c`, `src/920618-1.c`, and `src/pr66233.c`.
- Run focused backend object-emission tests.
- Run `ctest --test-dir build -j --output-on-failure -R '^backend_'`.
- Update `todo.md` with proof and residual risks.

Completion check:

- Fresh validation is recorded in `todo.md`.
- The runbook can be closed, or remaining work is explicitly separated from
  residual scalar F32/F64 cast object lowering.
