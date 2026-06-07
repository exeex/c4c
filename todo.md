# Current Packet

Status: Active
Source Idea Path: ideas/open/123_prepared_call_result_late_publication_contract.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Convert AArch64 Calls To Consume The Shared Surface

## Just Finished

Plan Step 5 remains active after acceptance review. The last implementation
packet narrowed the AArch64 call-result late-publication eligibility checks
that were still rediscovering shared source-register publication facts, but
review blocked closure because current-block publication consumption is still
not represented by the shared surface.

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

Reviewer `review/idea_123_acceptance.md` judged the route on track and not
testcase-overfit, but not closure-ready. The source idea and active plan both
keep consumption of idea 117 current-block publication facts in scope, so the
unimplemented `current_block_publication_consumption_available` gap must remain
inside idea 123 rather than being treated as acceptance-ready.

## Suggested Next

Execute a narrow Step 5 follow-up: add or route through a real
publication-fact-backed query/signature for current-block publication
consumption that accepts or references existing idea 117 prepared
current-block publication/producer facts. Then convert only the matching
AArch64 consumer path to consult that surface while keeping comparison
publication authority, same-block producer discovery, scalar-state mutation,
and machine-record emission local.

## Watchouts

- `current_block_publication_consumption_available` is still intentionally
  unclaimed by `find_prepared_call_result_late_publication`; do not infer it
  from destination register/slot/stack-offset fields.
- Do not close idea 123 until current-block publication consumption is either
  implemented through existing idea 117 facts or deliberately split into a new
  source idea by supervisor/lifecycle decision.
- The next query shape should reference real current-block publication facts;
  extending `PreparedCallResultPlan` destination heuristics would be route
  drift.
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
