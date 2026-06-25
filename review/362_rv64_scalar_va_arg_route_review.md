# RV64 Scalar `va_arg` Route Review

## Scope

- Active source idea: `ideas/open/362_rv64_scalar_vararg_and_variadic_lowering.md`
- Active plan: `plan.md`
- Review base: `ed3b6cd02` (`[plan] Activate RV64 scalar vararg lowering plan`)
- Reviewed range: `ed3b6cd02..HEAD`
- Commits since base: 2
- Trigger: hook marked `code_review_pending` after `babf48f5` (`Materialize RV64 scalar va_arg plans`)

## Base Selection

`ed3b6cd02` is the correct checkpoint because it created the current active
`plan.md` and `todo.md` for `ideas/open/362_rv64_scalar_vararg_and_variadic_lowering.md`.
The later lifecycle-tagged commit, `24dce338` (`[todo_only] Record RV64 scalar
vararg audit`), only advanced execution scratch state and did not reset,
reactivate, materially change, or reviewer-checkpoint the active source idea.

## Findings

No blocking findings.

The implementation is keyed on prepared RV64 vararg facts, not testcase names
or representative fixture paths. `make_rv64_scalar_va_arg_access_plan` consumes
`scalar_result`, `source_va_list`, and explicit `va_arg_payload_abi` facts, then
supports the narrow integer scalar class by producing an overflow-arg-area plan
from the RV64 single-field `va_list` layout
(`src/backend/prealloc/variadic_entry_plans.cpp:686`).

The RV64 helper operand population now records `VaArg` homes and either removes
the old generic scalar access-plan missing fact after materialization or replaces
operand-complete unsupported scalar classes with the narrower
`target_abi.va_arg.scalar_payload_abi` diagnostic
(`src/backend/prealloc/variadic_entry_plans.cpp:1067`). This matches Step 2's
contract to consume or precisely diagnose scalar `va_arg` without moving ahead
to `va_copy` or the object-emission admission gate.

Focused tests exercise both the supported integer scalar path and the unsupported
scalar payload diagnostic while preserving the explicit `va_copy` missing facts
for Step 3
(`tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp:8131`,
`tests/backend/bir/backend_prepared_printer_test.cpp:6697`).

No testcase-overfit reject signals were found. The diff does not mention
`src/20030914-2.c` or `src/920908-1.c`, does not add representative-name
matching, does not downgrade unsupported expectations, and does not introduce
printed-LLVM or rendered-output shape matching in the backend route.

## Judgments

- Idea alignment: matches source idea
- Runbook transcription: plan matches idea
- Route alignment: on track
- Technical debt: acceptable
- Validation sufficiency: narrow proof sufficient for the Step 2 slice; broader
  representative proof remains required before Step 5 or plan closure
- Reviewer recommendation: continue current route

## Validation Observed

`test_after.log` records a successful focused proof:

```sh
cmake --build build --target backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test && ctest --test-dir build -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer)$' --output-on-failure > test_after.log
```

Both `backend_prepare_frame_stack_call_contract` and
`backend_prepared_printer` passed.

## Plan-Owner Rewrite

No plan-owner rewrite is needed. The next packet should continue with Step 3
(`va_copy`) as already recorded in `todo.md`; the current hook-added dirty
review reminder in `todo.md` was not edited by this review.
