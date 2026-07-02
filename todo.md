Status: Active
Source Idea Path: ideas/open/557_bir_local_memory_semantic_producer_admission.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Reinspect Remaining Local-Memory Admission Boundary

# Current Packet

## Just Finished

Step 5 inspection completed. The first remaining producer/admission boundary
after the local-slot `MemoryAddress` repair is the direct local-slot
load/store path for pointer-value local memory accesses: the byte-array
widened path now publishes provenance-rich `MemoryAddress` records, but exact
same-slot `LoadLocalInst`/`StoreLocalInst` branches in
`memory/local_slots.cpp` still emit bare local instructions and the pointer
slot store/load helpers also finish through bare local slot carriers after
recording side-table address state.

Selected shared boundary: publish/admit complete producer-owned address facts
for direct local-slot pointer-value stores and loads, including local-slot
identity, requested range, known extent, layout authority, and in-bounds
verdict when the local slot extent is known.

Representative row to prove first: `src/20001026-1.c`
(`build_real_from_int_cst_1`, store local-memory semantic family), because its
`args->d = real_value_from_int_cst(...)` assignment is the store-family seed
and exercises a derived local aggregate write through a pointer before the
backend-object handoff.

## Suggested Next

Execute Step 6 in `plan.md` for this boundary. Add focused
`backend_lir_to_bir_notes_test.cpp` coverage that constructs a local aggregate
or local pointer-derived store fixture matching the `20001026-1` shape and
asserts the resulting direct `StoreLocalInst` carries a `MemoryAddress` with
local-slot provenance, requested range, complete known extent, scalar/local
layout authority, and an in-bounds verdict. Include the neighboring direct
same-slot `LoadLocalInst` case in the same family so the repair cannot be
testcase-shaped around the store seed alone.

## Watchouts

- The five current representative `case.log` files still show only family
  admission notes: `20000314-1` load, `20000717-4` GEP, `20001026-1` store,
  `20000519-1` scalar/local-memory, and `20050604-1` alloca.
- This can stay in the current source idea. The selected gap is producer-side
  local-memory fact publication/admission, not a distinct lifecycle initiative.
- Do not change expectations, unsupported markers, allowlists, runtime
  comparison behavior, or semantic admission strength.
- If Step 6 proves the `20001026-1` store failure is instead an aggregate-copy
  lowering miss with no direct same-slot address carrier, keep the packet in
  this source idea but retarget the focused test to derived local aggregate
  store publication rather than splitting lifecycle state.

## Proof

Inspection packet only. No build or test command was required or run. Evidence
inspected: the five current `case.log` files under
`build/rv64_gcc_c_torture_backend/`, the five representative C sources,
`docs/rv64_gcc_torture_post_contract/bir_local_memory_producer_boundary.md`,
focused local-memory coverage in `tests/backend/bir/backend_lir_to_bir_notes_test.cpp`,
and local-memory producer paths in `src/backend/bir/lir_to_bir/memory/`.
