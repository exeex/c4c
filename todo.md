Status: Active
Source Idea Path: ideas/open/214_aarch64_aapcs64_call_return_frame_mir_contract.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Reconcile Ledgers And Markdown Consumers

# Current Packet

## Just Finished

Step 3 reconciled `BIR_PREPARED_GAP_LEDGER.md` and relevant AArch64 codegen
markdown consumers with
`src/backend/mir/aarch64/AAPCS64_CALL_RETURN_FRAME_CONTRACT.md`.

The ledger now names the AAPCS64 contract as the owner for call, return,
frame, prologue, epilogue, memory-return, indirect-call, and variadic
target-MIR boundaries. It also records the remaining deferred carriers:
dedicated return-boundary records, direct variadic-count exposure, outgoing
call-area carriers, explicit special-register role records, and full variadic
function-entry carriers.

The calls, returns, prologue, variadic, memory, and target-record markdown now
treat legacy register names, stack layouts, scratch registers, save/restore
sequences, and ABI examples as historical reference only. Current consumers
must use prepared records, allocation-result records, and the AAPCS64 contract
instead of choosing argument/result registers, stack-argument placement,
special-register roles, frame layout, save/restore behavior, or return-control
policy locally.

## Suggested Next

Proceed to Step 4 with a bounded record-surface packet if the supervisor wants
direct machine-verifiable carriers for one of the deferred AAPCS64 facts, such
as direct `CallRecord` exposure of variadic FP argument count or a narrow
return-boundary record.

## Watchouts

- Step 3 was docs-only; no implementation files, tests, `plan.md`, source idea,
  or contract file were changed.
- The contract still intentionally treats direct `CallRecord` exposure of
  `variadic_fpr_arg_register_count`, an outgoing call-area carrier, dedicated
  return-boundary metadata, full variadic function-entry metadata, and explicit
  special-register role records as deferred/narrow-record candidates.
- `module::CallRecord` currently retains full indirect-callee details and
  variadic FP argument count through `source_call`; Step 4 can add direct fields
  only if the supervisor wants machine-verifiable record exposure.

## Proof

Docs-only Step 3. No build required and no `test_after.log` produced per the
delegated proof. Required proof: stale-claim searches for old local ABI policy
and `git diff --check`.
