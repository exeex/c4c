Status: Active
Source Idea Path: ideas/open/109_hir_to_lir_struct_layout_lookup_by_struct_name_id.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Existing Layout Lookup Surfaces

# Current Packet

## Just Finished

Activation created the runbook for `plan.md` Step 1 from
`ideas/open/109_hir_to_lir_struct_layout_lookup_by_struct_name_id.md`.

## Suggested Next

Start Step 1 by auditing the HIR-to-LIR layout lookup surfaces named in
`plan.md`, then update this file with the first bounded implementation packet.

## Watchouts

Keep `TypeSpec::tag` and `Module::struct_defs` fallback intact. Do not change
emitted LLVM output, aggregate layout, ABI behavior, or printer authority in
this plan.

## Proof

Lifecycle-only activation; no code validation run.
