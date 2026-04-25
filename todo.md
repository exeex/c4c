Status: Active
Source Idea Path: ideas/open/05_bir-memory-intrinsic-family-extraction.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Extract `memory/intrinsics.cpp`

# Current Packet

## Just Finished

Step 2: Extract `memory/intrinsics.cpp` completed as a behavior-preserving
mechanical move.

- Created `src/backend/bir/lir_to_bir/memory/intrinsics.cpp`.
- Moved the intrinsic-only anonymous structs `LocalArrayView`,
  `LocalMemcpyLeaf`, `LocalMemcpyLeafView`, `LocalMemcpyScalarSlot`, and
  `LocalMemsetScalarSlot` into `intrinsics.cpp`.
- Moved the five target member definitions into `intrinsics.cpp`:
  `BirFunctionLowerer::try_lower_immediate_local_memset`,
  `BirFunctionLowerer::try_lower_immediate_local_memcpy`,
  `BirFunctionLowerer::lower_memory_memcpy_inst`,
  `BirFunctionLowerer::lower_memory_memset_inst`, and
  `BirFunctionLowerer::try_lower_direct_memory_intrinsic_call`.
- Left `LocalAggregateRawByteSliceLeaf`,
  `resolve_local_aggregate_raw_byte_slice_leaf`, and the local-slot-only
  lowering code in `local_slots.cpp`.
- Added `memory/intrinsics.cpp` to the manual
  `backend_lir_to_bir_notes_test` source list in
  `tests/backend/CMakeLists.txt`.

## Suggested Next

Supervisor packet: run the Step 4 acceptance review/check for this extraction,
including rechecking the diff for no header additions, no expectation changes,
and no semantic lowering edits beyond implementation placement.

## Watchouts

- No new headers were added.
- No testcase files or expectation snippets were edited.
- The moved definitions remain `BirFunctionLowerer` member definitions.
- `intrinsics.cpp` includes `../lowering.hpp` and did not need
  `memory_helpers.hpp`.
- `src/backend/CMakeLists.txt` already picks up the new source through the
  backend source glob; only the backend notes test needed manual source-list
  registration.

## Proof

Passed. Proof log: `test_after.log`.

Command:

```sh
bash -lc "cmake --build build --target c4c_backend backend_lir_to_bir_notes_test c4cll -j && ctest --test-dir build -R '^(backend_lir_to_bir_notes|backend_codegen_route_x86_64_builtin_mem(cpy|set).+_observe_semantic_bir)$' --output-on-failure" > test_after.log 2>&1
```
