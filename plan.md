# RV64 Runtime Scalar And Local Foundation Runbook

Status: Active
Source Idea: ideas/open/296_rv64_runtime_scalar_local_foundation.md

## Purpose

Extend the qemu-backed rv64 backend runtime path beyond immediate branch
smokes so it can execute a small foundation of scalar arithmetic,
return-value, and tiny local-slot backend cases from `tests/backend/case/`.

Goal: make the public `c4c --codegen asm --target riscv64-linux-gnu` route
execute selected scalar and local cases under qemu without accepting fallback
BIR or LLVM text as assembly.

## Core Rule

Repair semantic lowering in the rv64 prepared-BIR emitter. Do not add
testcase-name checks, exact-source-shape shortcuts, or expectation downgrades.

## Read First

- `ideas/open/296_rv64_runtime_scalar_local_foundation.md`
- `tests/backend/CMakeLists.txt`
- `src/backend/mir/riscv/codegen/emit.cpp`
- Existing rv64 runtime tests registered through
  `c4c_add_backend_rv64_runtime_case(...)`

## Current Targets

- Runtime registration for this ordered case set, adjusted only if a named case
  proves outside the idea's scope:
  - `return_zero.c`
  - `return_add.c`
  - `return_add_sub_chain.c`
  - one tiny local-slot case such as `local_temp.c`
- rv64 prepared-BIR emitter support for:
  - integer binary operations used by the selected cases
  - named temporary values
  - returning a named scalar value
  - the smallest necessary local-slot load/store path

## Non-Goals

- Full rv64 ABI completion.
- Function-call lowering, aggregate returns, varargs, global memory, or broad
  pointer provenance.
- Moving backend case files into target-specific directories.
- Rewriting the rv64 backend around a new architecture.
- Weakening existing backend, AArch64, semantic-BIR, or route tests.

## Working Model

- Keep qemu runtime execution as the acceptance boundary.
- Preserve the runner-side rejection of `bir.func` and LLVM IR fallback text
  before linking.
- Start with scalar return and arithmetic cases before touching local-slot
  machinery.
- If every candidate local-slot case pulls in broad stack, aggregate, global,
  or pointer work, stop and ask the supervisor to open a narrower local-slot
  source idea instead of expanding this runbook.

## Execution Rules

- Keep each implementation packet narrow enough to prove with a fresh build or
  focused compile/test command.
- Prefer a tiny value environment for named scalar temporaries before changing
  stack or local-slot machinery.
- Generalize binary-op value tracking for selected scalar cases; do not emit
  one-off instruction sequences tailored to a single testcase.
- When emitter or backend routing behavior changes materially, escalate proof
  from the rv64 runtime subset to the focused RISC-V/backend route subset.
- Escalate further to `ctest --test-dir build -R '^backend_' --output-on-failure`
  if shared backend routing or prepared emitter behavior changes materially.

## Ordered Steps

### Step 1: Register And Prove `return_zero.c`

Goal: include the simplest non-branch return case in the rv64 runtime subset.

Primary target:
- `tests/backend/CMakeLists.txt`

Actions:
- Register `return_zero.c` through `c4c_add_backend_rv64_runtime_case(...)`.
- Build the affected test target if needed.
- Run the rv64 runtime subset.
- Confirm the qemu runner still rejects fallback BIR or LLVM IR text before
  linking.

Completion check:
- `ctest --test-dir build -R '^backend_rv64_runtime' --output-on-failure`
  passes with `return_zero.c` included.

### Step 2: Add Named Scalar Return Support For `return_add.c`

Goal: make a non-branch scalar arithmetic return execute under qemu.

Primary targets:
- `tests/backend/CMakeLists.txt`
- `src/backend/mir/riscv/codegen/emit.cpp`

Actions:
- Register `return_add.c` in the rv64 runtime subset.
- If it fails, inspect the prepared BIR emitted for the case.
- Add the smallest semantic value materialization needed for named scalar
  temporaries and returning named scalar values.
- Keep existing semantic-BIR observation tests intact.

Completion check:
- The rv64 runtime subset passes with `return_add.c` included.
- `return_add.c` executes through qemu, not host-native execution or fallback
  text acceptance.

### Step 3: Generalize Binary-Op Value Tracking

Goal: support a short chain of scalar integer arithmetic without one-off
instruction emission.

Primary targets:
- `tests/backend/CMakeLists.txt`
- `src/backend/mir/riscv/codegen/emit.cpp`

Actions:
- Register `return_add_sub_chain.c`.
- Generalize integer binary-op lowering and value tracking for the selected
  scalar operations.
- Avoid special casing the exact expression sequence in the testcase.

Completion check:
- The rv64 runtime subset passes with `return_add_sub_chain.c` included.
- Nearby same-feature scalar cases remain plausible under the same lowering
  model, even if they are not registered in this packet.

### Step 4: Try One Tiny Local-Slot Case

Goal: add the smallest local-slot load/store path that fits this idea.

Primary targets:
- `tests/backend/CMakeLists.txt`
- `src/backend/mir/riscv/codegen/emit.cpp`

Actions:
- Try `local_temp.c` first, or another tiny local-slot case if `local_temp.c`
  proves broader than this idea.
- Implement only the minimal stack/local load-store path required by the
  selected tiny case.
- Stop instead of stretching the runbook if the case requires broad memory,
  aggregate, global, or pointer-provenance support.

Completion check:
- At least one tiny local-slot case executes under qemu without weakening
  existing tests, or the route is explicitly handed back for a narrower
  local-slot source idea.

### Step 5: Focused Backend Validation

Goal: prove the route remains compatible with existing RISC-V observation and
backend routing behavior.

Primary targets:
- Existing backend tests.
- Canonical executor proof log requested by the supervisor.

Actions:
- Run the rv64 runtime subset after all selected cases are registered.
- Run the focused RISC-V/backend route subset.
- Escalate to the broader backend subset if shared backend routing or prepared
  emitter behavior changed materially.

Completion check:
- `ctest --test-dir build -R '^backend_rv64_runtime' --output-on-failure`
  passes.
- `ctest --test-dir build -R 'backend_riscv|backend_codegen_route_riscv64' --output-on-failure`
  passes.
- Any required broader backend proof is recorded by the executor or supervisor.
