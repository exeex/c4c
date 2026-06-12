Status: Active
Source Idea Path: ideas/open/223_phase_e2_prepared_lookup_api_private_pass_context_readiness.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Close Out E2 Analysis

# Current Packet

## Just Finished

Completed plan.md Step 4: created the two accepted follow-up implementation
ideas for the only Step 3 ready-to-draft helper families and linked them from
`docs/bir_prealloc_fusion/phase_e2_prepared_lookup_api_private_pass_context_readiness.md`.

New follow-up ideas:

- `ideas/open/224_phase_e2_control_flow_branch_target_helper_private_pass_context.md`
- `ideas/open/225_phase_e2_fused_compare_operand_producer_helper_private_pass_context.md`

## Suggested Next

Start plan.md Step 5: close out the E2 analysis document and prepare lifecycle
closure review without claiming implementation progress.

## Watchouts

- This plan is analysis-only; do not edit implementation files.
- The Step 1 baseline explicitly prevents treating the two closed E1 helpers
  as proof that whole lookup groups, aggregate `PreparedFunctionLookups`, or
  public fallback/oracle APIs can be deleted.
- Step 3 accepts only
  `find_prepared_control_flow_branch_target_labels(...)` and
  `find_prepared_fused_compare_operand_producer_facts(...)` as ready for Step 4
  draft consideration; Step 4 created exactly those two follow-up ideas.
- `call_plans`, `memory_accesses`, `edge_publications`,
  `edge_publication_source_producers`, mixed Route 1/2/5/6/7
  source-producer/printer/oracle rows, `move_bundles`, and `value_homes` are
  not ready for direct E2 contraction.
- Draft 155 / E5 and aggregate `PreparedFunctionLookups` retirement remain
  unopened.

## Proof

Docs-only Step 4 packet. No build, ctest, or root-level log was required or
run. Proof is document/source-idea completeness:

- both new follow-up ideas are under `ideas/open/` and each is scoped to one
  helper family;
- each idea names the proven BIR/route semantic owner, retained prepared
  fallback/policy/oracle, public consumers still present, and proof required
  before privatization or deletion can be claimed;
- each idea includes concrete reviewer reject signals;
- the E2 durable document links both ideas and states that all other candidate
  surfaces remain retained, deferred, or no-action.
