# Idea 71 Acceptance Review

## Scope

- Active source idea: `ideas/open/71_aarch64_scalar_control_flow_prepared_authority_cleanup.md`
- Review base: `5212b96c2 [plan] Activate idea71 scalar control-flow cleanup`
- Base rationale: this commit created the active `plan.md`/`todo.md` for idea 71 and is the active-plan start requested for review. Later lifecycle commit `ab8ca3b13` only recorded the Step 1 authority mapping, so it is execution context rather than the activation checkpoint.
- Commits reviewed: 5
- Diff reviewed: `5212b96c2..HEAD`

## Findings

No blocking findings.

No testcase-overfit found. The route does not add `print_llvm()`, `rendered_contains_all(...)`, named testcase dispatch, or case-number shortcuts. The focused fixture change in `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp` strengthens the old unpublished compare-cast case into a prepared compare-cast expectation: it now requires no diagnostics, four emitted instructions, and visible `sxtw`/`csel`/stack publication/store ordering. It does not mark a supported path unsupported or weaken an assertion.

## Alignment Notes

- `src/backend/mir/aarch64/codegen/alu.cpp` now groups BIR scalar opcode/type facts into `ScalarAluSemanticFacts` and uses those facts before record construction, while shift legality, unsigned reduction replacement, immediate admissibility, materialization, post-extension policy, and final `ScalarAluRecord` construction remain AArch64-local.
- `src/backend/mir/aarch64/codegen/alu.cpp` also replaces same-block binary/cast rescans for control-value materialization with `find_prepared_control_same_block_scalar_producer(...)`, consuming prepared producer facts before local target materialization.
- `src/backend/mir/aarch64/codegen/cast_ops.cpp` groups BIR cast opcode/type facts into `ScalarCastSemanticFacts`, but keeps conversion opcode spelling, register-bank checks, storage-plan validation, and `ScalarCastRecord` construction local.
- `src/backend/mir/aarch64/codegen/comparison.cpp` centralizes prepared branch, short-circuit, materialized compare-join target, and fused operand producer facts in local AArch64 helpers. Prepared facts provide branch conditions, labels, join continuation targets, and operand provenance; local code still chooses condition-code spelling, compare immediate admissibility, scratch registers, materialization, and emitted branch/compare records.

## Judgments

- Idea alignment: matches source idea
- Runbook transcription: plan matches idea
- Route alignment: on track
- Technical debt: acceptable
- Validation sufficiency: needs broader proof for final acceptance
- Reviewer recommendation: continue current route

## Validation Observed

`test_after.log` currently contains a successful no-op build followed by full CTest: `3417/3417` tests passed. That is stronger than the backend-only proof recorded in `todo.md`, but this review did not rerun validation.
