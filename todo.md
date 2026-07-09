Status: Active
Source Idea Path: ideas/open/629_prepared_return_destination_home_authority.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add Narrow RV64 Return Consumer Admission

# Current Packet

## Just Finished

Step 4 added narrow RV64 object-emission admission for explicit prepared
return destination-home authority on before-return stack-slot-to-register
function return moves.

The consumer now emits the selected pointer return load only when the prepared
move carries `FunctionReturnDestinationHome` on
`function_return_authority_kind`, keeps generic move/bundle authority as
`None`, has phase `BeforeReturn`, reason `return_stack_to_register`,
destination kind `FunctionReturnAbi`, register storage, a stack-slot
`PreparedValueHome` source, matching destination register name, occupied name,
placement, and target identity for RV64 `a0`, width 1, matching block and
instruction association, and no immediate, parallel-copy, cycle-temp, or
destination-stack side channels.

Focused RV64 object-emission coverage now proves the authorized pointer return
loads from the prepared stack home and returns without an extra generic pointer
return load. The same coverage rejects missing or unrelated return authority,
missing/non-stack source home, wrong destination kind/storage/reason/phase,
missing or mismatched destination name/placement/target identity/occupied
names, wrong width, malformed block/instruction association, side channels, and
generic move-bundle authority.

## Suggested Next

Execute Step 5 representative row reclassification for
`src/20001130-2.c` and `src/20080719-1.c` to decide whether idea 629 is
close-ready or whether another return-authority packet is justified.

## Watchouts

The direct-global pointer return path remains preserved and separately owned;
the new admission is only for explicit prepared stack-slot return authority.
Step 5 should classify any residual representative-row failures without
broadening into scalar call/result transport, pointer stack-results, FPR policy,
generic move-bundle authority, runtime triage, local/global repair,
variadic/library policy, expectation changes, unsupported marker changes,
allowlists, timeouts, or accounting.

## Proof

Proof command:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`

Result: passed, 347/347 backend tests.

Log path: `test_after.log`.
