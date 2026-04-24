Status: Active
Source Idea Path: ideas/open/03_bir-memory-coordinator-dispatch-split.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Split Runtime Memory Intrinsic Handling

# Current Packet

## Just Finished

Completed `Step 5: Split Runtime Memory Intrinsic Handling` by moving direct
`LirMemcpyOp` and `LirMemsetOp` lowering behind private runtime-memory family
handlers in `local_slots.cpp`, and by moving direct-call `memcpy`/`memset`
lowering behind `BirFunctionLowerer::try_lower_direct_memory_intrinsic_call`.
The coordinator now delegates those runtime memory paths through concise family
calls while reusing the existing immediate local memcpy/memset helpers.

## Suggested Next

Execute `Step 6: Validate Coordinator Boundary` from `plan.md`.

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
- Load/store lowering lives behind private family handlers in
  `local_slots.cpp`; runtime memcpy/memset lowering now does too, through the
  existing immediate local memcpy/memset helper boundary.
- `calloc` pointer-address inference remains in call dispatch because this
  slice only moved cohesive memcpy/memset lowering paths that already had local
  memory helper ownership.
- Provenance and alias mutations remain visible as calls to the existing
  provenance/local-slot/dynamic-array lowering paths; do not hide them behind
  generic catch-all helpers.

## Proof

`cmake --build --preset default --target c4c_backend && ctest --test-dir build -j --output-on-failure -R '^backend_'`
passed: 97 backend tests passed, 0 failed, 12 disabled not run. Proof log:
`test_after.log`. `git diff --check` passed.
