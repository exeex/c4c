# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2.3
Current Step Title: Remove Remaining Parser Semantic Spelling And Fallback Authority

## Just Finished

Step 2.2 completed parser `VisibleNameResult` projection-overload cleanup by
deleting the remaining value/type/concept `std::string*` lookup projection
overloads. Tests now keep structured lookup results until explicit display
assertions through `visible_name_spelling(...)`.

Reviewer report `review/step2_projection_api_review.md` found that this was
legitimate API cleanup but not completion of rendered-spelling semantic lookup
removal: production parser code still has rendered-spelling semantic re-entry,
and semantic lookup APIs still expose spelling/fallback parameters.

## Suggested Next

Execute Step 2.3: remove remaining parser semantic spelling/fallback authority.
Start with the value-alias/visible-type route called out by the review:
`resolve_visible_type(...)` projects `lookup_using_value_alias(...)` through
`visible_name_spelling(...)`, recovers a `TextId`, and then drives
typedef/type lookup from that rendered spelling. The packet should delete that
semantic re-entry or replace it with structured metadata.

In the same route family, remove or replace spelling/fallback parameters on
parser semantic lookup APIs such as `lookup_using_value_alias`,
`lookup_value_in_context`, `lookup_type_in_context`, and
`lookup_concept_in_context` where structured carriers are available.

## Watchouts

The const-int blocker from commit `28c1e5c5` remains parked: deleting the
rendered-name `eval_const_int(...)` compatibility overload requires HIR
`NttpBindings` metadata migration. Parser-owned callers should stay on
`TextId` or structured maps, but do not pull `src/frontend/hir` carrier
migration into this parser packet; route that through
`ideas/open/140_hir_legacy_string_lookup_metadata_resweep.md` or a narrower HIR
metadata idea if the supervisor switches scope.

Do not claim projection-overload deletion as Step 2.3 completion. The next
packet must remove semantic authority from rendered spelling/fallbacks or
record a missing-metadata blocker; helper renames, wrapper moves, or display
projection cleanup alone are not enough.

`test_after.log` is currently missing in this working tree, so prior proof
claims from the completed projection-overload slice should not be treated as a
fresh canonical after log.

## Proof

Lifecycle-only rewrite after review; no code validation was run by the plan
owner.

Next executor packet should produce a fresh canonical `test_after.log` for the
supervisor-delegated proof command.
