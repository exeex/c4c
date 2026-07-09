Status: Active
Source Idea Path: ideas/open/629_prepared_return_destination_home_authority.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Publish Or Verify Prepared Return Destination Homes

# Current Packet

## Just Finished

Step 3 added focused prepared-layer coverage for one RV64 pointer-typed stack-slot-to-register function return before producer changes.

The new `check_rv64_return_destination_home_contract()` fixture proves:

- the selected return source has pointer type, a nonzero prepared value identity, value name `tmp.return.home`, and `PreparedValueHomeKind::StackSlot` with slot/offset tied to 8-byte pointer frame-slot metadata
- the before-return move bundle is associated with `rv64_return_destination_home_contract`, block `0`, return instruction index `3`, phase `BeforeReturn`, and bundle `authority_kind=None`
- the semantic function return and terminator value are pointer-typed, matching the representative `source_type=ptr`, `destination_type=ptr` blocker shape
- the move has `from_value_id=to_value_id=tmp.return.home`, `destination_kind=FunctionReturnAbi`, `destination_storage_kind=Register`, `op_kind=Move`, `reason=return_stack_to_register`, no temp/parallel/immediate/destination-stack side channels, and `authority_kind=None`
- the destination ABI register facts are explicit: register name `a0`, occupied register `a0`, width `1`, GPR call-result placement slot `0`, and RV64 GPR target identity with physical index `10`
- `make_prepared_move_bundle_lookups(...)` and `find_prepared_before_return_abi_move_by_source_and_destination_bank(...)` recover the exact carrier by `(block_index, source_value_id, destination_bank)`

Coverage confirms the existing prepared carriers are sufficient for pointer source home, destination register placement, width, return ABI storage, function association, and lookup identity. It also confirms the first missing authority boundary remains explicit: both the bundle and move still publish `authority_kind=None`, so the next packet must add a narrow return-specific authority publication path before RV64 consumer admission.

## Suggested Next

Execute the next Step 3 producer packet: add a narrow return-specific authority publication path for assigned before-return `return_stack_to_register` moves whose source has a concrete stack-slot `PreparedValueHome` and whose destination is an explicit `FunctionReturnAbi` register placement. Keep the existing prepared carrier test green and add focused coverage that the move or adjacent return-authority carrier no longer relies only on `authority_kind=None`.

## Watchouts

The coverage fixture marks only the selected liveness value as home-slot required to exercise the existing stack-source carrier without changing producers; it should not become a production policy shortcut. The existing `plan_prepared_direct_global_return_authority(...)` helper is only for named global pointer constants already homed in a register; it is not the owner for these pointer stack-slot return rows. Do not infer destination homes from the RV64 return convention or final assembly. Keep scalar call/result transport, pointer stack-results, FPR policy, generic move-bundle authority, runtime triage, local/global repair, variadic/library policy, expectation changes, unsupported marker changes, allowlists, timeouts, and accounting outside this idea.

## Proof

Proof command:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`

Result: passed, 347/347 backend tests.

Log path: `test_after.log`.
