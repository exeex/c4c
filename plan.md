# RV64 Direct Scalar Call Boundary Hardening Runbook

Status: Active
Source Idea: ideas/open/298_rv64_direct_scalar_call_boundary_hardening.md

## Purpose

Make the bounded rv64 direct scalar call support from idea 296 an explicit
qemu-backed runtime capability, with owned fail-closed boundaries for call
forms outside direct scalar register calls.

## Goal

Execute at least one direct scalar helper-call runtime case under
`qemu-riscv64` while preserving rejection for unsupported call ABI forms.

## Core Rule

Use prepared call plans, call argument/result metadata, and value homes as the
source of truth. Do not key behavior to helper names, filenames, exact source
text, or one known testcase shape.

## Read First

- `ideas/open/298_rv64_direct_scalar_call_boundary_hardening.md`
- `src/backend/mir/riscv/codegen/emit.cpp`
- `tests/backend/CMakeLists.txt`
- `tests/backend/cmake/run_backend_rv64_runtime_case.cmake`
- Candidate cases under `tests/backend/case/`:
  - `two_arg_helper.c`
  - `local_arg_call.c`
  - `two_arg_local_arg.c`
- Relevant idea 296 route reviews under `review/` if present.

## Current Targets / Scope

- Register rv64 runtime tests with `c4c_add_backend_rv64_runtime_case(...)`.
- Support direct non-indirect scalar helper calls with register arguments.
- Support scalar integer return values from direct calls.
- Preserve `ra` save/restore behavior for functions that contain direct calls.
- Preserve fail-closed behavior for indirect calls, stack arguments, variadic
  calls, aggregate arguments, and aggregate returns.

## Non-Goals

- Indirect calls or function pointer calls.
- Varargs.
- Stack-passed arguments.
- Aggregate/byval/sret ABI.
- Global memory or function pointer tables.
- Broad ABI cleanup for non-rv64 backends.
- Filename-specific, helper-name-specific, or exact-source-shape shortcuts.

## Working Model

- Start from the smallest existing direct scalar helper-call case.
- Register one runtime case only after confirming it stays inside direct scalar
  register-call scope.
- Add explicit fail-closed checks before widening accepted call behavior.
- Treat candidate cases that require memory, pointer-heavy provenance, stack
  arguments, or aggregate ABI as split candidates instead of broadening this
  runbook.

## Execution Rules

- Keep each step small enough for one executor packet.
- Prefer `build -> rv64 runtime subset -> RISC-V focused subset` proof.
- Escalate to `ctest --test-dir build -R '^backend_' --output-on-failure` if
  shared backend routing, prepared call plans, or non-rv64 code changes.
- Do not weaken existing backend expectations or downgrade supported-path tests
  without explicit supervisor approval.
- Do not accept BIR or LLVM fallback text as runtime success.

## Ordered Steps

### Step 1: Register and classify the first direct scalar call case

Goal: Select the smallest helper-call candidate that fits direct scalar
register-call scope and add it to rv64 runtime coverage.

Primary targets:

- `tests/backend/CMakeLists.txt`
- `tests/backend/case/two_arg_helper.c`
- `tests/backend/case/local_arg_call.c`
- `tests/backend/cmake/run_backend_rv64_runtime_case.cmake`

Actions:

- Inspect `two_arg_helper.c` first, then `local_arg_call.c` if needed.
- Reject candidates that require indirect calls, varargs, stack-passed
  arguments, aggregate ABI, globals/function pointer tables, or pointer-heavy
  memory.
- Register the first in-scope candidate with
  `c4c_add_backend_rv64_runtime_case(...)` using the expected qemu process
  status from the C `main` return value.
- Run the narrow rv64 runtime target and capture the exact failure or success
  mode.
- Confirm fallback rejection still occurs before clang/qemu execution.

Completion check:

- One in-scope direct scalar helper-call case is registered as an rv64 runtime
  case, or `todo.md` records why the first candidates exceed this idea's scope.
- The observed behavior is attributable to rv64 direct scalar call lowering,
  not test harness drift or fallback acceptance.

### Step 2: Harden direct scalar argument and return lowering

Goal: Make the selected direct scalar helper-call case execute through prepared
call metadata.

Primary target:

- `src/backend/mir/riscv/codegen/emit.cpp`

Actions:

- Inspect the prepared call plan, argument source, result, and value-home data
  available to rv64 emission.
- Lower only direct calls with scalar register arguments and scalar integer
  returns.
- Preserve argument order and integer width/sign-extension behavior required by
  the selected case.
- Keep unsupported forms rejected before runnable assembly is emitted.
- Avoid broad call ABI invention outside the selected direct scalar path.

Completion check:

- The selected rv64 runtime case executes under `qemu-riscv64`.
- Existing rv64 runtime cases still pass.
- Unsupported call forms encountered during the step still fail closed.

### Step 3: Verify call-frame preservation boundaries

Goal: Ensure functions containing direct calls preserve return-address behavior
and do not regress scalar/local execution.

Primary targets:

- `src/backend/mir/riscv/codegen/emit.cpp`
- Existing rv64 runtime scalar/local cases.

Actions:

- Inspect current prologue/epilogue and call emission behavior for `ra`
  save/restore.
- Add or tighten the smallest semantic handling needed for functions that
  contain direct calls.
- Verify non-call scalar/local rv64 runtime cases remain unchanged in behavior.
- Keep leaf-function and non-call paths from gaining unnecessary frame changes
  unless the existing backend model requires it.

Completion check:

- Direct-call runtime behavior does not clobber the caller return path.
- Existing scalar/local rv64 runtime cases continue to pass.
- Any frame changes are tied to call presence or existing prepared metadata,
  not blanket testcase-specific emission.

### Step 4: Add one neighboring direct scalar call case if still in scope

Goal: Prove the direct scalar call boundary is not limited to one exact helper
shape.

Primary candidate cases:

- `local_arg_call.c`
- `two_arg_local_arg.c`
- Neighboring direct scalar helper-call cases in `tests/backend/case/`

Actions:

- Inspect candidate cases before registering them.
- Proceed only when the case stays within direct scalar register arguments and
  scalar integer returns.
- Reuse the same prepared-call lowering path as Step 2.
- Stop and report a split if the case requires stack arguments, varargs,
  aggregates, indirect calls, global memory, or pointer-heavy memory.

Completion check:

- One neighboring direct scalar call case either passes under qemu through the
  same semantic path, or `todo.md` records why all candidates exceed this
  idea's boundaries.

### Step 5: Run acceptance validation and document residual boundaries

Goal: Prove the active direct scalar call surface without hiding unsupported
call forms or regressing existing backend coverage.

Actions:

- Run:
  `ctest --test-dir build -R '^backend_rv64_runtime' --output-on-failure`
- Run:
  `ctest --test-dir build -R 'backend_riscv|backend_codegen_route_riscv64' --output-on-failure`
- If shared backend routing, prepared call plans, or non-rv64 code changed,
  run:
  `ctest --test-dir build -R '^backend_' --output-on-failure`
- Record any preserved baseline failures in `todo.md` with the exact command
  and failure names.

Completion check:

- At least one new direct scalar helper-call runtime case passes under qemu.
- The rv64 runner still rejects BIR and LLVM fallback text before linking.
- Unsupported call forms remain fail-closed.
- RISC-V focused tests are monotonic or only retain documented baseline
  failures.
