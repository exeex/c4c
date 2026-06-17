# RV64 Direct Scalar Call Neighbor Coverage Runbook

Status: Active
Source Idea: ideas/open/300_rv64_direct_scalar_call_neighbor_coverage.md

## Purpose

Broaden qemu-backed rv64 direct scalar call coverage from the first helper-call
cases to neighboring local-scalar argument shapes while preserving the bounded
direct scalar register-call contract from idea 298.

## Goal

Execute at least one neighboring direct scalar local-argument case under
`qemu-riscv64`, then prove the rv64 runtime and RISC-V focused backend subsets
remain monotonic.

## Core Rule

Use prepared call plans, prepared value homes, and existing scalar local
materialization metadata as authority. Do not key behavior to helper names,
filenames, exact source text, or one known testcase shape.

## Read First

- `ideas/open/300_rv64_direct_scalar_call_neighbor_coverage.md`
- `tests/backend/case/two_arg_local_arg.c`
- `tests/backend/case/two_arg_both_local_arg.c`
- `tests/backend/case/two_arg_second_local_arg.c`
- `tests/backend/CMakeLists.txt`
- `tests/backend/cmake/run_backend_rv64_runtime_case.cmake`
- `src/backend/mir/riscv/codegen/emit.cpp`
- Prepared-call and prepared-value-home helpers reached from the rv64 emitter.

## Current Targets / Scope

- Register one or more neighboring direct scalar local-argument cases with
  `c4c_add_backend_rv64_runtime_case(...)`.
- Support direct scalar call arguments sourced from rematerializable
  immediates, prepared scalar local homes, or already-supported loaded local
  scalar values.
- Preserve `ra` save-restore behavior for functions that contain calls.
- Preserve fail-closed rejection for unsupported call forms.
- Preserve rejection of BIR/LLVM fallback before linking and qemu execution.

## Non-Goals

- Pointer parameters or member indexing; that scope belonged to idea 299.
- Indirect calls or function pointer calls.
- Varargs.
- Stack-passed arguments.
- Aggregate/byval/sret ABI.
- Global memory or function pointer tables.
- Filename-specific, helper-name-specific, or exact-source-shape shortcuts.

## Working Model

- Treat this as a narrow follow-on to the rv64 direct scalar call boundary work.
- Prefer registering and proving one candidate cleanly before adding more
  neighboring cases.
- If a candidate requires pointer/member indexing, stack arguments, varargs, or
  aggregate ABI, stop and record the boundary in `todo.md` instead of expanding
  this plan.
- If a candidate already passes by registration only, still run the focused
  proof and document that no compiler changes were needed.

## Execution Rules

- Keep each step small enough for one executor packet.
- Prefer `build -> narrow runtime case -> rv64 runtime subset -> RISC-V focused
  subset` proof.
- Escalate to `ctest --test-dir build -R '^backend_' --output-on-failure` if
  shared backend routing, prepared call plans, or non-rv64 code changes
  materially.
- Do not weaken existing backend expectations or downgrade supported-path tests
  without explicit supervisor approval.
- Do not accept BIR or LLVM fallback text as runtime success.

## Ordered Steps

### Step 1: Inspect candidate cases and register the first in-scope target

Goal: Confirm the first neighboring local-argument case stays inside this
idea's direct scalar call scope and add qemu-backed runtime coverage for it.

Primary targets:

- `tests/backend/case/two_arg_local_arg.c`
- `tests/backend/CMakeLists.txt`
- `tests/backend/cmake/run_backend_rv64_runtime_case.cmake`
- Existing prepared-call/value-home debug routes for rv64 backend cases.

Actions:

- Inspect `two_arg_local_arg.c` and its expected process status.
- Inspect the prepared call plan and value homes for the local scalar argument.
- Stop if the case requires pointer parameters, member indexing, indirect
  calls, varargs, stack-passed arguments, aggregate ABI, globals, or function
  pointer tables.
- Register the case with `c4c_add_backend_rv64_runtime_case(...)` using the
  expected qemu process status.
- Run the narrow runtime target and capture the exact success or failure mode.
- Confirm fallback rejection still occurs before clang/qemu execution.

Completion check:

- The first in-scope neighboring runtime case is registered, or `todo.md`
  records why it exceeds this idea's scope.
- The observed behavior is attributable to direct scalar local-argument
  lowering, not test harness drift or fallback acceptance.

### Step 2: Harden direct scalar local-argument materialization

Goal: Make direct scalar call arguments from prepared local scalar homes lower
through the existing rv64 prepared-call path.

Primary target:

- `src/backend/mir/riscv/codegen/emit.cpp`

Actions:

- Follow prepared call-plan argument metadata and value homes for the local
  scalar source.
- Lower only the direct scalar register-call forms needed by the in-scope
  candidate.
- Preserve rematerializable immediate argument behavior and scalar return
  behavior from the existing rv64 direct-call route.
- Preserve `ra` save-restore behavior for call-containing functions.
- Keep unsupported stack-argument, aggregate/byval/sret, vararg, indirect-call,
  pointer/member, and global forms rejected before runnable assembly is
  emitted.

Completion check:

- The registered neighboring case executes under `qemu-riscv64` with its
  expected status, or `todo.md` records the next metadata-driven blocker.
- Existing direct scalar rv64 runtime call cases remain green or preserve only
  documented baseline failures.

### Step 3: Add adjacent direct scalar local-argument coverage

Goal: Extend coverage to the remaining neighboring direct scalar local-argument
shapes if they stay inside the idea's bounded contract.

Primary targets:

- `tests/backend/case/two_arg_both_local_arg.c`
- `tests/backend/case/two_arg_second_local_arg.c`
- `tests/backend/CMakeLists.txt`
- `src/backend/mir/riscv/codegen/emit.cpp`, only if metadata-driven lowering is
  still incomplete.

Actions:

- Inspect each adjacent candidate before registration.
- Register only candidates that remain direct scalar register calls with local
  scalar argument sources.
- Prefer no-op registration plus proof when existing lowering already supports
  the case.
- If another candidate exposes a real local scalar materialization gap, extend
  the same metadata-driven path rather than adding testcase-shaped matching.
- Stop and record a boundary if a candidate drags in out-of-scope ABI or
  pointer/member behavior.

Completion check:

- In-scope adjacent candidates are registered and either pass under qemu or
  have a recorded metadata-driven blocker.
- No helper-name, filename, or exact-source-shape shortcut is introduced.

### Step 4: Run acceptance validation and document boundaries

Goal: Prove the broadened rv64 direct scalar call surface without regressing
existing backend behavior.

Actions:

- Run:
  `ctest --test-dir build -R '^backend_rv64_runtime' --output-on-failure`
- Run:
  `ctest --test-dir build -R 'backend_riscv|backend_codegen_route_riscv64' --output-on-failure`
- If shared backend routing, prepared call plans, or non-rv64 code changed
  materially, run:
  `ctest --test-dir build -R '^backend_' --output-on-failure`
- Record any preserved baseline failures in `todo.md` with the exact command
  and failure names.

Completion check:

- At least one new neighboring direct scalar local-argument case passes under
  qemu.
- Existing rv64 runtime cases pass with the new case or cases included.
- The rv64 runner still rejects BIR and LLVM fallback before linking.
- Unsupported call forms remain fail-closed.
- RISC-V focused tests are monotonic or only retain documented baseline
  failures.
