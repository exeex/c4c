# RV64 Runtime Pointer Parameter Member Index Foundation Runbook

Status: Active
Source Idea: ideas/open/299_rv64_runtime_pointer_param_member_index_foundation.md

## Purpose

Extend the qemu-backed rv64 runtime route to the next local-memory frontier:
a direct scalar helper call that receives a pointer to a local aggregate and
performs simple member-array indexing through that pointer.

## Goal

Execute `tests/backend/case/local_dynamic_member_array.c` under
`qemu-riscv64` with expected exit code `11` while preserving fail-closed
behavior for unsupported pointer, member, and call forms.

## Core Rule

Use prepared value homes, memory accesses, call plans, and address
materialization metadata as the source of truth. Do not key behavior to
`local_dynamic_member_array.c`, `get_at`, exact source text, or one known
testcase shape.

## Read First

- `ideas/open/299_rv64_runtime_pointer_param_member_index_foundation.md`
- `tests/backend/case/local_dynamic_member_array.c`
- `tests/backend/CMakeLists.txt`
- `tests/backend/cmake/run_backend_rv64_runtime_case.cmake`
- `src/backend/mir/riscv/codegen/emit.cpp`
- Prepared-memory, prepared-call, and address-materialization helpers reached
  from the rv64 emitter.

## Current Targets / Scope

- Register `local_dynamic_member_array.c` with
  `c4c_add_backend_rv64_runtime_case(...)`.
- Support passing a local aggregate address as a scalar pointer argument.
- Support receiving that pointer parameter in the callee.
- Support computing simple member-array element addresses from a pointer
  parameter plus scalar index.
- Support loading an I32 element from that computed address and returning it
  as a scalar integer result.
- Preserve rejection of BIR/LLVM fallback before linking and qemu execution.
- Preserve fail-closed behavior for unsupported pointer/member/index forms.

## Non-Goals

- Global memory or string constants.
- Indirect calls or function pointer calls.
- Varargs.
- Stack-passed arguments.
- Aggregate/byval/sret ABI.
- Dynamic alloca or VLA.
- General pointer provenance beyond this pointer-parameter local aggregate
  access path.
- Filename-specific, helper-name-specific, or exact-source-shape shortcuts.

## Working Model

- Treat this as the join point between previous local stack-memory work and
  bounded direct scalar register-call work.
- Start by observing the prepared metadata for the candidate case before
  changing emission.
- Keep support limited to the path where `main` passes a local aggregate
  address to a direct scalar helper and the callee indexes an I32 array member
  through that pointer parameter.
- Stop and split if the case requires a general non-local pointer provenance
  design or broader ABI support.

## Execution Rules

- Keep each step small enough for one executor packet.
- Prefer `build -> narrow case -> rv64 runtime subset -> RISC-V focused subset`
  proof.
- Escalate to `ctest --test-dir build -R '^backend_' --output-on-failure` if
  shared prepared-memory, call-plan, address-materialization, or backend
  routing behavior changes materially.
- Do not weaken existing backend expectations or downgrade supported-path tests
  without explicit supervisor approval.
- Do not accept BIR or LLVM fallback text as runtime success.

## Ordered Steps

### Step 1: Inspect metadata and register the candidate case

Goal: Confirm the candidate stays inside this idea's bounded pointer-parameter
member-indexing scope and add it to rv64 runtime coverage.

Primary targets:

- `tests/backend/case/local_dynamic_member_array.c`
- `tests/backend/CMakeLists.txt`
- `tests/backend/cmake/run_backend_rv64_runtime_case.cmake`
- Existing prepared metadata dumps or debug routes for rv64 backend cases.

Actions:

- Inspect the candidate source and expected return value.
- Inspect the prepared memory/access records, value homes, call plan, and
  address materialization available for the case.
- Reject or stop if the case requires globals, indirect calls, varargs,
  stack-passed arguments, aggregate ABI, dynamic stack, or broad pointer
  provenance outside this idea.
- Register the case with `c4c_add_backend_rv64_runtime_case(...)` using the
  expected qemu process status `11`.
- Run the narrow rv64 runtime target and capture the exact failure or success
  mode.
- Confirm fallback rejection still occurs before clang/qemu execution.

Completion check:

- `backend_rv64_runtime_local_dynamic_member_array` is registered, or
  `todo.md` records why the candidate exceeds this idea's scope.
- The observed behavior is attributable to rv64 pointer-parameter member
  indexing or address lowering, not test harness drift or fallback acceptance.

### Step 2: Materialize local aggregate address arguments

Goal: Pass a local aggregate address from the caller to a direct scalar helper
as a bounded scalar pointer argument.

Primary target:

- `src/backend/mir/riscv/codegen/emit.cpp`

Actions:

- Follow prepared call-plan argument metadata and value homes for the address
  of the caller's local aggregate.
- Lower only the direct scalar register-call argument form needed for a local
  aggregate address.
- Preserve existing scalar argument and return behavior from prior rv64 direct
  call work.
- Keep unsupported aggregate/byval/sret, stack-argument, and indirect-call
  forms rejected before runnable assembly is emitted.

Completion check:

- The caller can pass the local aggregate address through the prepared call
  path without using filename, helper-name, or exact-source-shape matching.
- Existing direct scalar rv64 runtime call cases remain green or preserve only
  documented baseline failures.

### Step 3: Resolve pointer-parameter member-array addresses in the callee

Goal: Compute a simple member-array element address from a pointer parameter
plus scalar index in the callee.

Primary target:

- `src/backend/mir/riscv/codegen/emit.cpp`

Actions:

- Inspect how the callee's pointer parameter is represented in prepared value
  homes and memory/address metadata.
- Reuse existing member-offset and local array element-address machinery where
  it applies.
- Add only the bounded bridge needed for pointer-parameter base plus simple
  member-array offset plus scalar index.
- Preserve fail-closed rejection for unsupported pointer provenance, nested or
  complex member forms, non-I32 element loads, and missing metadata.

Completion check:

- The callee can compute the address for `p->xs[i]` through metadata-driven
  lowering.
- Unsupported pointer/member/index shapes still fail closed instead of
  emitting partial runnable assembly.

### Step 4: Load and return the indexed I32 element under qemu

Goal: Complete the scalar load and return path for the computed member-array
element.

Primary target:

- `src/backend/mir/riscv/codegen/emit.cpp`

Actions:

- Lower the I32 load from the computed address using existing prepared-memory
  load conventions.
- Preserve integer width and return-register behavior expected by direct
  scalar calls.
- Run the narrow runtime case under qemu and inspect any remaining failure as
  address, load, call-frame, or return-value behavior.
- Keep the route fail-closed if required metadata is absent.

Completion check:

- `backend_rv64_runtime_local_dynamic_member_array` executes under
  `qemu-riscv64` with expected exit code `11`.
- The result comes from semantic metadata-driven lowering, not a named-case
  shortcut.

### Step 5: Run acceptance validation and document boundaries

Goal: Prove the rv64 runtime pointer-parameter member-indexing surface without
regressing existing backend behavior.

Actions:

- Run:
  `ctest --test-dir build -R '^backend_rv64_runtime' --output-on-failure`
- Run:
  `ctest --test-dir build -R 'backend_riscv|backend_codegen_route_riscv64' --output-on-failure`
- If shared prepared-memory, call-plan, address-materialization, or backend
  routing behavior changed materially, run:
  `ctest --test-dir build -R '^backend_' --output-on-failure`
- Record any preserved baseline failures in `todo.md` with the exact command
  and failure names.

Completion check:

- `backend_rv64_runtime_local_dynamic_member_array` passes under qemu.
- Existing rv64 runtime cases pass with the new case included.
- The rv64 runner still rejects BIR and LLVM fallback before linking.
- Unsupported pointer/member/index forms remain fail-closed.
- RISC-V focused tests are monotonic or only retain documented baseline
  failures.
