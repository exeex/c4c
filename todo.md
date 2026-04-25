Status: Active
Source Idea Path: ideas/open/05_bir-memory-intrinsic-family-extraction.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Map the Intrinsic Family Boundary

# Current Packet

## Just Finished

Step 1: Map the Intrinsic Family Boundary completed as discovery-only mapping.

Move from `src/backend/bir/lir_to_bir/memory/local_slots.cpp` to new
`src/backend/bir/lir_to_bir/memory/intrinsics.cpp`:

- Anonymous-namespace intrinsic-only structs:
  `LocalArrayView`, `LocalMemcpyLeaf`, `LocalMemcpyLeafView`,
  `LocalMemcpyScalarSlot`, and `LocalMemsetScalarSlot`.
- Member definitions:
  `BirFunctionLowerer::try_lower_immediate_local_memset`,
  `BirFunctionLowerer::try_lower_immediate_local_memcpy`,
  `BirFunctionLowerer::lower_memory_memcpy_inst`,
  `BirFunctionLowerer::lower_memory_memset_inst`, and
  `BirFunctionLowerer::try_lower_direct_memory_intrinsic_call`.

Keep in `local_slots.cpp`:

- `ScalarLayoutLeafFacts` helper implementation
  `resolve_scalar_layout_facts_at_byte_offset`.
- Anonymous-namespace raw-byte local aggregate helpers:
  `LocalAggregateRawByteSliceLeaf` and
  `resolve_local_aggregate_raw_byte_slice_leaf`; they are used by local-slot
  GEP/addressing paths after the intrinsic block.
- Local-slot allocation, GEP, load, store, provenance, and scalar-layout code
  outside the five target member definitions.

Build registration:

- `src/backend/CMakeLists.txt` uses `file(GLOB_RECURSE ... bir/*.cpp)` with
  `CONFIGURE_DEPENDS`, so `memory/intrinsics.cpp` is picked up by the backend
  library source glob.
- `tests/backend/CMakeLists.txt` manually lists LIR-to-BIR sources for
  `backend_lir_to_bir_notes_test`; add
  `${PROJECT_SOURCE_DIR}/src/backend/bir/lir_to_bir/memory/intrinsics.cpp`
  beside `memory/local_slots.cpp` there.

## Suggested Next

Step 2 packet: create `src/backend/bir/lir_to_bir/memory/intrinsics.cpp`, move
only the mapped intrinsic family into it, keep declarations in
`lowering.hpp`, add the manual `backend_lir_to_bir_notes_test` source-list
entry, and leave all local-slot-only helpers in `local_slots.cpp`.

## Watchouts

- Do not add new headers.
- Do not edit testcase expectations.
- Do not convert the moved functions away from `BirFunctionLowerer` members.
- `intrinsics.cpp` should include `../lowering.hpp`; include
  `memory_helpers.hpp` only if the moved definitions still need a declaration
  from it after extraction.
- The moved intrinsic definitions use `lir_to_bir_detail::parse_i64`,
  `lir_to_bir_detail::lower_integer_type`, and
  `lir_to_bir_detail::type_size_bytes`, plus standard library facilities such
  as `std::sort`, `std::min`, `std::optional`, `std::string`,
  `std::string_view`, and `std::vector`.
- `LocalArrayView` is intrinsic-only today because both uses are inside
  `try_lower_immediate_local_memset` and
  `try_lower_immediate_local_memcpy`.
- Keep `LocalAggregateRawByteSliceLeaf` and
  `resolve_local_aggregate_raw_byte_slice_leaf` in `local_slots.cpp`; they are
  referenced by later local aggregate raw-byte GEP/addressing code.
- Narrow proof command for Step 2/3:
  `cmake --build build --target c4c_backend backend_lir_to_bir_notes_test c4cll -j && ctest --test-dir build -R '^(backend_lir_to_bir_notes|backend_codegen_route_x86_64_builtin_mem(cpy|set).+_observe_semantic_bir)$' --output-on-failure`

## Proof

Not run - discovery-only Step 1, no implementation files changed.
