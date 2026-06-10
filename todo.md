# Current Packet

Status: Active
Source Idea Path: ideas/open/157_bir_call_boundary_source_facts.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Bridge production argument source facts and audit field scope

## Just Finished

Step 5 completed the next production-backed call-argument source slice. BIR
direct call lowering now reconstructs same-block direct-global select-chain
dependencies for named call arguments and populates
`bir::CallArgumentSourceRelationship::direct_global_select_chain_dependency`
when the argument root is produced by a load-global/select/cast/binary chain.
The extractor fails closed on missing roots, non-named values, local-load-only
chains, and recursion depth exhaustion.

`backend_lir_to_bir_notes_test.cpp` now covers a production-lowered dynamic
global selected scalar passed directly as a call argument, proves the populated
direct-global select-chain dependency is exposed through
`find_call_argument_publication_source_routing`, and adds explicit
unavailable-source routing parity for production BIR call arguments. The
existing prepared lookup helper still compares prepared and BIR source routing,
direct-global select-chain, source-producer materialization, and unavailable
routing oracle answers.

No AArch64 or prealloc consumers were switched. Prepared-only stack/layout
fields remain quarantined from production BIR call source relationships.

## Suggested Next

Supervisor should review whether Step 5 is now acceptance-ready against the
source idea and decide whether to route a Step 6 consumer-switch packet or ask
for a reviewer/plan-owner pass first.

## Watchouts

- BIR production lowering still does not mint prepared numeric value ids;
  consumers that require `PreparedValueId` must continue to resolve ids through
  prepared name/value-location tables or add an explicit parity bridge.
- The production direct-global dependency extractor is intentionally BIR-owned
  and same-block only; it does not import prepared source-producer tables and it
  does not cross CFG edges.
- `find_call_argument_source_producer_materialization` remains a producer query
  gated by relationship uniqueness and call identity, not by publication-routing
  availability. Unavailable-source parity in this slice is covered on the
  publication-routing API.
- No AArch64 or prealloc consumers were switched in this packet.
- The result identity surface from Step 4 remains value-identity-only and must
  not be treated as result ABI placement authority.

## Proof

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`

Passed. `test_after.log` contains the successful build and backend test run:
179 backend tests passed, 0 failed.
