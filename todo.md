# Current Packet

Status: Active
Source Idea Path: ideas/open/123_prepared_call_result_late_publication_contract.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Source-In-Destination And Source-Register Visibility

## Just Finished

Completed plan Step 3: converted the AArch64 source-register recording and
register-source-in-destination alias gates to consult the shared
`PreparedCallResultLatePublicationFact` where the target-neutral facts are
available.

Changed files:
- `src/backend/mir/aarch64/codegen/calls.cpp`: `record_call_result_source_register`
  now gates direct call-result ABI source-register recording through
  `find_prepared_call_result_late_publication`; the selected register-source
  alias path in `record_call_boundary_source_in_destination` uses a narrow
  local bridge into the shared query before recording the source value in the
  selected destination register.
- `src/backend/prealloc/calls.hpp`: refined
  `source_in_destination_alias_available` to describe a register-backed source
  value that can be recorded in a prepared register destination with the same
  prepared bank, instead of requiring identical physical register names.
- `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp`:
  adjusted the focused alias query fixture to cover a distinct source and
  destination register in the same prepared bank.

Scalar-state recording, register parsing/view selection, and concrete
call-boundary machine records remain AArch64-local. Memory-backed
source-in-destination recording remains target-local because the shared
call-result late-publication query represents register-backed call-result
publication, not arbitrary memory-source move aliases.

## Suggested Next

Execute Step 4 by converting FPR/VREG call-result store-value retarget
availability to consume the shared late-publication query while keeping
`MemoryInstructionRecord` mutation, AArch64 register-bank/view checks, q/vector
spelling, and f128 handling local.

## Watchouts

- The local alias bridge preserves the old fallback when the selected
  `CallBoundaryMoveInstructionRecord` lacks prepared-bank identity; the shared
  query cannot classify that target-neutral route without prepared source and
  destination banks.
- `current_block_publication_consumption_available` remains intentionally
  unclaimed by this query. Do not derive it from destination identity; a future
  surface needs to accept or reference the existing current-block
  publication/producer facts.
- Do not move scalar-state mutation or machine-record construction into shared
  prepared code.

## Proof

Ran the delegated proof exactly:
`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_frame_stack_call_contract|backend_prealloc_call_boundary_classification|backend_call_boundary_effect_plan|backend_aarch64_call_boundary_owner|backend_aarch64_instruction_dispatch|backend_aarch64_target_instruction_records)$' >> test_after.log 2>&1`

Result: passed. `test_after.log` shows 6/6 tests passed:
`backend_prepare_frame_stack_call_contract`,
`backend_prealloc_call_boundary_classification`,
`backend_call_boundary_effect_plan`,
`backend_aarch64_call_boundary_owner`,
`backend_aarch64_instruction_dispatch`, and
`backend_aarch64_target_instruction_records`.

Supervisor regression guard:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
passed with 6/6 before and 6/6 after, no new failures.
