Status: Active
Source Idea Path: ideas/open/557_bir_local_memory_semantic_producer_admission.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Semantic Fact Publication

# Current Packet

## Just Finished

Step 3 - Repair Semantic Fact Publication completed the first local-memory
producer repair. Added `local_slot_access_provenance()` and used it at the
local-slot `MemoryAddress` publication sites for byte-array scalar load/store,
local-slot pointer load/store, and local memcpy local-slot access paths. Added
focused BIR coverage in
`expect_local_byte_array_scalar_access_publishes_local_slot_provenance()` to pin
LocalSlot provenance identity, requested byte range, known object extent,
layout authority, and in-bounds verdict on producer-emitted local-slot memory
addresses.

## Suggested Next

Run Step 4 with a narrow RV64 gcc_torture subset selected by the supervisor to
check whether the representative local-memory rows advance for producer-owned
reasons.

## Watchouts

- This packet did not touch semantic admission checks, expectations,
  unsupported markers, allowlists, runtime comparison behavior, or downstream
  prepared/RV64 consumers.
- Direct same-slot scalar load/store instructions that do not publish a
  `MemoryAddress` remain intentionally outside this first repair; the repaired
  boundary is the existing local-slot address publication sites.
- The helper records complete extent only where the producer already knows the
  local object extent, such as fixed byte-array and memcpy scalar-slot paths.

## Proof

Ran exactly `cmake --build --preset default` followed by
`ctest --test-dir build -j --output-on-failure -R '^backend_'`, with combined
output recorded in `test_after.log`. Result: build succeeded and the backend
CTest subset passed.
