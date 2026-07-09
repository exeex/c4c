Status: Active
Source Idea Path: ideas/open/629_prepared_return_destination_home_authority.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Publish Or Verify Prepared Return Destination Homes

# Current Packet

## Just Finished

Step 3 published narrow prepared return destination-home authority for assigned
stack-slot-to-register function return moves.

The producer-side change added
`PreparedMoveAuthorityKind::FunctionReturnDestinationHome` and publishes it on
the new adjacent `PreparedMoveResolution::function_return_authority_kind`
carrier from `append_return_move_resolution(...)` only when the return move
already has: `source_kind=StackSlot`, `destination_kind=FunctionReturnAbi`,
`destination_storage_kind=Register`, an explicit destination ABI register name,
an explicit destination register placement, and a target register identity. The
generic move/bundle `authority_kind` remains `None` until Step 4 consumer
admission owns the RV64 object-emission surface; unrelated fan-in and widening
authority kinds were not reused.

The prepared carrier coverage still proves the selected pointer return source
has a concrete stack-slot `PreparedValueHome`, value identity
`tmp.return.home`, 8-byte frame-slot metadata, `return_stack_to_register`
reason, RV64 `a0` GPR call-result placement/target identity, and source/bank
lookup identity. It now also proves the before-return move bundle and its move
preserve generic `authority_kind=None` for current consumers while the move
publishes adjacent `FunctionReturnDestinationHome` return authority.

## Suggested Next

Execute Step 4 consumer admission: admit RV64 object emission only for
before-return `return_stack_to_register` moves that carry explicit
`FunctionReturnDestinationHome` authority and matching prepared source home,
destination register placement, width, phase, destination kind/storage, and
return association. Add positive and fail-closed object-emission coverage.

## Watchouts

The existing `plan_prepared_direct_global_return_authority(...)` helper is only
for named global pointer constants already homed in a register; it is not the
owner for pointer stack-slot return rows. Step 4 should consume the new
prepared authority, not infer destination homes from the RV64 return convention
or final assembly. Keep scalar call/result transport, pointer stack-results, FPR
policy, generic move-bundle authority, runtime triage, local/global repair,
variadic/library policy, expectation changes, unsupported marker changes,
allowlists, timeouts, and accounting outside this idea.

## Proof

Proof command:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`

Result: passed, 347/347 backend tests.

Log path: `test_after.log`.
