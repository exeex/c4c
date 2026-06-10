# Current Packet

Status: Complete
Source Idea Path: ideas/open/157_bir_call_boundary_source_facts.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove prepared-oracle equivalence for argument source facts

## Just Finished

Step 3 extended the BIR-owned call-argument source surface with
same-block source-producer materialization facts. BIR now exposes a
target-neutral query for call argument source producers that carries producer
identity, instruction index, produced value identity, and materializable
eligibility only.

`backend_prepared_lookup_helper_test.cpp` now compares the BIR query against
`PreparedCallArgumentSourceProducerMaterialization` for load-local,
materializable binary opcode, nonmaterializable binary opcode, missing
producer, producer-after-call, and duplicate source-record fail-closed paths.
No AArch64 or prealloc consumers were switched, and no ABI placement/final
lowering fields were added.

## Suggested Next

No remaining Step 3 gaps are known inside this packet. The next coherent packet
is supervisor-owned: either accept/commit this BIR query and oracle proof slice
or choose the next call-boundary source/result identity surface while keeping
consumers on prepared authority.

## Watchouts

- The new BIR query requires the `CallInst` to be the instruction at the
  provided same-block call index and requires exactly one `arg_sources` record
  for the argument; duplicates fail closed.
- Nonmaterializable binary producers are reported as available producers with
  `materializable=false`, while the prepared materialization oracle remains
  absent; the test comparator checks equivalence through the materializable
  eligibility bit.
- The materialization surface currently recognizes load-local and binary
  producers only, matching the prepared oracle for this packet.

## Proof

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`

Passed. `test_after.log` contains the successful build and backend test run:
179 backend tests passed, 0 failed.
