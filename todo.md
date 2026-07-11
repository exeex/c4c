Status: Active
Source Idea Path: ideas/open/694_bir_route_index_retirement_umbrella.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Digest Research Inputs

# Current Packet

## Just Finished

Step 1: Digest Research Inputs completed. Added
`docs/bir_route_index_retirement/research_digest.md`, consuming all nine idea
693 research input files and summarizing route inventory, BIR-to-prealloc
inputs, named view replacement shape, publication boundaries, retirement
sequence, test policy, follow-up recommendations, stale route-numbered API
assumptions, and the stack destination authority handoff.

## Suggested Next

Start Step 2 in `plan.md`: classify follow-up work by first owning layer and
dependency order before creating new source ideas.

## Watchouts

Keep route-numbered records compatibility-only until a named view or prepared
authority record owns the fact. Do not treat Route 4, Route 5, Route 7,
`RouteIndexReferenceFacade`, or dump rows as prepared publication, freshness,
move-bundle, stack destination, or MIR authority.

## Proof

Docs-only proof ran:
`test -f docs/bir_route_index_retirement/research_digest.md && rg -n "index.md|01_current_route_inventory|02_required_bir_to_prealloc_inputs|03_named_view_replacement_shape|04_publication_and_authority_boundaries|05_retirement_sequence|06_test_and_dump_policy_after_route_retirement|07_followup_idea_recommendations|08_stack_view_and_destination_authority_handoff|supersedes|research input" docs/bir_route_index_retirement/research_digest.md`

No build was required for this documentation-only packet. The delegated proof
does not write `test_after.log`; no root-level log file was created.
