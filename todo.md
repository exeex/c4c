# Current Packet

Status: Active
Source Idea Path: ideas/open/157_bir_call_boundary_source_facts.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Switch one call-source consumer at a time

## Just Finished

Step 6 completed the first call-source consumer switch. The AArch64
direct-global select-chain call-argument materialization path now reads the
semantic direct-global dependency from `bir::find_call_argument_publication_source_routing`
for the matching call argument when BIR publishes it, and falls back to the
prepared call-argument routing dependency when BIR facts are unavailable.

The switch is scoped to that single direct-global dependency read. Prepared
call plans still provide call instruction identity, source value id checks,
value-home/register authority, argument package index, and all ABI placement
facts.

`backend_aarch64_instruction_dispatch_test.cpp` now covers the switched path by
removing the prepared direct-global dependency from an existing dynamic selected
global call-argument fixture, seeding the equivalent BIR call-argument
relationship, and proving the selected value is still materialized before the
call.

## Suggested Next

Supervisor should review this first Step 6 consumer switch and then decide the
next single consumer packet, likely either source-producer materialization or
publication-source routing, with the same prepared fallback/ABI-authority
boundary.

## Watchouts

- BIR production lowering still does not mint prepared numeric value ids;
  consumers that require `PreparedValueId` must continue to resolve ids through
  prepared name/value-location tables or add an explicit parity bridge.
- This Step 6 switch converts the BIR direct-global dependency into the
  existing prepared-shaped dependency payload only at the local AArch64
  materialization boundary; do not treat that as BIR ownership of ABI placement
  or prepared numeric ids.
- `find_call_argument_source_producer_materialization` remains a producer query
  gated by relationship uniqueness and call identity, not by publication-routing
  availability.
- The result identity surface from Step 4 remains value-identity-only and must
  not be treated as result ABI placement authority.

## Proof

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`

Passed. `test_after.log` contains the successful build and backend test run:
179 backend tests passed, 0 failed.
