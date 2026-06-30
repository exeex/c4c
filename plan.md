# RV64 Integer Div And Rem Lowering Plan

Status: Active
Source Idea: ideas/open/430_rv64_integer_div_rem_lowering.md

## Purpose

Add or validate semantic RV64 object lowering for coherent scalar integer
division and remainder fragments.

Goal: prepared scalar `udiv`, `sdiv`, `urem`, and `srem` rows with coherent
operand/result facts should no longer stop at `unsupported_instruction_fragment`.

## Core Rule

Use BIR opcode, type, signedness, and prepared operand/result homes as
authority. Do not key behavior to a divisor value, representative filename,
instruction index, or one prepared dump shape.

## Read First

- `ideas/open/430_rv64_integer_div_rem_lowering.md`
- `docs/rv64_gcc_torture_post_contract/failure_bucket_map.md`
- `docs/rv64_gcc_torture_post_contract/followup_idea_plan.md`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/prepared_function_emit.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `src/backend/mir/riscv/assembler/README.md`

## Current Targets

- Coherent scalar `udiv`, `sdiv`, `urem`, and `srem` BIR operations.
- RV64-supported integer widths only, primarily `i32` and `i64`.
- Representative rows: `src/20021120-3.c`, `src/20030105-1.c`,
  `src/20090113-2.c`, `src/20090113-3.c`, and `src/20100416-1.c`.

## Non-Goals

- Constant-divisor strength reduction or divisor-specific shortcuts.
- Floating-point division, F128 helper work, I128 helper policy, or runtime
  helper expansion.
- Pointer casts, select/join, call publication, aggregate ABI, arithmetic
  shift lowering, global memory/addressing residuals, or broad ALU rewrites.
- Expectation, allowlist, unsupported-marker, runtime-comparison, or pass/fail
  accounting changes.

## Working Model

- RV64 has M-extension integer `div`, `divu`, `rem`, and `remu` operations,
  plus `w` forms for 32-bit results.
- `prepared_function_emit.cpp` routes supported BIR binary instructions
  through the RV64 prepared binary emission path.
- `object_emission.cpp` owns encoded scalar binary fragments and should map
  signedness and width to the correct RV64 instruction only when prepared homes
  are coherent.
- Current code and tests already contain nearby scalar binary infrastructure;
  Step 1 must determine whether the remaining work is missing capability,
  missing representative coverage, or stale lifecycle state.

## Execution Rules

- Keep every implementation packet semantic and opcode/type driven.
- Add or preserve focused backend tests for signed division, unsigned
  division, signed remainder, and unsigned remainder across supported widths.
- Preserve fail-closed diagnostics for unsupported widths, missing homes,
  non-integer operands, F128/I128 helper cases, and incoherent prepared facts.
- If representative rows still fail for select, global memory, call,
  aggregate, pointer, or producer reasons after div/rem is supported, record
  those classes in `todo.md` instead of expanding this plan.
- For each code step, run the supervisor-delegated proof command, expected to
  be:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1
```

- Also run `git diff --check` before returning a packet.

## Step 1: Audit Div/Rem Evidence And Choose Packet

Goal: identify whether the first packet is capability repair, representative
probe validation, or close-readiness cleanup for existing div/rem support.

Actions:

- Inspect current RV64 scalar binary lowering and focused backend tests for
  `UDiv`, `SDiv`, `URem`, and `SRem`.
- Inspect available handoff artifacts for the 17-row integer div/rem bucket
  and representative rows.
- Classify rows into coherent signed division, unsigned division, signed
  remainder, unsigned remainder, unsupported width/helper, and unrelated
  residual buckets.
- Pick the first implementation or validation packet that has coherent
  prepared facts and a focused proof path.
- Record the chosen packet, out-of-scope rows, primary files, and exact proof
  command in `todo.md`.

Completion check:

- `todo.md` names the first packet and whether it is code-changing or
  validation/classification-only.
- No implementation files are changed in this step.

## Step 2: Lower Or Validate Scalar Division

Goal: ensure coherent scalar `sdiv` and `udiv` operations lower with correct
signedness and width.

Primary targets:

- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/prepared_function_emit.cpp`
- focused tests under `tests/backend/mir/`

Actions:

- Map `sdiv` to RV64 signed division and `udiv` to RV64 unsigned division for
  supported integer widths.
- Use `divw`/`divuw` for `i32` and `div`/`divu` for `i64`, or validate that
  this route is already present and covered.
- Require coherent prepared operand and result homes.
- Add or tighten focused tests for signed and unsigned division.

Completion check:

- Focused backend tests prove scalar signed and unsigned division.
- The backend subset proof passes and is saved in `test_after.log`.
- Unsupported or incoherent division shapes remain fail-closed.

## Step 3: Lower Or Validate Scalar Remainder

Goal: ensure coherent scalar `srem` and `urem` operations lower with correct
signedness and width.

Primary targets:

- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/prepared_function_emit.cpp`
- focused tests under `tests/backend/mir/`

Actions:

- Map `srem` to RV64 signed remainder and `urem` to RV64 unsigned remainder for
  supported integer widths.
- Use `remw`/`remuw` for `i32` and `rem`/`remu` for `i64`, or validate that
  this route is already present and covered.
- Require coherent prepared operand and result homes.
- Add or tighten focused tests for signed and unsigned remainder.

Completion check:

- Focused backend tests prove scalar signed and unsigned remainder.
- The backend subset proof passes and is saved in `test_after.log`.
- Unsupported or incoherent remainder shapes remain fail-closed.

## Step 4: Re-Probe Representatives And Decide Close Readiness

Goal: decide whether the integer div/rem source idea is complete or whether
remaining representative failures belong to other source ideas.

Actions:

- Re-run representative RV64 object-route probes for the rows selected in
  Step 1.
- Classify any remaining failures as in-scope div/rem target work or
  out-of-scope select, call, pointer/address, aggregate, global memory, F128,
  unsupported width/helper, producer, or accounting work.
- Run the supervisor-delegated backend proof and `git diff --check`.
- Summarize coverage, rejected row classes, and close readiness in `todo.md`.

Completion check:

- `todo.md` records whether the source idea appears ready for lifecycle close
  review.
- No expectation, allowlist, unsupported-marker, runtime-comparison, or
  pass/fail accounting files are changed or used as progress.
