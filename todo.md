Status: Active
Source Idea Path: ideas/open/126_bir_legacy_string_lookup_removal_convergence.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory BIR String Lookup Surfaces

# Current Packet

## Just Finished

Activation only. No executor packet has run yet.

## Suggested Next

Execute Step 1 from `plan.md`: inventory BIR string lookup surfaces in
`src/backend/bir`, classify retained strings, and identify the first bounded
conversion packet.

## Watchouts

Do not change parser, HIR, or LIR producer behavior as part of Step 1. If the
inventory exposes missing upstream metadata, record that as a separate open
idea rather than expanding this BIR route.

## Proof

Not run; lifecycle activation only.
