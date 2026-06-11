Status: Active
Source Idea Path: ideas/open/181_phase_d_mir_consumer_bir_view_switch_plan.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Gather Phase A-C and Route Closure Evidence

# Current Packet

## Just Finished

Step 1 from `plan.md` is complete. Created
`docs/bir_prealloc_fusion/phase_d_mir_consumer_switch_plan.md` with a concise
evidence baseline table covering Phase A, Phase B, Phase C, route closures
167-174, and return-chain as a no-route gap for this active Phase D plan.

## Suggested Next

Execute Step 2 from `plan.md`: map concrete MIR/codegen consumer families that
still read `PreparedBirModule`, `PreparedFunctionLookups`, or domain
`Prepared*` query structs, then classify each dependency with the four-category
working model.

## Watchouts

- Phase D is analysis-only; do not edit backend implementation files.
- Do not treat selected route migrations from ideas 167-174 as full prepared
  API contraction.
- Keep return-chain lookup as a no-route gap unless a separate owner/schema
  idea exists.
- Preserve the Phase C residual-consumer findings: most prepared helpers remain
  public or fallback oracles until route-specific consumer equivalence is proven.

## Proof

Docs-only analysis packet; no build/test required by the delegated proof. Local
verification checked that
`docs/bir_prealloc_fusion/phase_d_mir_consumer_switch_plan.md` contains the
Step 1 evidence table and records Phase A, Phase B, Phase C, routes 167-174,
and return-chain as a no-route gap.
