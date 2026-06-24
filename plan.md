# Target Object Emitter Scalar Scan Expansion Runbook

Status: Active
Source Idea: ideas/open/336_target_object_emitter_scalar_scan_expansion.md
Activated from: ideas/open/334_object_route_scan_and_default_readiness.md
Blocked parent: ideas/open/334_object_route_scan_and_default_readiness.md

## Purpose

Repair the first target-owned object-emitter gaps that block object-route scan
expansion. The scan harness can select object cases, but RV64 and AArch64
reject simple scalar/no-global object output before link/runtime.

## Goal

Make the first RV64 runtime scalar object candidates and AArch64 object-byte
scalar candidates pass through native target object emission, then hand back to
idea 334 for broader scan/default-readiness work.

## Core Rule

Do not satisfy these cases through textual assembly, expected-failure scan
labels, or testcase-shaped shortcuts. Support the target object
records/instructions that the scalar candidates actually produce, and preserve
existing asm-route and child-333 object-route proof.

## Read First

- `ideas/open/336_target_object_emitter_scalar_scan_expansion.md`
- `ideas/open/334_object_route_scan_and_default_readiness.md`
- `todo.md`
- `test_after.log` from the blocked Step 2 scan attempt, if present
- `src/backend/backend.hpp`
- `src/backend/backend.cpp`
- RV64 object emission and prepared-module lowering under `src/backend/`
- AArch64 object emission and machine-module construction under `src/backend/`
- `tests/backend/CMakeLists.txt`
- `tests/backend/cmake/run_backend_rv64_object_runtime_case.cmake`
- `tests/backend/cmake/run_backend_codegen_object_case.cmake`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `tests/backend/mir/backend_aarch64_object_emission_test.cpp`

## Current Scope

- Diagnose why RV64 prepared modules for `return_add.c`, `two_arg_helper.c`,
  `return_add_sub_chain.c`, and `local_temp.c` are rejected by object emission.
- Diagnose why AArch64 object emission rejects the machine instructions
  produced by `return_add.c` and `return_add_sub_chain.c`.
- Add minimal target-owned object support for that scalar/no-global subset.
- Add the green scan cases back only after the target object emitters support
  them.
- Preserve asm-route selectors and defaults.

## Non-Goals

- Do not implement broad RV64 globals/data object output.
- Do not implement x86 object output.
- Do not change c-testsuite defaults.
- Do not add object stdout.
- Do not add object `--backend-bir-stage semantic` mode.
- Do not require AArch64 runtime execution.
- Do not build textual assembler support.

## Working Model

The blocked scan candidates should eventually follow these routes:

```text
RV64 HIR -> LIR -> prepared BIR -> RV64 object records -> ELF .o -> clang link -> qemu-riscv64
AArch64 HIR -> LIR -> prepared BIR -> AArch64 machine module -> object records -> ELF .o
```

Failure ownership should stay target-local:

```text
unsupported prepared module shape -> inspect/fix RV64 prepared object writer support
unsupported AArch64 machine instruction -> inspect/fix AArch64 object writer support
```

## Execution Rules

- Start with inspection-only diagnosis of the rejected RV64 prepared-module
  shapes and AArch64 machine instructions; record target-owned seams and proof
  plan in `todo.md`.
- Add one target capability at a time and prove it with focused tests before
  restoring scan cases.
- Keep object output byte-producing and native; do not call external
  assemblers for `--codegen obj`.
- Do not weaken unsupported diagnostics or scan expectations to create green
  results.
- If a candidate requires broader semantic lowering unrelated to target object
  emission, split another focused idea instead of absorbing it here.

## Step 1: Inspect Scalar Object Rejections

Goal: identify the exact target object records, prepared-module shapes, and
machine instructions rejected by the first scalar scan candidates.

Primary targets:

- `src/backend/backend.cpp`
- RV64 prepared object writer and object-emission sources under `src/backend/`
- AArch64 object writer and machine-module sources under `src/backend/`
- `tests/backend/case/return_add.c`
- `tests/backend/case/two_arg_helper.c`
- `tests/backend/case/return_add_sub_chain.c`
- `tests/backend/case/local_temp.c`
- `tests/backend/CMakeLists.txt`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `tests/backend/mir/backend_aarch64_object_emission_test.cpp`

Actions:

- Reproduce or inspect diagnostics for the six blocked candidates.
- Locate the RV64 predicate or unsupported-shape check that rejects the
  prepared modules.
- Locate the AArch64 unsupported-instruction dispatch for scalar arithmetic.
- Record in `todo.md` the target-owned seams, first minimal implementation
  slice, expected tests, and unsupported boundaries.

Completion check:

- `todo.md` names the exact RV64 and AArch64 object-emitter seams and the
  first implementation/proof packet.

## Step 2: Add RV64 Scalar Prepared Object Support

Goal: support the first RV64 scalar/no-global prepared-module shapes needed by
the blocked runtime object candidates.

Actions:

- Extend RV64 prepared object emission for the minimal scalar constants,
  arithmetic/local, and call-shaped records required by the candidates.
- Add focused RV64 object-emission or backend facade tests for the newly
  supported records where useful.
- Add back RV64 object runtime scan cases only when they pass as green tests:
  `backend_obj_runtime_rv64_return_add`,
  `backend_obj_runtime_rv64_two_arg_helper`,
  `backend_obj_runtime_rv64_return_add_sub_chain`, and
  `backend_obj_runtime_rv64_local_temp`.

Completion check:

- The RV64 object runtime scalar subset links and runs with qemu, and existing
  RV64 asm runtime counterparts remain selected and green.

## Step 3: Add AArch64 Scalar Object Instruction Support

Goal: support AArch64 object byte emission for the scalar/no-global machine
instructions produced by `return_add.c` and `return_add_sub_chain.c`.

Actions:

- Extend AArch64 object writer/encoder support for the minimal scalar
  instruction family needed by the blocked candidates.
- Add focused AArch64 object-emission tests where useful.
- Add back AArch64 object-byte scan cases only when they pass as green tests:
  `backend_cli_aarch64_return_add_writes_elf_obj` and
  `backend_cli_aarch64_return_add_sub_chain_writes_elf_obj`.

Completion check:

- AArch64 object-byte scalar scan cases emit valid ELF objects and the
  existing AArch64 asm external return smokes remain green.

## Step 4: Restore Scan Candidate Proof

Goal: prove the original Step 2 object-route scan candidates now pass without
expected-failure labels or asm fallback.

Actions:

- Run the original blocked scan proof command with the restored candidate names.
- Confirm object selectors are independently selectable through
  `backend_obj_runtime_.*` and `backend_cli_.*obj`.
- Keep child-333 baseline object tests and nearby asm-route tests selected.
- Record result counts and any remaining blockers in `todo.md`.

Completion check:

- The previously blocked object scan expansion passes, or remaining failures
  are split into focused follow-up ideas with exact ownership.

## Step 5: Hand Back To Object Route Scan

Goal: decide whether this focused child has unblocked idea 334 enough to resume
scan/default-readiness work.

Actions:

- Summarize supported scalar object-route expansion and proof commands.
- Record remaining unsupported object features that still belong outside this
  child.
- Ask the plan owner to close this child or switch back to idea 334 when the
  source idea is satisfied.

Completion check:

- `todo.md` clearly tells the supervisor whether idea 334 can resume, and with
  which object scan selectors/proof command.
