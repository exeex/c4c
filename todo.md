Status: Active
Source Idea Path: ideas/open/124_hir_legacy_string_lookup_removal_convergence.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory HIR String Lookup Surfaces

# Current Packet

## Just Finished

Activation created the active runbook and execution scratchpad for Step 1.

## Suggested Next

Start Step 1 by inventorying `std::unordered_map<std::string, ...>`, string
comparison, rendered-name fallback, and symbol matching paths under
`src/frontend/hir`. Classify each finding as pure text lookup, semantic lookup,
diagnostic/display, dump/final spelling, compatibility bridge, or unresolved
boundary, then name the first bounded conversion packet.

## Watchouts

Do not treat rendered strings used for diagnostics, dumps, mangling, ABI/link
spelling, or compatibility payloads as cleanup failures. Do not weaken tests or
add named-case shortcuts; stale rendered-name tests may only be updated when
they assert legacy lookup authority instead of supported semantics.

## Proof

Lifecycle-only activation; no build or test proof required.
