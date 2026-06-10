# Current Packet

Status: Active
Source Idea Path: ideas/open/157_bir_call_boundary_source_facts.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Switch one call-source consumer at a time

## Just Finished

Step 6 completed the next call-source consumer switch. The AArch64 scalar
call-argument source-producer materialization path now reads the semantic
producer fact from `bir::find_call_argument_source_producer_materialization`
for the matching call argument when a BIR call-argument relationship is
available, and falls back to the prepared source-producer lookup when BIR facts
are unavailable.

The switch is scoped to that single scalar call-argument producer read.
Prepared call plans still provide call instruction identity, argument index
iteration, prepared value-home/register authority, recursive operand fallback,
and all ABI placement/final call lowering facts.

`backend_aarch64_instruction_dispatch_test.cpp` now covers the switched path by
clearing the prepared source-producer lookup for a scalar call argument,
seeding the matching BIR call-argument relationship, and proving the existing
pre-call scalar add materialization is emitted from the BIR fact.

## Suggested Next

Supervisor should review this source-producer switch and then decide the next
single Step 6 consumer packet, likely publication-source routing or another
remaining call-source read, with the same prepared fallback/ABI-authority
boundary.

## Watchouts

- BIR production lowering still does not mint prepared numeric value ids;
  consumers that require `PreparedValueId` must continue to resolve ids through
  prepared name/value-location tables or add an explicit parity bridge.
- This Step 6 switch converts the BIR source-producer fact into a local
  AArch64 materialization payload only at the scalar call-argument boundary; do
  not treat that as BIR ownership of ABI placement or prepared numeric ids.
- Recursive operand materialization intentionally keeps using the prepared
  fallback path because those operands are not themselves call arguments with
  argument-index relationships.
- The result identity surface from Step 4 remains value-identity-only and must
  not be treated as result ABI placement authority.

## Proof

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`

Passed. `test_after.log` contains the successful build and backend test run:
179 backend tests passed, 0 failed.
