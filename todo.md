# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory Rendered-String Semantic Authority

## Just Finished

Rolled the branch back to `20657e1e` and rewrote the active parser/Sema idea
away from classification/renaming work. The active contract is now removal:
each semantic rendered-string lookup route must be deleted, replaced by
structured metadata, or split into a metadata blocker idea.

## Suggested Next

Start with `src/frontend/parser/impl/core.cpp` and identify the first concrete
rendered-string semantic route to remove. Candidate hot spots are typedef and
value lookup paths that render or accept a spelling after a structured key was
available.

Suggested proof command for the first narrow parser packet:
`cmake --build build --target c4cll && ctest --test-dir build -R 'cpp_(positive_parser|positive_sema|negative_tests)' --output-on-failure`

## Watchouts

Do not count helper renaming, comment changes, wrapper extraction, or relabeling
as progress. Do not retain a semantic string recovery route as the final shape
of a packet. If the structured carrier is missing, open a new metadata idea
instead of preserving string rediscovery.

## Proof

Lifecycle-only rewrite. No code validation run.
