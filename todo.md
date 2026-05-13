Status: Active
Source Idea Path: ideas/open/206_prepared_memory_volatility_address_space_carrier.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove Fact Preservation

# Current Packet

## Just Finished

Completed Step 3, `Prove Fact Preservation`.

Reviewed the Step 2 focused preservation coverage and found no missing
representative assertion for this step.

Existing direct BIR fixtures already prove typed prepared-field preservation
for:
- volatile and non-default address-space direct frame-slot accesses
- volatile and non-default address-space global-symbol accesses
- volatile and non-default address-space string-constant accesses
- volatile and non-default address-space pointer-indirect accesses
- default and explicit `PreparedMemoryAccess` designated-initializer behavior

No test changes were needed for Step 3.

## Suggested Next

Execute Step 4 from `plan.md`: update the prepared carrier documentation to
name `PreparedMemoryAccess::address_space` and
`PreparedMemoryAccess::is_volatile` as the shared typed carrier fields, while
leaving target-local memory lowering out of scope.

## Watchouts

- The carrier now preserves BIR-provided facts, but current LIR/frontend memory
  producers still do not appear to publish volatile or non-default
  address-space facts for ordinary loads/stores.
- Target MIR lowering remains out of scope for this idea; consumers should read
  the typed prepared fields rather than infer from names, dumps, or target-local
  patterns.
- No expectation downgrades or unsupported-test rewrites were made.

## Proof

Command:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepare_stack_layout$'; } > test_after.log 2>&1`

Result: pass.

Proof log: `test_after.log`.
