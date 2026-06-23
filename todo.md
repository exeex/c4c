Status: Active
Source Idea Path: ideas/open/321_rv64_i16_local_array_select_store_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Focused I16 Local-Array Select/Store Coverage

# Current Packet

## Just Finished

Completed Step 2 focused expected-repair coverage for RV64 i16 local-array
select/store emission.

New focused fixture:

- `tests/backend/case/riscv64_i16_local_array_select_store.c`

New focused tests:

- `backend_dump_riscv64_i16_local_array_select_store`
- `backend_codegen_route_riscv64_i16_local_array_select_store`

Coverage result:

- The fixture uses a compact local `short[4]` dynamic element assignment, so it
  exercises halfword local-array select/store lowering without copying
  `src/00143.c`, matching `%t9.store0`, relying on fixed block labels, or using
  an int-array reclassification.
- The dump test proves the intended prepared boundary: i16 store-source records
  such as `%t16.store2` are `status=available` with
  `source_producer=select_materialization`, select-chain metadata exists, and
  the prepared memory access is a size-2 store.
- The codegen route test records the current RV64 gap as expected repair:
  emission starts `main` and initializes the integer index, then currently lacks
  semantic halfword load/store operations and a return path. It forbids `sh`,
  `lh`, `lhu`, and `ret` so the test will fail when the repair starts emitting
  the missing halfword path.
- This distinguishes idea 321 from idea 319's stack-homed fused-compare branch
  repair and idea 320's missing destination-access publication repair.

## Suggested Next

Repair RV64 prepared emission for i16 local-array select/store values with
stack-slot homes and size-2 prepared memory accesses, then flip the focused
codegen expected-repair test to a positive semantic halfword emission contract
and add runtime coverage if the repaired fixture links.

## Watchouts

- Do not special-case `src/00143.c`, `.Lmain_block_1`, `.Lmain_block_2`,
  `%t9.store0`, fixed source spelling, or observed stack offsets.
- Do not claim progress by widening the focused fixture from i16/halfword
  storage to int storage unless that is explicitly recorded as
  reclassification rather than repair.
- The focused route should repair halfword load/select/store emission for
  stack-slot `bank=none` i16 values, not destination-access publication; the
  prepared store-source records are already available.
- Do not fold stack-homed fused compare control flow, nested select-chain
  store-source publication, aggregate/byval, function-pointer, external-call,
  or broad switch/control-flow work into this route.

## Proof

Focused proof passed:
`cmake --build --preset default -j && ctest --test-dir build -j --output-on-failure -R 'backend_(dump|codegen_route)_riscv64_i16_local_array_select_store'`.

Delegated proof ran:
`cmake --build --preset default -j && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`.

Result: build passed; backend CTest returned nonzero because the existing
unrelated `backend_riscv_prepared_edge_publication` test still fails. The new
focused dump and expected-repair codegen tests passed in `test_after.log`.
