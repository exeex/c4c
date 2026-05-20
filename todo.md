Status: Active
Source Idea Path: ideas/open/328_aarch64_byval_aggregate_call_argument_lane_publication.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Focused Backend Coverage

# Current Packet

## Just Finished

Step 3 added focused backend coverage in
`tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`.

The new dispatch case exercises a generic before-call move whose BIR call ABI
marks a one-byte pointer argument as integer-class `byval`, passed in an
AArch64 register. The aggregate object's ordinary stack home is intentionally
poisoned away from the prepared byte store, so the test distinguishes payload
lane publication from forwarding or loading through the prepared object
address.

Coverage now asserts that lowering selects the prepared payload byte store,
marks the move as `call_arg_byval_aggregate_register_lanes`, does not
materialize an address, and prints `ldrb w0, [sp, #128]`. It would reject the
old `add x0, sp, #...` address-forwarding shape and the wrong-source
`ldrb` fallback from the aggregate object home.

## Suggested Next

Execute Step 4 from `plan.md`: prove representative `00204` progress and
record whether any next first bad fact remains after the byval payload-lane
repair.

## Watchouts

- The new coverage is semantic and does not mention `00204`, `fa_s1`, source
  names from the external case, exact scratch registers, or generated stack
  offsets from the representative.
- Existing adjacent guards in the delegated proof preserved target instruction
  records, dispatch coverage, prepared-BIR handoff, byval helper runtime
  payload cases, and `00204`.
- The covered owner remains small integer-class register-passed `byval`
  aggregate payload publication. HFA/floating byval and larger indirect byval
  address passing remain separate owners unless fresh evidence ties them here.

## Proof

Ran the delegated proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_(target_instruction_records|instruction_dispatch)|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication|backend_runtime_byval_helper_payload_(8_to_13|9_to_14)|c_testsuite_aarch64_backend_src_00204_c)$' | tee test_after.log`

Result: build succeeded and all 6/6 selected tests passed. Proof log preserved
at `test_after.log`.
