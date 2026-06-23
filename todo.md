Status: Active
Source Idea Path: ideas/open/320_rv64_nested_select_chain_store_source_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Focused Nested Select-Chain Store-Source Coverage

# Current Packet

## Just Finished

Completed Step 2 focused expected-repair coverage for nested select-chain
store-source destination-access publication.

New focused fixture:

- `tests/backend/case/riscv64_nested_select_store_source_publication.c`

New focused test:

- `backend_dump_riscv64_nested_select_store_source_publication`

Coverage result:

- The fixture builds a nested pointer/integer select-chain store-source shape
  using local pointer stores and nested conditional expressions without copying
  `src/00144.c`, matching `tern.end.*`, or depending on observed RV64
  instruction text.
- The dump test asserts the boundary between already-repaired simple
  pointer-typed select publication and the remaining nested residual: an early
  store-source record is `status=available`, while a later nested select-chain
  store-source record reports `status=missing_destination_access`.
- The fixture uses pointer/integer values only and does not exercise idea 321's
  i16 local-array/select-store residual.
- The existing simple pointer-typed select publication neighbor remains green:
  `backend_dump_riscv64_pointer_typed_select_publication` passed beside the new
  expected-repair test.

## Suggested Next

Repair prepared destination-access publication for nested select-chain
store-source records, then flip the focused expected-repair dump contract to a
positive publication contract while preserving the simple pointer-typed select
neighbor.

## Watchouts

- Do not special-case `src/00144.c`, `tern.end.*`, fixed source ternary shapes,
  or observed RV64 instruction text.
- Do not treat already-repaired simple pointer-typed select publication as
  completion for this nested store-source route.
- Do not claim implementation progress from this evidence-only packet:
  `src/00144.c` now passes qemu, but the prepared publication residual remains.
- The new focused test intentionally asserts the current prepared residual via
  `missing_destination_access`; a repair packet should flip that expectation
  rather than preserve it.
- Do not fold idea 321's i16 local-array/select-store residual, stack-homed
  fused compare control flow, aggregate/byval, function-pointer, external-call,
  or broad switch/control-flow work into this route.

## Proof

Focused neighbor proof passed:
`cmake --build --preset default -j && ctest --test-dir build -j --output-on-failure -R 'backend_dump_riscv64_(nested_select_store_source_publication|pointer_typed_select_publication)'`.

Delegated proof ran:
`cmake --build --preset default -j && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`.

Result: build passed; backend CTest returned nonzero because the existing
unrelated `backend_riscv_prepared_edge_publication` test still fails. The new
`backend_dump_riscv64_nested_select_store_source_publication` expected-repair
test and the pointer-typed select neighbor tests passed in `test_after.log`.
