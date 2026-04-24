Status: Active
Source Idea Path: ideas/open/03_bir-memory-coordinator-dispatch-split.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Split GEP Handling

# Current Packet

## Just Finished

Completed `Step 3: Split GEP Handling` by moving `LirGepOp` lowering out of
the memory coordinator into the private GEP-family handler
`BirFunctionLowerer::lower_memory_gep_inst` in `addressing.cpp`. The coordinator
now delegates GEP instructions through a single dispatch call while the handler
keeps local aggregate/local slot, global, dynamic global, dynamic local, and
raw pointer projection branches explicit.

## Suggested Next

Execute `Step 4: Split Load And Store Handling` from `plan.md`.

## Watchouts

- Do not create new `.hpp` files.
- Keep `lowering.hpp` as the complete private `BirFunctionLowerer` index.
- Keep memory state ownership on `BirFunctionLowerer`.
- Split by real instruction-family boundaries, not one file per narrow case.
- Preserve behavior and diagnostics; do not rewrite expectations as proof.
- `lower_scalar_or_local_memory_inst` still owns the address-int/provenance cast
  branches; those were intentionally left for the later address-int or memory
  families.
- GEP lowering now lives in `addressing.cpp`; keep future load/store extraction
  from pulling GEP policy back into the coordinator.
- `lower_memory_gep_inst` intentionally preserves the existing local/global/
  dynamic branch ordering and state mutations on `BirFunctionLowerer`.

## Proof

`cmake --build --preset default --target c4c_backend && ctest --test-dir build -j --output-on-failure -R '^backend_'`
passed: 97 backend tests passed, 0 failed, 12 disabled not run. Proof log:
`test_after.log`. `git diff --check` passed.
