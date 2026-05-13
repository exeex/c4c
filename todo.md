Status: Active
Source Idea Path: ideas/open/214_aarch64_aapcs64_call_return_frame_mir_contract.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Align Narrow Record Surface If Required

# Current Packet

## Just Finished

Step 4 assessed whether the accepted AAPCS64 contract needs immediate narrow
AArch64 module record-surface changes. No code change is required for this
plan step.

The current contract is representable over existing records and prepared facts:
`CallRecord` preserves call site identity, direct/indirect kind, wrapper kind,
arguments, results, memory-return slot snapshots, preserved values, clobbers,
and `source_call` provenance; `CallArgumentRecord` and `CallResultRecord`
preserve register/stack/source/destination snapshots; `MoveRecord` and
`AbiBindingRecord` preserve `BeforeCall`, `AfterCall`, and `BeforeReturn`
movement authority; `FrameRecord`, `FrameSlotRecord`, `CalleeSaveRecord`, and
`DynamicStackRecord` preserve frame, callee-save, fixed-slot frame-pointer, and
dynamic-stack obligations; `BlockRecord` preserves return terminator identity;
and AArch64 ABI helpers identify `x8`, `x16`, `x17`, `x29`, and `x30`
special-register roles.

The only narrower carriers not directly exposed remain documented deferrals:
a dedicated return-boundary record, direct `CallRecord` exposure of
`variadic_fpr_arg_register_count`, a complete outgoing call-area carrier,
explicit special-register role records, full variadic function-entry metadata,
and broader target ABI classification/helper-call carriers. None blocks this
contract because consumers can use existing prepared provenance and must fail
closed or open a separate idea before relying on those deferred facts.

## Suggested Next

Proceed to Step 5 proof. Since Step 4 was assessment-only and made no record
changes, proof can focus on stale-claim searches for old local ABI policy plus
`git diff --check`; no focused structured test edits are required unless the
supervisor chooses to add machine-verifiable coverage for a deferred carrier.

## Watchouts

- Step 4 intentionally did not edit `module.hpp`, `module.cpp`, tests, docs,
  `plan.md`, source ideas, or review artifacts.
- `PreparedAbiBinding` still has no destination value id or slot id; current
  `AbiBindingRecord` therefore exposes only direct prepared ABI destination
  facts and source binding provenance.
- `module::CallRecord` keeps full indirect-callee details and
  `variadic_fpr_arg_register_count` through `source_call`, not direct fields.
- A future implementation packet for direct `CallRecord` variadic count,
  outgoing-area totals, or return-boundary records should include focused
  module tests and backend proof.

## Proof

Assessment-only Step 4. No build required and no `test_after.log` produced per
the delegated proof. Required proof: bounded record-surface inspection and
`git diff --check`.
