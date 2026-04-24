Status: Active
Source Idea Path: ideas/open/03_bir-memory-coordinator-dispatch-split.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Split Load And Store Handling

# Current Packet

## Just Finished

Completed `Step 4: Split Load And Store Handling` by moving `LirLoadOp` and
`LirStoreOp` lowering out of the memory coordinator into private load/store
family handlers `BirFunctionLowerer::lower_memory_load_inst` and
`BirFunctionLowerer::lower_memory_store_inst` in `local_slots.cpp`. The
coordinator now delegates load/store instructions through single dispatch calls,
while the handlers keep pointer provenance updates, local slot materialization,
dynamic pointer arrays, dynamic scalar globals, and dynamic local aggregate
load/store behavior explicit.

## Suggested Next

Execute `Step 5: Split Runtime Memory Intrinsic Handling` from `plan.md`.

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
- Load/store lowering now lives behind private family handlers in
  `local_slots.cpp`; keep the remaining runtime intrinsic split from folding
  `memcpy`/`memset` policy into generic load/store helpers.
- Provenance and alias mutations remain visible as calls to the existing
  provenance/local-slot/dynamic-array lowering paths; do not hide them behind
  generic catch-all helpers.

## Proof

`cmake --build --preset default --target c4c_backend && ctest --test-dir build -j --output-on-failure -R '^backend_'`
passed: 97 backend tests passed, 0 failed, 12 disabled not run. Proof log:
`test_after.log`. `git diff --check` passed.
