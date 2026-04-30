# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2.1
Current Step Title: Remove Parser Template And NTTP Default Rendered Mirrors

## Just Finished

Plan review split the oversized parser Step 2 route after
`review/step2_parser_lookup_removal_review.md` found the implementation
source-aligned but too broad for the next executor packet. The runbook now
breaks parser cleanup into numbered Step 2 substeps and keeps the next route
focused on template and NTTP-default rendered mirrors.

## Suggested Next

Execute Step 2.1 only: inspect parser template-state lookup and NTTP-default
token/evaluation paths, remove rendered mirror authority where
`QualifiedNameKey`, namespace context, `TextId`, or structured NTTP-default
keys are already available, and add focused drifted-rendered-spelling parser
tests for that route.

## Watchouts

The const-int blocker from commit `28c1e5c5` remains parked: deleting the
rendered-name `eval_const_int(...)` compatibility overload requires HIR
`NttpBindings` metadata migration. Parser-owned callers should stay on
`TextId` or structured maps, but do not pull `src/frontend/hir` carrier
migration into this parser packet; route that through
`ideas/open/140_hir_legacy_string_lookup_metadata_resweep.md` or a narrower HIR
metadata idea if the supervisor switches scope.

The missing canonical proof log noted by the reviewer is not accepted by this
lifecycle rewrite. The next code slice must produce a fresh `test_after.log`
for its delegated proof command before acceptance.

## Proof

Not run. This was a lifecycle-only plan/todo rewrite and does not claim a
fresh build or CTest result.
