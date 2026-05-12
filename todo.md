Status: Active
Source Idea Path: ideas/open/194_bir_global_memory_provenance_linknameid_expansion.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory Remaining Global Provenance Routes

# Current Packet

## Just Finished

Lifecycle activation created this active runbook from idea 194 and aligned the
execution pointer to Step 1.

## Suggested Next

Execute Step 1: inventory remaining global memory/provenance routes, classify
string-keyed maps as route-local or semantic global identity, choose one
non-187 route for `LinkNameId` hardening, and record the first implementation
target here.

## Watchouts

- Do not re-prove only the addressed-global pointer route already closed by
  idea 187.
- Do not treat local route names, local slots, SSA temporaries, or block labels
  as semantic global symbols.
- Matching final spelling must not override mismatched `LinkNameId` metadata
  on the selected route.

## Proof

Lifecycle-only activation; no build or test proof required for this packet.
