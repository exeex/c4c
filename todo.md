Status: Active
Source Idea Path: ideas/open/164_bir_call_use_source_annotation_schema.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Call-Use Source Surfaces

# Current Packet

## Just Finished

Lifecycle activation created the runbook for Step 1 of
`ideas/open/164_bir_call_use_source_annotation_schema.md`.

## Suggested Next

Execute Step 1: inventory prepared call argument/result source
materialization, direct-global dependency, memory/publication source, and
ABI-bound publication-routing surfaces, then record concrete BIR `CallInst` and
value schema targets, oracle helpers, positive and negative coverage, and the
narrow proof command for the first code-changing packet.

## Watchouts

- Keep Route 6 annotations target-neutral and semantic.
- Do not import ABI register names, stack slots, outgoing stack area sizing,
  variadic FPR counts, clobber/preservation sets, byval lane layout, scratch
  resources, helper/carrier protocol, destination homes, aggregate transport
  layout, or ABI/layout-bound publication-routing source-selection reads.
- Do not copy `PreparedCallArgumentPlan`, call result plans, outgoing-stack
  plans, or aggregate transport lane records as the BIR schema shape.
- Preserve explicit result, direct-global, memory/publication-source,
  wrong-call, missing-source, ABI-bound exclusion, and no-match cases where
  applicable.

## Proof

Activation only. No build or test proof required.
