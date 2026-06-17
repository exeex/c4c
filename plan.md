# RV64 Runtime Local Stack Memory Foundation Runbook

Status: Active
Source Idea: ideas/open/297_rv64_runtime_local_stack_memory_foundation.md

## Purpose

Extend the qemu-backed rv64 runtime path from scalar locals into narrow local
stack memory support without taking on globals, broad pointer provenance, calls,
or aggregate ABI work.

## Goal

Make at least one local stack memory case beyond `local_temp.c` execute under
`qemu-riscv64` while unsupported memory forms continue to fail closed.

## Core Rule

Use semantic prepared memory/access and value-home metadata for frame-slot based
local memory. Do not key behavior to filenames, exact source text, or one known
test shape.

## Read First

- `ideas/open/297_rv64_runtime_local_stack_memory_foundation.md`
- `src/backend/mir/riscv/codegen/emit.cpp`
- `tests/backend/CMakeLists.txt`
- `tests/backend/cmake/run_backend_rv64_runtime_case.cmake`
- Candidate cases under `tests/backend/case/`:
  - `local_array.c`
  - `local_pointer_deref.c`
  - `local_dynamic_member_array.c`
  - `packed_local_member_offsets.c`

## Current Targets / Scope

- Register rv64 runtime tests with `c4c_add_backend_rv64_runtime_case(...)`.
- Support frame-slot based I32 local loads and stores beyond `local_temp.c`.
- Support simple base-plus-constant-offset addressing when backed by prepared
  memory/access metadata.
- Support named pointer/local values only as needed for selected local
  frame-slot cases.
- Preserve fallback rejection before link/run.
- Preserve fail-closed behavior for memory forms outside the selected local
  frame-slot surface.

## Non-Goals

- Global memory and string constants.
- Function-call ABI expansion.
- Aggregate returns or by-value aggregate arguments.
- Dynamic alloca or VLA.
- Pointer provenance beyond local frame-slot base-plus-offset facts.
- Filename-specific, helper-name-specific, or exact-source-shape shortcuts.

## Working Model

- Start with the smallest existing local memory case that stays within frame
  slots.
- Add one accepted runtime case only after confirming the prepared metadata can
  describe the memory operation semantically.
- When a candidate needs globals, calls, aggregate ABI, dynamic stack support,
  or non-local provenance, stop and report the split instead of expanding this
  runbook.
- Treat fail-closed rejection as acceptable for unsupported local memory forms.

## Execution Rules

- Keep each step small enough for one executor packet.
- Prefer `build -> rv64 runtime subset -> RISC-V focused subset` proof.
- Escalate to the broader backend subset if shared prepared-memory or backend
  routing behavior changes materially.
- Do not weaken existing backend expectations or downgrade supported-path tests
  without explicit supervisor approval.
- Do not accept BIR or LLVM fallback text as runtime success.

## Ordered Steps

### Step 1: Register and classify the first local stack memory case

Goal: Select the smallest candidate that fits local frame-slot memory and add
it to rv64 runtime coverage only when its current failure is attributable to
missing rv64 local memory lowering.

Primary targets:

- `tests/backend/CMakeLists.txt`
- `tests/backend/case/local_array.c`
- `tests/backend/case/local_pointer_deref.c`
- `tests/backend/cmake/run_backend_rv64_runtime_case.cmake`

Actions:

- Inspect `local_array.c` and `local_pointer_deref.c` for globals, calls,
  aggregate ABI, dynamic stack allocation, or non-local provenance.
- Choose the first candidate that stays inside local frame-slot memory.
- Register it with `c4c_add_backend_rv64_runtime_case(...)` using the expected
  qemu process status from the C `main` return value.
- Run the narrow rv64 runtime target and capture the exact fail-closed mode.
- Confirm fallback rejection still occurs before clang/qemu execution.

Completion check:

- The selected case is registered as an rv64 runtime case, or a durable blocker
  explains why both first candidates exceed the idea scope.
- The observed failure is semantic rv64 local memory lowering work, not a test
  harness or fallback-acceptance issue.

### Step 2: Lower narrow frame-slot local loads and stores

Goal: Teach the rv64 prepared emitter to handle the selected local memory case
from prepared metadata.

Primary target:

- `src/backend/mir/riscv/codegen/emit.cpp`

Actions:

- Inspect the current prepared memory/access and value-home data available to
  rv64 emission.
- Add the smallest semantic lowering needed for frame-slot based I32 local
  loads and stores in the selected case.
- Use base frame slots and constant offsets from prepared metadata.
- Keep unsupported address forms rejected instead of emitting partial runnable
  assembly.
- Avoid broad pointer provenance or generic memory invention beyond the selected
  local frame-slot surface.

Completion check:

- The selected rv64 runtime case executes under `qemu-riscv64`.
- Existing rv64 runtime cases still pass.
- Unsupported forms encountered during the step still fail closed.

### Step 3: Add direct local pointer deref support if still in scope

Goal: Extend the local frame-slot model to a direct local pointer dereference
case only if it uses the same semantic machinery as Step 2.

Primary targets:

- `tests/backend/case/local_pointer_deref.c`
- `src/backend/mir/riscv/codegen/emit.cpp`

Actions:

- Register `local_pointer_deref.c` only if it remains within local frame-slot
  base-plus-offset facts.
- Reuse the Step 2 lowering path for pointer/local value homes.
- Fail closed if the case requires non-local pointer provenance, calls, globals,
  or aggregate ABI support.

Completion check:

- `local_pointer_deref.c` either passes under qemu through the same semantic
  local frame-slot path, or `todo.md` records why it must be split into a
  follow-up idea.

### Step 4: Probe simple member/offset candidates

Goal: Determine whether one simple local member or offset candidate can be
supported without broadening beyond local frame-slot base-plus-constant-offset
memory.

Primary candidate cases:

- `local_dynamic_member_array.c`
- `packed_local_member_offsets.c`

Actions:

- Inspect each candidate before registering it.
- Proceed only if the prepared metadata already exposes the needed local
  frame-slot base and constant offset information cleanly.
- Stop and report a split if the case requires dynamic index machinery,
  aggregate ABI expansion, globals, calls, or non-local provenance.

Completion check:

- One simple offset/member local case is either accepted and passes under qemu,
  or the plan has a clear fail-closed boundary and a suggested follow-up split.

### Step 5: Run acceptance validation and document residual boundaries

Goal: Prove the active local stack memory surface without hiding unsupported
forms or regressing existing backend coverage.

Actions:

- Run:
  `ctest --test-dir build -R '^backend_rv64_runtime' --output-on-failure`
- Run:
  `ctest --test-dir build -R 'backend_riscv|backend_codegen_route_riscv64' --output-on-failure`
- If shared prepared-memory or backend routing behavior changed materially, run:
  `ctest --test-dir build -R '^backend_' --output-on-failure`
- Record any preserved baseline failures in `todo.md` with the exact command
  and failure names.

Completion check:

- At least one new local stack memory case beyond `local_temp.c` passes under
  qemu.
- The rv64 runner still rejects BIR and LLVM fallback text before linking.
- RISC-V focused tests are monotonic or only retain documented baseline
  failures.
- Unsupported local memory forms remain fail-closed.
