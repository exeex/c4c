# Current Packet

Status: Active
Source Idea Path: ideas/open/157_bir_call_boundary_source_facts.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Bridge production argument source facts and audit field scope

## Just Finished

Step 4 added a BIR-owned call-result source identity/query surface for
target-neutral result identity only. BIR now exposes primary result value
identity and result-lane value identity with call-instruction boundary checks;
it does not expose result register names, destination homes, ABI bindings,
late-publication alias booleans, stack slots, or move records.

`backend_prepared_lookup_helper_test.cpp` compares the BIR result identity
queries against `PreparedCallResultPlan::destination_value_id` and
`PreparedAfterCallResultLaneBinding` semantic identity facts. The tests cover
primary result identity, lane identity, lane-0 primary-result aliasing,
duplicate lane fail-closed behavior, ABI-only result calls, and detached or
wrong-index call boundaries. `backend_prealloc_call_boundary_classification_test.cpp`
keeps the prepared result ABI classification available while proving the new
BIR query does not reconstruct identity from result ABI placement.

## Suggested Next

Execute Step 5 before any Step 6 consumer switch. The next packet should add or
route production population for `bir::CallInst::arg_sources`, or add a
production BIR extraction/analysis bridge that derives equivalent semantic
argument source facts from lowered calls.

The packet must prove the bridge on real production-lowered calls rather than
only hand-built fixtures. It should compare against the prepared oracle for the
source-producer materialization, direct-global dependency, publication-source
routing, unavailable-source, scalar, stack/frame-slot, and nearby aggregate/byval
cases that expose semantic source identity.

The same packet should audit `CallArgumentSourceSelection` fields that look like
stack layout or addressing policy: `source_stack_offset_bytes`,
`source_size_bytes`, `source_align_bytes`,
`address_materialization_frame_slot_id`, and
`address_materialization_byte_offset`. Keep only fields that are documented or
enforced as target-neutral source identity in the BIR-owned consumer contract;
remove, quarantine, or leave prepared-only any ABI placement, stack layout,
addressing-form, or aggregate transport fact.

## Watchouts

- The result identity surface intentionally reports only `Value` identity,
  call instruction index, lane index, and whether a lane aliases the primary
  result. It must not be used as result ABI placement authority.
- Lane lookup is by named value identity and fails closed when the same lane
  value appears more than once, except for the canonical lane-0 alias where
  `result_lanes[0]` repeats the primary call result.
- No AArch64 or prealloc consumers were switched in this packet.
- Reviewer report `review/157_call_boundary_source_route_review.md` blocks
  Step 6 argument consumer switches until production BIR calls populate or
  extract argument source facts. Do not treat manually seeded `arg_sources`
  fixtures as production readiness proof.
- Keep AArch64/prealloc materialization and routing consumers on prepared
  authority during Step 5.

## Proof

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`

Passed. `test_after.log` contains the successful build and backend test run:
179 backend tests passed, 0 failed.
