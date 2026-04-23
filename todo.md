Status: Active
Source Idea Path: ideas/open/84_parser_qualified_name_structured_lookup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory remaining qualified parser binding tables

# Current Packet

## Just Finished
Switched the active lifecycle from idea 83 to idea 84 so the remaining qualified/owner-scoped lookup work is tracked separately from unqualified lexical-scope migration.

## Suggested Next
Inventory the remaining qualified parser binding tables and classify which ones should move to structured keys versus remain compatibility-only for now.

## Watchouts
Do not pull unqualified lexical-scope migration back into this plan. The remaining scope here is qualified/owner-scoped lookup only.

## Proof
No code changes in this lifecycle packet.
