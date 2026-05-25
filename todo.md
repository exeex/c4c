Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Remove The Selected Local Decision

# Current Packet

## Just Finished

Step 2 removed the selected local AArch64 size decision for
`outgoing_stack_argument_bytes`.

- Added `PreparedCallArgumentPlan::destination_stack_size_bytes` as the prepared
  outgoing stack argument extent authority.
- Populated the extent during prepared call planning from the call ABI and
  threaded it into prepared call-boundary effect endpoints.
- Updated prepared printers to expose the destination stack size contract.
- Changed AArch64 `outgoing_stack_argument_bytes` to consume only prepared
  offset plus prepared size, removing its `bir::CallInst` parameter and the
  retained `call.arg_abi[argument.arg_index]` sizing read.
- Updated adjacent hand-built AArch64 instruction-dispatch fixtures to carry
  the prepared stack extent explicitly.

## Suggested Next

Have the supervisor review and commit this Step 2 slice, then select the next
retained call-emission authority leak for the following packet.

## Watchouts

The prepared stack size computation now lives in `call_plans.cpp` alongside
destination stack offset selection. Hand-built tests that expect outgoing stack
area setup must provide both `destination_stack_offset_bytes` and
`destination_stack_size_bytes`; AArch64 no longer reconstructs a missing size
from retained BIR.

## Proof

Ran the supervisor-selected proof exactly:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

Result: passing `162/162` `^backend_` tests. Proof log: `test_after.log`.
