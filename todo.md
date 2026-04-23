Status: Active
Source Idea Path: ideas/open/83_parser_scope_textid_binding_lookup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory parser binding tables for lexical lookup migration

# Current Packet

## Just Finished
Closed `ideas/open/85_parser_ctor_init_visible_head_probe_split.md` after
finishing the constructor-init visible-head seam split and reactivated the
broader lexical-scope `TextId` lookup route.

## Suggested Next
Execute step 1 by classifying the remaining parser binding tables into
`TextId`-native, direct-`TextId`, `LocalNameTable`, and structured-qualified
holdout buckets, then record the first narrow lexical migration packet here.

## Watchouts
Keep lexical scope lookup separate from namespace traversal. Do not reopen the
qualified-owner lookup slice completed under idea 84 while reactivating this
runbook.

## Proof
Not run. Lifecycle switch after closing idea 85.
