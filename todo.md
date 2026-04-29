Status: Active
Source Idea Path: ideas/open/125_lir_legacy_string_lookup_removal_convergence.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory LIR String Lookup Surfaces

# Current Packet

## Just Finished

Activated `ideas/open/125_lir_legacy_string_lookup_removal_convergence.md` as
the active runbook.

## Suggested Next

Start Step 1 by inventorying `std::unordered_map<std::string, ...>`, string
comparison, rendered-name fallback, symbol matching, and lookup paths under
`src/codegen/lir`. Classify each surface before selecting the first bounded
conversion packet.

## Watchouts

Do not fold BIR verifier policy work or upstream HIR metadata gaps into this
LIR runbook. Split separate producer or consumer blockers into new open ideas.

## Proof

Lifecycle activation only; no build or test proof required.
