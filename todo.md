# Current Packet

Status: Active
Source Idea Path: ideas/open/157_bir_call_boundary_source_facts.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Bridge production argument source facts and audit field scope

## Just Finished

Step 5 partially populated production-lowered `bir::CallInst::arg_sources`
from BIR lowering-side semantic facts. BIR call argument source relationships
now carry native `source_value_name` identity, immediate/register/symbol/
computed/frame encodings, and computed global or pointer base/delta facts
without requiring prepared numeric value ids.

`backend_lir_to_bir_notes_test.cpp` now proves a real lowered direct call with
scalar, immediate, symbol-address, computed global-address, and aggregate/byval
arguments. The test exercises `find_call_argument_source_relationship`,
`find_call_argument_publication_source_routing`, and
`find_call_argument_source_producer_materialization` on the production-lowered
call and verifies production BIR does not populate stack-layout-shaped
`CallArgumentSourceSelection` fields.

This is not full Step 5 completion under `plan.md`: production-backed
direct-global select-chain dependency, unavailable-source parity, and explicit
prepared-oracle comparison coverage for all Step 5 argument-source families are
still missing. Step 6 consumer switches are not acceptance-ready yet.

`CallArgumentSourceSelection` now documents the prepared-only quarantine for
`source_stack_offset_bytes`, `source_size_bytes`, `source_align_bytes`,
`address_materialization_frame_slot_id`, and
`address_materialization_byte_offset`; production BIR lowering identifies
source values and slots but leaves ABI placement/layout policy to prepared
consumers.

## Suggested Next

Continue Step 5. The next packet should add production-backed
direct-global-select-chain dependency extraction/population or a documented
BIR-owned fail-closed analysis path, then prove it against the prepared oracle.
The same packet should add unavailable-source parity coverage and compare
production BIR source answers with prepared call-plan oracle answers for the
remaining source families that the plan names.

Keep AArch64/prealloc consumers on prepared authority. Do not start Step 6 until
the Step 5 completion check is satisfied or the plan owner explicitly narrows
the Step 5 contract.

## Watchouts

- BIR production lowering still does not mint prepared numeric value ids;
  consumers that require `PreparedValueId` must continue to resolve ids through
  prepared name/value-location tables or add an explicit parity bridge.
- `direct_global_select_chain_dependency` remains a prepared-only dependency
  fact in this slice. The production BIR bridge covers direct symbol-address
  source identity and computed base/delta facts, but it does not reconstruct the
  select-chain dependency query; this is the main remaining Step 5 gap.
- The current production test proves BIR query availability on lowered calls,
  but it is not yet the full prepared-oracle equivalence suite required by the
  Step 5 completion check.
- No AArch64 or prealloc consumers were switched in this packet.
- The result identity surface from Step 4 remains value-identity-only and must
  not be treated as result ABI placement authority.

## Proof

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`

Passed. `test_after.log` contains the successful build and backend test run:
179 backend tests passed, 0 failed.
