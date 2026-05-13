Status: Active
Source Idea Path: ideas/open/214_aarch64_aapcs64_call_return_frame_mir_contract.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Draft The AAPCS64 Target-MIR Contract

# Current Packet

## Just Finished

Step 2 added
`src/backend/mir/aarch64/AAPCS64_CALL_RETURN_FRAME_CONTRACT.md` as the
accepted target-MIR contract for AAPCS64 call, return, and frame boundaries.

The contract defines:

- direct-call and indirect-call responsibilities over `PreparedCallPlan` and
  `module::CallRecord`
- argument and result binding responsibilities over prepared call argument,
  result, `BeforeCall`, `AfterCall`, and `BeforeReturn` move/ABI-binding facts
- memory-return ownership through `PreparedMemoryReturnPlan` and the `x8`
  indirect result-location role
- `x16`/`x17` IP, PLT, veneer, and linker-sensitive roles as special call
  resources, not ordinary scratch or long-lived homes
- caller-saved clobber and live-across-call preservation responsibilities over
  prepared clobber and preserved-value records
- callee-save, frame slot, dynamic stack, frame-size, frame-alignment, and
  frame-pointer obligations over `module::FrameRecord` and related records
- stack argument and outgoing call-area minimum behavior, with total outgoing
  call-area and call-time alignment proof deferred when no structured carrier
  exists
- return-control responsibilities through return blocks, `BeforeReturn`
  movement, and the `x30` link-register role, while explicitly deferring a
  dedicated return-boundary record
- `x29` frame-pointer reservation policy through
  `FrameRecord::uses_frame_pointer_for_fixed_slots`
- minimum variadic behavior: preserve direct variadic wrapper kind and
  `PreparedCallPlan::variadic_fpr_arg_register_count`, while deferring full
  variadic function-entry save-area and `va_list` carriers

No implementation behavior, tests, gap ledger, codegen markdown consumers,
source idea, or plan file were changed.

## Suggested Next

Proceed to Step 3 with a docs-only packet that reconciles the gap ledger and
relevant markdown consumers so they point at
`AAPCS64_CALL_RETURN_FRAME_CONTRACT.md` instead of preserving hidden local ABI
policy.

## Watchouts

- Keep Step 3 limited to ledger and markdown-consumer reconciliation; do not
  start instruction selection or implementation edits from the new contract.
- The contract intentionally treats direct `CallRecord` exposure of
  `variadic_fpr_arg_register_count`, an outgoing call-area carrier, dedicated
  return-boundary metadata, full variadic function-entry metadata, and explicit
  special-register role records as deferred/narrow-record candidates.
- When updating legacy docs, preserve historical examples as descriptive only
  and make the new contract the authority for current target-MIR behavior.
- `module::CallRecord` currently retains full indirect-callee details and
  variadic FP argument count through `source_call`; Step 4 can add direct fields
  only if the supervisor wants machine-verifiable record exposure.

## Proof

Docs-only Step 2. No build required and no `test_after.log` produced per the
delegated proof. Stale-claim search and `git diff --check` were the required
proof for this packet.
