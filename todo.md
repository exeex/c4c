Status: Active
Source Idea Path: ideas/open/03_bir-memory-coordinator-dispatch-split.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Split Alloca And Local-Slot Handling

# Current Packet

## Just Finished

Completed `Step 2: Split Alloca And Local-Slot Handling` by moving alloca
lowering, dynamic alloca address tracking, scalar local slot creation, scalar
array slot creation, and aggregate slot declaration behind the private
local-memory handler in `local_slots.cpp`. The memory coordinator now delegates
the alloca family through a single dispatch call and preserves the existing
alloca diagnostic/failure path.

## Suggested Next

Execute `Step 3: Split GEP Handling` from `plan.md`.

## Watchouts

- Do not create new `.hpp` files.
- Keep `lowering.hpp` as the complete private `BirFunctionLowerer` index.
- Keep memory state ownership on `BirFunctionLowerer`.
- Split by real instruction-family boundaries, not one file per narrow case.
- Preserve behavior and diagnostics; do not rewrite expectations as proof.
- `lower_scalar_or_local_memory_inst` still owns the address-int/provenance cast
  branches; those were intentionally left for the later address-int or memory
  families.
- Alloca and local slot declaration state still lives on `BirFunctionLowerer`;
  the new local-memory handler only groups the existing mutation paths.

## Proof

`cmake --build --preset default --target c4c_backend && ctest --test-dir build -j --output-on-failure -R '^backend_'`
passed: 97 backend tests passed, 0 failed, 12 disabled not run. Proof log:
`test_after.log`. `git diff --check` passed.
