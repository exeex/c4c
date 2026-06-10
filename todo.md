# Current Packet

Status: Complete
Source Idea Path: ideas/open/157_bir_call_boundary_source_facts.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add the BIR call-use source relationship surface

## Just Finished

Step 2 added the first BIR-owned call-argument source relationship surface:
`bir::CallInst` now carries target-neutral `arg_sources` records keyed by
argument index, and BIR exposes fail-closed lookup/query helpers for scalar
argument source identity, source-selection identity, and publication-source
routing including direct-global select-chain dependency fields.

The slice kept AArch64 and prealloc consumers on their existing prepared
authority. It did not copy ABI placement, aggregate transport lanes, scratch,
preservation storage, variadic state, helper protocols, or final-lowering
fields into BIR. `backend_prepared_lookup_helper_test.cpp` now compares the new
BIR call-argument routing query against prepared oracle answers for register,
computed-address, frame-slot value, local-frame-address materialization, direct
global select-chain, unavailable, duplicate, and out-of-range cases.

## Suggested Next

Delegate the next narrow packet for the supervisor-selected call-boundary fact
family, likely expanding from call-argument routing into the next BIR-owned
source/result identity surface without switching AArch64 or prealloc consumers.

## Watchouts

- The new BIR surface is intentionally populated only by direct BIR records in
  this slice; no lowering, prealloc, or AArch64 consumer has been switched.
- `arg_sources` is semantic source identity only. Keep ABI register/stack
  placement, aggregate/byval transport policy, preservation/clobber storage,
  helper protocol state, and final call lowering outside this BIR surface.
- Duplicate source records for the same argument fail closed by design.
- Source base value name is available on the call-argument routing record, not
  on prepared source-selection records.

## Proof

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`

Passed. `test_after.log` contains the successful build and backend test run:
179 backend tests passed, 0 failed.
