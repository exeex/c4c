Status: Active
Source Idea Path: ideas/open/24_riscv_prepared_edge_publication_register_destination_consumer.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Focused RISC-V Coverage and Negative Proof

# Current Packet

## Just Finished

Completed Step 2 and Step 3 for the first RISC-V register-destination
edge-publication consumer.

Implemented a RISC-V target-local helper in
`src/backend/mir/riscv/codegen/emit.cpp` and declared it in
`src/backend/mir/riscv/codegen/emit.hpp`. The helper consumes shared prepared
`edge_publications` through
`prepare::find_unique_indexed_prepared_edge_publication(...)`, accepts only
`Register -> Register` moves to register destinations, and renders
`mv <destination_register>, <source_register>` without rescanning BIR edges or
building a RISC-V-local edge fact table.

Added `emit_prepared_module(...)` for RISC-V prepared modules and wired an
explicit `emit_riscv_bir_module_entry(...)` through `backend.hpp`,
`backend.cpp`, and `src/backend/CMakeLists.txt`.

Added focused coverage in
`tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp` with CMake
registration as `backend_riscv_prepared_edge_publication`. The positive case
proves shared-lookup consumption emits `mv a1, a0`; the lookup-index removal
case proves RISC-V does not rediscover the edge move locally. Negative cases
cover missing shared lookups, missing publication authority, unsupported stack
source, unsupported stack destination, and non-move publication operations.

## Suggested Next

Proceed to Step 4: decide whether the remaining ready register-destination
source homes, especially `StackSlot -> Register` and
`RematerializableImmediate -> Register`, should be folded into this route or
left as separate RISC-V coverage packets.

## Watchouts

- `PointerBasePlusOffset -> Register` and every source-to-`StackSlot`
  destination still fail closed and remain out of scope for this route unless
  lifecycle state changes.
- `StackSlot -> Register` and `RematerializableImmediate -> Register` are not
  implemented by this first code packet; Step 4 should make the fold-in versus
  follow-up decision explicitly.
- `src/backend/mir/riscv/codegen/emit.cpp` no longer includes the stale
  RISC-V mirror header because adding this file to the backend build exposed a
  bad relative include there; this packet kept the compile fix local to
  `emit.cpp`.

## Proof

Ran the delegated proof command:

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(backend_riscv_prepared_edge_publication|backend_codegen_route_riscv64_.*|backend_prepared_lookup_helper|backend_prealloc_block_entry_publications)$' >> test_after.log 2>&1`

Result: PASS. Build succeeded and CTest reported 5/5 selected tests passing.
Supervisor ran the matching regression guard against the focused
`test_before.log` and the focused `test_after.log`; it passed with pass count
increasing from 4 to 5 and no new failures.

Supervisor then ran broader backend validation:

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`

Result: PASS. Build succeeded and CTest reported 163/163 backend tests passing.
This backend run is broader validation only, not the matching before/after
regression comparison for the focused packet.
