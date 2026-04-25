Status: Active
Source Idea Path: ideas/open/07_bir-local-gep-family-extraction.md
Source Plan Path: plan.md
Current Step ID: Steps 2-4
Current Step Title: Create local_gep.cpp, move local GEP members, preserve private index and build wiring

# Current Packet

## Just Finished

Steps 2-4 completed the mechanical local GEP extraction:
`src/backend/bir/lir_to_bir/memory/local_gep.cpp` now owns the twelve
inventory-listed local GEP member definitions plus the exclusive raw-byte-slice
helper, and `src/backend/bir/lir_to_bir/memory/local_slots.cpp` keeps local
slot declaration, alloca, local scalar-array helpers, load, and store behavior.

The explicit `backend_lir_to_bir_notes_test` source list in
`tests/backend/CMakeLists.txt` now includes `local_gep.cpp`; the main backend
library picks it up through the existing glob.

## Suggested Next

Have the supervisor review the extraction diff against the plan and commit the
coherent code plus `todo.md` slice if accepted.

## Watchouts

No behavior or expectation changes were made. `plan.md`,
`ideas/open/07_bir-local-gep-family-extraction.md`, and
`ideas/open/08_bir-address-projection-model-consolidation.md` were not edited.

## Proof

Ran the supervisor-selected proof command and wrote combined stdout/stderr to
`test_after.log`:

```sh
cmake --build --preset default --target c4c_codegen c4c_backend c4cll backend_lir_to_bir_notes_test && ctest --test-dir build -j --output-on-failure -R 'backend_codegen_route_x86_64_(nested_member_pointer_array|local_dynamic_member_array|local_dynamic_member_array_store|local_direct_dynamic_member_array_store|local_direct_dynamic_member_array_load|local_direct_dynamic_struct_array_call)_observe_semantic_bir'
```

Result: passed. All six selected semantic BIR tests passed.
