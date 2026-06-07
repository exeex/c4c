# Current Packet

Status: Active
Source Idea Path: ideas/open/123_prepared_call_result_late_publication_contract.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Convert AArch64 Calls To Consume The Shared Surface

## Just Finished

Completed the Step 5 follow-up: added a publication-fact-backed current-block
publication consumption query and routed the matching AArch64 consumer through
it.

Changed files:
- `src/backend/prealloc/prepared_lookups.hpp` and
  `src/backend/prealloc/prepared_lookups.cpp`: added
  `PreparedCurrentBlockPublicationConsumption` and
  `find_prepared_current_block_publication_consumption`, backed by
  `PreparedEdgePublicationSourceProducerLookups`. The query verifies the
  current block label, producer ordering, producer/instruction pointer
  agreement, and produced value name before reporting availability.
- `src/backend/mir/aarch64/codegen/calls.cpp`: `prepared_call_boundary_source_value`
  now consumes the shared current-block publication query for prepared
  source-producer-backed values instead of locally rediscovering the producer
  kind/result. The legacy same-block fallback remains separate in
  `call_boundary_source_value_by_name`.
- `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp`:
  added proof inside the delegated test subset that current-block publication
  consumption is fact-backed and fails closed for future or mismatched
  producer facts.
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`: added focused
  helper coverage for the same query; this test is built by the delegated
  proof and available for broader runs.

`find_prepared_call_result_late_publication` still does not infer
`current_block_publication_consumption_available` from
`PreparedCallResultPlan`. Current-block authority is now represented by the
separate producer-fact-backed query, which is the accurate surface for this
route.

Comparison publication authority, same-block fallback discovery,
scalar-state mutation, and machine-record emission remain AArch64-local.

## Suggested Next

Acceptance review is ready for the idea 123 Step 5 slice. The shared
late-publication surface now covers source-register publication,
source-in-destination aliasing, FPR/VREG store-value retarget eligibility, and
current-block publication consumption through a real producer-fact-backed
query.

## Watchouts

- Do not flip `current_block_publication_consumption_available` on
  `PreparedCallResultLatePublicationFact` unless that fact grows a real
  publication/producer-backed signature; destination identity alone is not
  authority.
- `call_boundary_source_value_by_name` still has a legacy same-block fallback
  after the prepared query. That fallback is intentionally separate from the
  prepared current-block publication-consumption route.
- The local bridge fallbacks for selected machine records that lack prepared
  identity remain. Removing them requires a separate proof that all relevant
  emitted CallAbi registers carry prepared value id and bank.

## Proof

Ran the supervisor-expanded proof to include the new helper-test coverage:
`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_lookup_helper|backend_prealloc_call_boundary_classification|backend_call_boundary_effect_plan|backend_aarch64_call_boundary_owner|backend_aarch64_instruction_dispatch|backend_aarch64_target_instruction_records)$' >> test_after.log 2>&1`

Result: passed. `test_after.log` shows 7/7 tests passed:
`backend_prepare_frame_stack_call_contract`,
`backend_prepared_lookup_helper`,
`backend_prealloc_call_boundary_classification`,
`backend_call_boundary_effect_plan`,
`backend_aarch64_call_boundary_owner`,
`backend_aarch64_instruction_dispatch`, and
`backend_aarch64_target_instruction_records`.

Supervisor regression guard:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
passed with 6/6 before and 7/7 after, no new failures.
