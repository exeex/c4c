Status: Active
Source Idea Path: ideas/open/223_phase_e2_prepared_lookup_api_private_pass_context_readiness.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Classify Readiness One Boundary at a Time

# Current Packet

## Just Finished

Completed plan.md Step 2: inventoried public consumers for each E2 candidate
surface in
`docs/bir_prealloc_fusion/phase_e2_prepared_lookup_api_private_pass_context_readiness.md`.
The inventory separates production, printer/debug, target-wrapper,
oracle/fallback/expected-string, aggregate/pass-context, and retained prepared
responsibility notes without readiness classification.

## Suggested Next

Start plan.md Step 3: classify each candidate one boundary at a time as ready
to draft, retained public fallback/oracle, retained target/prepared policy,
E1/E3/E4/Route 8/E5 deferred, blocked aggregate compatibility, or no-action.

## Watchouts

- This plan is analysis-only; do not edit implementation files.
- The Step 1 baseline explicitly prevents treating the two closed E1 helpers
  as proof that whole lookup groups, aggregate `PreparedFunctionLookups`, or
  public fallback/oracle APIs can be deleted.
- Step 2 intentionally kept consumer inventory separate from readiness
  classification; classification belongs to Step 3.
- Step 3 should not treat the inventory as permission to delete, privatize,
  hide aggregate lookup delivery, migrate wrappers, replace oracles, or change
  expected strings.
- Accepted follow-up ideas later must each name exactly one public prepared
  API boundary or one helper family.

## Proof

Docs-only Step 2 packet. No build, ctest, or root-level log was required or
run. Proof is document completeness in
`docs/bir_prealloc_fusion/phase_e2_prepared_lookup_api_private_pass_context_readiness.md`:
the Step 2 consumer inventory covers every Step 1 candidate and keeps
production, printer/debug, target-wrapper, oracle/fallback/expected-string,
aggregate/pass-context, and retained prepared responsibility notes separate
from Step 3 readiness classification.
