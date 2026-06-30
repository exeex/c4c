# RV64 Call-Adjacent Scalar And Inline-Asm Materialization Plan

Status: Active
Source Idea: ideas/open/428_rv64_call_adjacent_scalar_inline_asm_materialization.md

## Purpose

Turn the 38 call-adjacent `unsupported_instruction_fragment` rows into
ordinary scalar RV64 object-lowering work without pulling in aggregate ABI or
F128 helper-call behavior.

Goal: materialize coherent scalar call results, scalar call arguments, and
scalar inline-asm fragments through prepared call facts and object homes.

## Core Rule

Use prepared call metadata as authority. Do not infer missing call homes,
argument bindings, return homes, inline-asm operands, or aggregate/F128
semantics from source shape, representative filenames, or raw BIR alone.

## Read First

- `ideas/open/428_rv64_call_adjacent_scalar_inline_asm_materialization.md`
- `docs/rv64_gcc_torture_post_contract/failure_bucket_map.md`
- `docs/rv64_gcc_torture_post_contract/followup_idea_plan.md`
- `src/backend/mir/riscv/codegen/prepared_call_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_function_emit.cpp`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`

## Current Targets

- Prepared scalar call-result publication.
- Prepared scalar call-argument publication.
- Prepared scalar inline-asm input/output materialization.
- Representative evidence rows: `src/pr38533.c`, `src/pr40657.c`,
  `src/pr45695.c`, `src/pr49279.c`, and `src/pr56982.c`.

## Non-Goals

- Aggregate `sret` or `byval` call-storage lowering.
- F128 helper calls, long-double ABI, or runtime support.
- Reconstructing missing prepared call metadata in the RV64 lowering layer.
- Pointer/address publication, frame-slot address publication, or global
  memory work unless it is already represented as a coherent scalar prepared
  call fact.
- Expectation, allowlist, unsupported-marker, runtime-comparison, or pass/fail
  accounting changes.

## Working Model

- `prepared_call_emit.cpp` owns RV64 prepared call emission around direct calls,
  call boundary effects, immediate call arguments, and byval aggregate address
  arguments.
- `prepared_function_emit.cpp` already recognizes `llvm.inline_asm` and routes
  prepared inline-asm text emission through the prepared inline-asm carrier
  surface.
- `object_emission.cpp` owns encoded fragments and scalar publication helpers.
- The implementation route should extend general scalar publication where
  prepared homes are coherent, while preserving fail-closed diagnostics for
  unsupported constraints, missing homes, non-scalar storage, aggregate ABI,
  and F128.

## Execution Rules

- Keep each code step semantic and type/home driven; do not key behavior to
  `pr38533.c`, `pr40657.c`, or any other named testcase.
- Add focused backend tests for every newly supported scalar call-adjacent
  shape.
- Preserve existing diagnostics for missing prepared authority; improve them
  only if that helps distinguish producer gaps from RV64 lowering gaps.
- After each code step, run the supervisor-delegated proof command, expected
  to be:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1
```

- Also run `git diff --check` before returning a packet.

## Step 1: Inspect Call-Adjacent Evidence And Choose First Packet

Goal: identify the first narrow scalar call-adjacent implementation packet.

Actions:

- Inspect the representative rows and any regenerated row TSV/log artifacts
  for the 38 call-adjacent scalar/inline-asm failures.
- Classify rows into scalar call result, scalar call argument, scalar
  inline-asm, aggregate ABI, F128, and missing-prepared-fact buckets.
- Pick the first packet that has coherent prepared call facts and a focused
  backend test shape.
- Record the chosen packet, rejection point, proof command, and out-of-scope
  rows in `todo.md`.

Completion check:

- `todo.md` names the first implementation target, its primary files, and the
  exact proof command.
- No implementation files are changed in this step.

## Step 2: Lower Prepared Scalar Call Results

Goal: publish ordinary scalar call results when prepared result placement and
destination homes are coherent.

Primary targets:

- `src/backend/mir/riscv/codegen/prepared_call_emit.cpp`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- focused tests under `tests/backend/mir/`

Actions:

- Follow existing prepared call-boundary planning rather than creating a
  separate call-result route.
- Add or extend scalar publication helpers for integer and pointer-sized
  scalar call results whose source ABI register and destination object home are
  both prepared.
- Keep aggregate, F128, missing home, and mismatched plan cases fail-closed.
- Add focused tests for register-to-stack and, if already represented by
  prepared facts, register-to-register scalar result publication.

Completion check:

- Focused backend tests prove scalar call-result publication.
- The backend subset proof passes and is saved in `test_after.log`.
- No aggregate ABI or F128 row is accepted through this route.

## Step 3: Lower Prepared Scalar Call Arguments

Goal: publish ordinary scalar call arguments through prepared argument
placement and homes.

Primary targets:

- `src/backend/mir/riscv/codegen/prepared_call_emit.cpp`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- focused tests under `tests/backend/mir/`

Actions:

- Materialize scalar argument movement only when the argument ABI binding,
  source value, and required object/register home are present and coherent.
- Reuse existing immediate-call-argument handling where it already owns the
  shape; do not duplicate that path.
- Preserve the existing aggregate `byval` route and do not reinterpret byval
  objects as scalar call arguments.
- Add focused tests for scalar register and stack argument publication that
  exercise prepared facts rather than filename-specific cases.

Completion check:

- Focused backend tests prove scalar call-argument publication.
- The backend subset proof passes and is saved in `test_after.log`.
- Missing bindings and aggregate/F128 arguments still reject cleanly.

## Step 4: Lower Prepared Scalar Inline-Asm Fragments

Goal: materialize scalar inline-asm inputs and outputs through the same
prepared call-adjacent authority.

Primary targets:

- `src/backend/mir/riscv/codegen/prepared_function_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_call_emit.cpp`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- focused tests under `tests/backend/mir/`

Actions:

- Use prepared inline-asm carriers and operand metadata as authority for
  operand substitution and scalar publication.
- Support only coherent scalar GPR-style inline-asm inputs and outputs in this
  plan.
- Reject unsupported constraints, missing carriers, malformed operands,
  aggregate operands, and F128 operands.
- Add tests that prove a general scalar inline-asm rule, including at least one
  shape related to `%t0 = bir.call i32 llvm.inline_asm(i32 0)` without
  keying to `src/pr38533.c`.

Completion check:

- Focused backend tests prove scalar inline-asm materialization.
- The backend subset proof passes and is saved in `test_after.log`.
- Unsupported inline-asm constraints and non-scalar rows remain fail-closed.

## Step 5: Broader Validation And Close Readiness

Goal: decide whether the source idea is complete or whether remaining
call-adjacent rows need a new follow-up idea.

Actions:

- Re-run representative RV64 object-route probes for the call-adjacent rows
  chosen in Step 1.
- Classify any remaining failures as in-scope missing scalar call/inline-asm
  work or out-of-scope aggregate ABI, F128, pointer/address, global memory, or
  missing-prepared-fact work.
- Run the supervisor-delegated backend proof and `git diff --check`.
- Summarize acceptance coverage, rejected row classes, and close readiness in
  `todo.md`.

Completion check:

- `todo.md` records whether the source idea appears ready for lifecycle close
  review.
- No expectation, allowlist, unsupported-marker, runtime-comparison, or
  pass/fail accounting files are changed or used as progress.
