# Current Packet

Status: Active
Source Idea Path: ideas/open/123_prepared_call_result_late_publication_contract.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Convert AArch64 Calls To Consume The Shared Surface

## Just Finished

Completed plan Step 5: narrowed the remaining AArch64 call-result
late-publication eligibility checks that were still rediscovering shared
source-register publication facts.

Changed files:
- `src/backend/mir/aarch64/codegen/calls.cpp`: `AfterCallMoveLocalOwner`
  now computes `PreparedCallResultLatePublicationFact` once and uses it for
  selected after-call result register moves and stack-home result publication
  eligibility. `lower_after_call_moves` also uses the shared fact before
  synthesizing a stack-result home publication bundle.

Already-converted Step 3/4 consumers still consume the shared surface for
source-register recording, selected register-source-in-destination alias
availability, and FPR/VREG store-value retarget availability. Target-specific
emission, scalar-state mutation, scratch/register view choices, memory-store
records, and call-boundary machine records remain AArch64-local.

Current-block publication consumption remains explicitly not represented by
`find_prepared_call_result_late_publication`. The existing prepared
current-block/store/source-producer facts live in broader publication planning
surfaces, not in `PreparedCallResultPlan`; representing that route accurately
needs a fact/signature that accepts or references those existing publication
facts instead of deriving authority from call-result destination identity.

## Suggested Next

Acceptance review is ready for the idea 123 shared late-publication surface
slice. If the review requires current-block publication consumption to be
represented in this plan, split or add a follow-up packet for a real
publication-fact-backed query signature rather than extending
`PreparedCallResultPlan` heuristics.

## Watchouts

- `current_block_publication_consumption_available` is still intentionally
  unclaimed by `find_prepared_call_result_late_publication`; do not infer it
  from destination register/slot/stack-offset fields.
- The local bridge fallbacks remain for selected machine records that lack
  prepared identity. Removing those fallbacks requires first proving every
  relevant emitted CallAbi register carries prepared value id and bank.
- Memory-backed source-in-destination alias recording remains target-local;
  the shared call-result late-publication fact covers register-backed
  call-result publication, not arbitrary memory-source move aliases.

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
