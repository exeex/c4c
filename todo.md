Status: Active
Source Idea Path: ideas/open/557_bir_local_memory_semantic_producer_admission.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Pin The Selected Admission Contract

# Current Packet

## Just Finished

Step 6 was attempted and is blocked. The testcase/process-shaped helper name
and RV64 exclusion from the first slice were removed. A revised neutral
producer rule was also tried: publish local-slot `MemoryAddress` facts for
direct scalar store/load accesses to local alloca aggregate-field slots, with
RV64 enabled and byval parameter copy slots excluded by slot ownership rather
than target.

That general rule pinned the intended `backend_lir_to_bir_notes_test.cpp`
contract, but it broke existing prepared/codegen route identity expectations.
Because the route-consumer repair is larger than this Step 6 coverage packet,
the implementation and focused test additions were removed from the worktree.

## Suggested Next

Route review or split the next packet before implementing Step 7. The next
coherent packet should decide how prepared/route consumers should treat
producer-published direct local-slot address facts without changing existing
byval aggregate and source-identity contracts.

## Watchouts

Exact failing evidence from the neutral general-rule attempt:
- `backend_codegen_route_riscv64_byval_aggregate_fixed_call` failed with
  missing snippet `lw t3, 32(sp)`.
- `backend_store_source_publication_plan` failed with `expected BIR load-local
  source identity to match prepared oracle`.
- `backend_aarch64_prepared_scalar_alu_records` failed with `expected Route
  3/prepared source mismatch to reject source-home operand`.
- `backend_prepared_lookup_helper` failed with `BIR load-local memory identity
  should match prepared semantic fields`.

These failures show that publishing direct local-slot address facts is not only
a producer-side contract; route/prepared consumers currently attach meaning to
the presence of those facts.

## Proof

Command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`

Result after removing the blocked implementation/test additions: passed,
`345/345` backend tests. Proof log: `test_after.log`.
