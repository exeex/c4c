# RV64 Object Route 20000112 Instruction Fragment Lowering Runbook

Status: Active
Source Idea: ideas/open/378_rv64_object_route_20000112_instruction_fragment_lowering.md

## Purpose

Repair the next RV64 object-route ordinary instruction-fragment blocker exposed
by `src/20000112-1.c` after idea 369 moved the case past terminator lowering.

## Goal

Audit the first unsupported prepared instruction fragment, implement the first
supportable semantic lowering, and prove that `src/20000112-1.c` either passes
or advances to a distinct next owner.

## Core Rule

Base progress on prepared instruction, operand, type, value-home, and
publication facts. Do not infer lowering behavior from testcase names, source
syntax, value ids, instruction indexes, block numbering, or prepared-BIR text
matching.

## Read First

- `ideas/open/378_rv64_object_route_20000112_instruction_fragment_lowering.md`
- `test_before.log`
- `build/agent_state/369_step5_terminator_representatives.runner.log`
- `build/rv64_gcc_c_torture_backend/src_20000112-1.c/case.log`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`

## Current Targets

- Primary representative: `src/20000112-1.c`
- Primary diagnostic family: `unsupported_instruction_fragment`
- Focused proof targets:
  - `backend_riscv_object_emission_test`
  - `backend_prepare_frame_stack_call_contract_test`
  - `backend_prepared_printer_test`

## Non-Goals

- Do not reopen idea 369 terminator-fragment lowering, branch-target handling,
  or CFG reconstruction.
- Do not mix in data sections, globals, strings, relocations, byval aggregate
  homes, aggregate `va_arg`, non-register parameter homes, or unrelated
  call-argument ownership.
- Do not perform a broad assembler or object-route replacement.
- Do not claim diagnostic-only changes, expectation rewrites, or allowlist
  edits as capability progress.

## Working Model

The object route has already accepted the module far enough to expose an
ordinary prepared instruction-fragment failure in `src/20000112-1.c`. The first
packet should identify the semantic instruction shape from object-route
evidence, then later packets should implement only the first supportable class
and keep adjacent unsupported shapes fail-closed.

## Execution Rules

- Keep implementation packets small and semantic.
- Preserve precise unsupported diagnostics for adjacent shapes that are not
  part of the selected lowering.
- Add focused object-emission tests before treating a lowering as accepted.
- Rerun the representative after each admitted instruction-fragment repair.
- If the representative advances to a different owner, record that owner in
  `todo.md` instead of broadening this plan silently.

## Steps

### Step 1: Audit First Unsupported Instruction Fragment

Goal: identify the first ordinary prepared instruction-fragment shape rejected
by the RV64 object route for `src/20000112-1.c`.

Primary target: `build/rv64_gcc_c_torture_backend/src_20000112-1.c/case.log`
and object-route diagnostics.

Actions:

- Reproduce or inspect the current `src/20000112-1.c` failure.
- Capture the prepared instruction opcode, operand kinds, operand types,
  published value homes, and destination facts relevant to the failing
  fragment.
- Improve diagnostics only if needed to reveal semantic prepared facts.
- Record the audited shape and the next implementation target in `todo.md`.

Completion check:

- `todo.md` names the first unsupported semantic instruction shape from
  prepared/object-route evidence, and any diagnostic-only changes are not
  claimed as capability progress.

### Step 2: Implement First Supportable Instruction Lowering

Goal: lower the first audited supportable instruction-fragment shape through
the RV64 object route.

Primary target: `src/backend/mir/riscv/codegen/object_emission.cpp`

Actions:

- Use existing prepared instruction, operand, type, and value-home helpers
  where possible.
- Emit RV64 object code from semantic facts rather than testcase-specific
  identifiers.
- Keep unsupported adjacent instruction variants fail-closed with precise
  diagnostics.
- Avoid unrelated rewrites to terminators, CFG, data sections, globals,
  aggregate ABI handling, and call arguments.

Completion check:

- The selected instruction-fragment shape has semantic RV64 object lowering,
  and unsupported nearby variants still reject explicitly.

### Step 3: Add Focused Object-Emission Coverage

Goal: prove the admitted shape and nearby rejected shapes in focused backend
tests.

Primary target: `tests/backend/mir/backend_riscv_object_emission_test.cpp`

Actions:

- Add or extend tests for emitted bytes, relocations, value-home effects, or
  diagnostics as appropriate for the selected instruction.
- Add fail-closed tests for adjacent unsupported shapes that could otherwise be
  accidentally admitted.
- Keep test contracts semantic and independent of testcase names.

Completion check:

- Focused tests cover both the supported shape and rejected neighboring shapes.

### Step 4: Run Focused Backend Proof

Goal: verify the implementation slice against the focused backend contract.

Actions:

- Run:

```sh
( cmake --build build --target c4cll backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test backend_riscv_object_emission_test && ctest --test-dir build -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_riscv_object_emission)$' --output-on-failure ) > test_after.log 2>&1
```

- Inspect failures for route drift or overfit before continuing.

Completion check:

- `test_after.log` contains a green focused backend proof, or `todo.md`
  records the exact blocker and the route remains inside this idea.

### Step 5: Rerun Representative Case

Goal: prove the representative advanced because of semantic
instruction-fragment repair.

Actions:

- Run:

```sh
mkdir -p build/agent_state && printf '%s\n' 'src/20000112-1.c' > build/agent_state/378_step5_20000112.allowlist.txt && ALLOWLIST=build/agent_state/378_step5_20000112.allowlist.txt STOP_ON_FAILURE=1 VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > test_after.log 2>&1
```

- Record whether `src/20000112-1.c` passes or advances to a documented
  distinct next owner.
- If a distinct owner appears, stop and recommend a lifecycle handoff instead
  of expanding this plan.

Completion check:

- `src/20000112-1.c` either passes or the next owner is documented in
  `todo.md`, and the original audited instruction-fragment shape no longer
  blocks first.
