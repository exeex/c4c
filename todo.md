Status: Active
Source Idea Path: ideas/open/849_lir_intrinsic_binding_evidence.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Binding Producers And Intrinsic Forms

# Current Packet

## Just Finished

Closed 848 as evidence complete with no direct 734 handoff, then activated
849 as the next ordered documentation successor.

## Suggested Next

Execute `plan.md` Step 1: inventory `LirInlineAsmValueBinding` producers and
intrinsic binding forms with concrete source locations and explicit
missing-evidence notes.

## Watchouts

This is documentation/evidence work only. Do not edit implementation files,
tests, expectations, unsupported markers, allowlists, runtime behavior, or
Raw-BIR receiver code. Do not parse inline-assembly templates or constraints
for binding, type, value, ABI, or dispatch facts.

## Proof

Required for this documentation packet: `git diff --check`, verification that
the required docs directory contains exactly the required files once created,
and any lightweight repository docs/link check if one exists.
