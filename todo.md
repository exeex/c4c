Status: Active
Source Idea Path: ideas/open/223_phase_e2_prepared_lookup_api_private_pass_context_readiness.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Write Accepted Follow-Up Ideas

# Current Packet

## Just Finished

Completed plan.md Step 3: classified every E2 candidate surface in
`docs/bir_prealloc_fusion/phase_e2_prepared_lookup_api_private_pass_context_readiness.md`.
The readiness table covers each source-idea candidate, records the two proven
E1 helpers as ready for Step 4 draft consideration, keeps lookup-group and
mixed helper/oracle rows deferred or retained, and records explicit no-action
decisions for aggregate lookup construction plus target-policy,
fallback/oracle, wrapper, printer/debug, and expected-string surfaces.

## Suggested Next

Start plan.md Step 4: write accepted follow-up ideas only for the Step 3
ready-to-draft helper families, each scoped to exactly one public prepared API
boundary or helper family with retained fallback/policy/oracle and proof
requirements named.

## Watchouts

- This plan is analysis-only; do not edit implementation files.
- The Step 1 baseline explicitly prevents treating the two closed E1 helpers
  as proof that whole lookup groups, aggregate `PreparedFunctionLookups`, or
  public fallback/oracle APIs can be deleted.
- Step 3 accepts only
  `find_prepared_control_flow_branch_target_labels(...)` and
  `find_prepared_fused_compare_operand_producer_facts(...)` as ready for Step 4
  draft consideration.
- `call_plans`, `memory_accesses`, `edge_publications`,
  `edge_publication_source_producers`, mixed Route 1/2/5/6/7
  source-producer/printer/oracle rows, `move_bundles`, and `value_homes` are
  not ready for direct E2 contraction.
- Draft 155 / E5 and aggregate `PreparedFunctionLookups` retirement remain
  unopened.

## Proof

Docs-only Step 3 packet. No build, ctest, or root-level log was required or
run. Proof is document completeness in
`docs/bir_prealloc_fusion/phase_e2_prepared_lookup_api_private_pass_context_readiness.md`:
the Step 3 readiness table covers every candidate surface named in the source
idea, limits classifications to the source idea buckets, includes explicit
no-action/deferral statements for aggregate lookup construction and
target-policy/fallback/oracle surfaces, and states that draft 155 / E5 plus
aggregate `PreparedFunctionLookups` retirement remain unopened.
