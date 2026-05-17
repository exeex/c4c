# AArch64 Float Ops Shard Redistribution Runbook

Status: Active
Source Idea: ideas/open/260_aarch64_float_ops_markdown_shard_implementation_redistribution.md

## Purpose

Move the valid `float_ops.md` shard behavior into compiled AArch64 codegen owners without changing floating-point semantics.

## Goal

Create `src/backend/mir/aarch64/codegen/float_ops.cpp` and `float_ops.hpp` as the ordinary owner for scalar F32/F64 floating operation construction, lowering, and spelling, then delete the stale markdown shard.

## Core Rule

This is ownership redistribution only. Preserve existing selected-node behavior, fail-closed checks, diagnostics, and emitted assembly.

## Read First

- `ideas/open/260_aarch64_float_ops_markdown_shard_implementation_redistribution.md`
- `src/backend/mir/aarch64/codegen/float_ops.md`
- `src/backend/mir/aarch64/codegen/alu.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- `tests/backend/mir/backend_aarch64_prepared_scalar_alu_records_test.cpp`
- `tests/backend/mir/backend_aarch64_scalar_record_contract_test.cpp`
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`
- `tests/backend/mir/backend_aarch64_machine_printer_test.cpp`

## Current Targets

- `src/backend/mir/aarch64/codegen/float_ops.cpp`
- `src/backend/mir/aarch64/codegen/float_ops.hpp`
- `src/backend/mir/aarch64/codegen/alu.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- `src/backend/mir/aarch64/codegen/float_ops.md`
- focused AArch64 scalar ALU, dispatch, printer, and backend smoke tests

## Non-Goals

- Do not redistribute cast, f128, i128, memory, call, branch, or intrinsic shards.
- Do not invent missing FP prepared facts or expand floating-point semantics.
- Do not route F128 arithmetic through scalar F32/F64 float ops.
- Do not redesign `instruction.hpp` or the instruction type hierarchy solely for this shard.
- Do not rewrite expectations, downgrade support, or mark supported FP paths unsupported.
- Do not restore a centralized record pile under a new name.

## Working Model

- `float_ops` owns scalar F32/F64 floating ALU helpers that are currently mixed into broader scalar owners.
- `dispatch.cpp` should route prepared scalar FP ALU work to the float-ops owner instead of containing float-family bodies.
- `machine_printer.cpp` may keep generic printer routing, but scalar F32/F64 FP operation spelling should be delegated to `float_ops` where practical.
- `instruction.cpp` should retain family-neutral scalar instruction structure and common validation, not float-family bulk implementation.
- F128 behavior stays at the existing f128/helper boundary unless a later f128 idea changes it.

## Execution Rules

- Keep each step behavior-preserving.
- Prefer small moves with focused proof over broad file cleanup.
- Preserve fail-closed behavior for missing FPR facts, wrong register banks, F128 operands, and unsupported opcodes.
- If a helper is shared by integer scalar ALU and scalar FP ALU, move only the float-specific part or leave a narrow shared helper in the existing owner.
- If an apparent float helper is actually part of another shard, document that boundary in `todo.md` instead of moving it into this plan.
- Run `git diff --check` after code edits that touch printer or formatting-heavy files.

## Step 1: Audit Float Ownership And Boundaries

Goal: identify exactly which compiled code belongs to `float_ops`.

Actions:
- Inspect scalar FP ALU classification, record construction, prepared record construction, dispatch lowering, and printer spelling in the target files.
- Distinguish scalar F32/F64 binary ALU behavior from cast, f128, integer ALU, and scalar FP intrinsic behavior.
- Identify current focused tests that cover F32/F64 selected records, fail-closed facts, dispatch, and printed mnemonics.
- Record any boundary decision in `todo.md` if a tempting helper should stay outside `float_ops`.

Completion check:
- The executor can name the functions to move, the functions to leave in place, and the first proof subset before making code edits.

## Step 2: Create Float Ops Owner And Move Construction Helpers

Goal: add compiled `float_ops` files and move scalar F32/F64 ALU classification and construction into them.

Primary target:
- `src/backend/mir/aarch64/codegen/float_ops.cpp`
- `src/backend/mir/aarch64/codegen/float_ops.hpp`

Actions:
- Create `float_ops.hpp` declarations for scalar FP ALU predicates, operation selection helpers, and prepared/scalar record helpers that are genuinely float-family behavior.
- Move the corresponding definitions out of broad owners while keeping shared scalar core in `instruction.cpp` or `alu.cpp` as appropriate.
- Update includes and tests so direct float helpers include `float_ops.hpp`.
- Preserve public names where churn is not needed; namespace ownership can move without changing behavior.

Completion check:
- Focused scalar ALU record tests build and pass.
- No cast, f128, or integer ALU behavior changes.

Suggested proof:

```bash
{ cmake --build build --target c4c_backend backend_aarch64_scalar_record_contract_test backend_aarch64_prepared_scalar_alu_records_test && ctest --test-dir build -R '^(backend_aarch64_scalar_record_contract|backend_aarch64_prepared_scalar_alu_records)$' --output-on-failure; } > test_after.log 2>&1
```

## Step 3: Move Prepared Dispatch Lowering To Float Ops

Goal: make `dispatch.cpp` route scalar FP ALU lowering through the float-ops owner.

Primary target:
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/float_ops.cpp`
- `src/backend/mir/aarch64/codegen/float_ops.hpp`

Actions:
- Move prepared scalar FP ALU lowering bodies and related fail-closed checks into `float_ops`.
- Leave `dispatch.cpp` responsible for route selection and generic sequencing.
- Preserve the existing selected-node facts for F32/F64 FPR operands and results.
- Preserve fail-closed behavior for missing or wrong prepared register facts.

Completion check:
- Dispatch still selects supported F32/F64 scalar FP ALU when complete facts exist.
- Dispatch still defers/fails closed when FPR facts are incomplete or mismatched.

Suggested proof:

```bash
{ cmake --build build --target c4c_backend backend_aarch64_instruction_dispatch_test backend_aarch64_prepared_scalar_alu_records_test && ctest --test-dir build -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_prepared_scalar_alu_records)$' --output-on-failure; } > test_after.log 2>&1
```

## Step 4: Move Scalar FP Printer Spelling

Goal: delegate scalar F32/F64 FP mnemonic spelling and diagnostics from `machine_printer.cpp` into `float_ops`.

Primary target:
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- `src/backend/mir/aarch64/codegen/float_ops.cpp`
- `src/backend/mir/aarch64/codegen/float_ops.hpp`

Actions:
- Move scalar FP ALU printer spelling for `fadd`, `fsub`, `fmul`, and `fdiv` into `float_ops`.
- Keep `machine_printer.cpp` as generic printer routing and ALU fallback owner.
- Preserve diagnostics for incomplete printer facts, wrong register banks, unsupported operations, and non-selected records.
- Do not move F128 helper printing in this step.

Completion check:
- Machine-printer tests still print the same scalar FP assembly and reject the same invalid records.
- `git diff --check` passes.

Suggested proof:

```bash
{ cmake --build build --target c4c_backend backend_aarch64_machine_printer_test backend_aarch64_scalar_record_contract_test backend_aarch64_prepared_scalar_alu_records_test && ctest --test-dir build -R '^(backend_aarch64_machine_printer|backend_aarch64_scalar_record_contract|backend_aarch64_prepared_scalar_alu_records)$' --output-on-failure; } > test_after.log 2>&1
git diff --check
```

## Step 5: Reconcile And Delete Float Ops Markdown

Goal: remove the stale markdown shard after its valid durable content has a compiled owner.

Actions:
- Compare `float_ops.md` against the new `float_ops.cpp` and `float_ops.hpp`.
- Ensure valid scalar F32/F64 operation guidance is represented in code structure, tests, or concise comments only where useful.
- Do not copy stale reference-implementation prose into another doc file.
- Delete `src/backend/mir/aarch64/codegen/float_ops.md`.

Completion check:
- `float_ops.md` is gone.
- `float_ops.cpp` and `float_ops.hpp` are the discoverable compiled owner for the shard.

Suggested proof:

```bash
{ cmake --build build --target c4c_backend backend_aarch64_scalar_record_contract_test backend_aarch64_prepared_scalar_alu_records_test backend_aarch64_machine_printer_test && ctest --test-dir build -R '^(backend_aarch64_scalar_record_contract|backend_aarch64_prepared_scalar_alu_records|backend_aarch64_machine_printer)$' --output-on-failure; } > test_after.log 2>&1
```

## Step 6: Broader Backend Validation

Goal: prove the redistribution did not regress backend behavior.

Actions:
- Run the broader backend validation selected by the supervisor.
- Confirm scalar FP ALU, dispatch, printer, and public backend smoke paths are covered.
- Mark the runbook ready for closure review only after fresh broad proof is recorded in `test_after.log`.

Completion check:
- Broad backend proof passes with no new failures.
- `todo.md` records the final proof command and result.

Suggested proof:

```bash
{ cmake --build build --target c4c_backend c4cll && ctest --test-dir build -R '^backend_' --output-on-failure; } > test_after.log 2>&1
```
