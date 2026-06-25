# RV64 Vararg ABI Hook Materialization

Status: Closed
Type: Target ABI follow-up
Parent: `ideas/closed/360_lir_bir_vararg_va_arg_contract_completion.md`

## Goal

Materialize the RV64 target ABI hook work exposed by the completed shared
LIR/BIR/prepared vararg contract so representative variadic cases can move
past structured object-admission requirements without target code guessing
source semantics.

## Why This Exists

Idea 360 completed the shared vararg contract and proved that the representative
RV64 cases now reach object admission with explicit missing target ABI facts.
The remaining work is target-specific:

- `target_abi.variadic_entry_state`
- `target_abi.va_list_layout`
- `helper_resources.scratch_register_count`
- `helper_resources.scratch_stack_bytes`
- `helper_operand_homes.va_start.destination_va_list`
- `helper_operand_homes.va_start.destination_va_list_address`
- `helper_operand_homes.va_arg_aggregate.source_va_list`
- `helper_operand_homes.va_arg_aggregate.aggregate_destination_payload`
- `helper_operand_homes.va_arg_aggregate.aggregate_access_plan`

That target hook/materialization work should be executed as its own initiative
instead of reopening the shared-contract source idea.

## In Scope

- Inspect how RV64 object admission consumes the prepared vararg facts emitted
  by idea 360.
- Define the RV64 ABI hook boundary for variadic entry state and `va_list`
  layout.
- Materialize RV64 `va_start` operand homes from explicit prepared state.
- Materialize or precisely diagnose aggregate `va_arg` operand homes without
  source/LIR/BIR reinspection.
- Add focused backend tests for the hook boundary and representative object
  admission outcomes.
- Re-run the representative RV64 allowlist for `src/20030914-2.c` and
  `src/920908-1.c`.

## Out of Scope

- Changing the shared LIR/BIR/prepared vararg contract unless a real contract
  defect is found and routed back as a separate idea.
- Testcase-name matching for `src/20030914-2.c`, `src/920908-1.c`, or any
  other torture case.
- Marking variadic cases unsupported to improve scan counts.
- Broad RV64 data-section, vector ABI, FPR ABI, or unrelated instruction
  coverage.
- Full implementation of every C vararg ABI corner if a focused hook boundary
  can precisely diagnose unsupported classes.

## Acceptance Criteria

- RV64 object admission consumes explicit prepared vararg facts for variadic
  entry state and `va_list` layout.
- `src/20030914-2.c` moves past the current target-only missing facts or fails
  with a more specific target ABI diagnostic.
- `src/920908-1.c` moves past the current helper-resource and aggregate
  operand-home missing facts or fails with a more specific target ABI
  diagnostic.
- Focused backend tests cover both successful hook publication and structured
  unsupported paths.
- Existing baseline tests for touched backend/prepared surfaces remain green.

## Closure Notes

Closed after Step 5 milestone validation. The focused backend proof passed for:

- `backend_prepare_frame_stack_call_contract`
- `backend_prepared_printer`
- `backend_cli_riscv64_variadic_entry_missing_contract_obj`
- `backend_riscv_object_emission`

The representative RV64 allowlist for `src/20030914-2.c` and `src/920908-1.c`
still failed, but both cases now fail at the broader RV64 object-route gate:
`unsupported_function_admission: variadic functions are not supported by the RV64
object route; RV64 object variadic function lowering is not implemented`.

The original target-only missing facts for variadic entry state, `va_list`
layout, `va_start` helper resources/homes, and aggregate `va_arg` helper
resources/homes were not observed in the inspected representative logs.

Residual scalar `va_arg`, `va_copy`, and broader RV64 object variadic function
lowering work is tracked separately instead of expanding this completed target
hook milestone.

## Reviewer Reject Signals

- Reject testcase-shaped shortcuts or named-case-only fixes for the two
  representative torture cases.
- Reject unsupported expectation downgrades, skip broadening, or scan filtering
  claimed as target ABI progress.
- Reject helper renames, diagnostic text churn, or classification-only changes
  that leave the same missing RV64 facts behind a new name.
- Reject RV64 object emission that guesses source-level vararg semantics
  instead of consuming explicit prepared vararg facts.
- Reject broad rewrites of unrelated RV64 object-route features such as
  globals, vector ABI, FPR ABI, or generic instruction selection.
- Reject retaining the exact old object-admission failure mode while claiming a
  hook was materialized.
