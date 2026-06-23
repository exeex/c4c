Status: Active
Source Idea Path: ideas/open/320_rv64_nested_select_chain_store_source_publication.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Prepared Destination-Access Publication

# Current Packet

## Just Finished

Completed Step 3 prepared destination-access publication repair for nested
select-chain store-source records.

Implementation result:

- Store-source publication now falls back from exact BIR instruction-index
  lookup to the unique same-block prepared memory access for the stored value
  when select-chain materialization shifted the prepared access before the
  logical `store_local` instruction.
- Source-producer ordering still uses the logical publication instruction index,
  so earlier select/cast materialization can be attached to the store-source
  record without allowing a producer after the store.
- `backend_dump_riscv64_nested_select_store_source_publication` was flipped from
  expected-repair to a positive dump contract: nested store-source records for
  the focused path must be `status=available`, and
  `status=missing_destination_access` is forbidden.
- The simple pointer-typed select publication neighbor remains green.

Candidate reprobe:

- `src/00144.c` was reprobed under
  `build/rv64_c_testsuite_probe_latest/triage_320_step3/`.
- BIR dump, prepared-BIR dump, RV64 emit, clang link, and qemu all returned 0.
- The prepared dump no longer contains `missing_destination_access`; former
  nested residual store-source records such as `%t36`, `%t56`, and `%t65` now
  report `status=available`.

## Suggested Next

Reprobe/close idea 320 classification or ask the plan owner/supervisor whether
the source idea is closure-ready, since the focused fixture and `src/00144.c`
now both pass this destination-access publication boundary.

## Watchouts

- The fallback is intentionally constrained to a unique same-block prepared
  memory access for the stored value; ambiguous repeated same-value stores
  still decline the fallback instead of guessing a destination.
- Do not special-case `src/00144.c`, `tern.end.*`, fixed source ternary shapes,
  or observed RV64 instruction text.
- Do not fold idea 321's i16 local-array/select-store residual, stack-homed
  fused compare control flow, aggregate/byval, function-pointer, external-call,
  or broad switch/control-flow work into this route.

## Proof

Focused neighbor proof passed after the repair:
`cmake --build --preset default -j && ctest --test-dir build -j --output-on-failure -R 'backend_dump_riscv64_(nested_select_store_source_publication|pointer_typed_select_publication)'`.

Delegated proof ran:
`cmake --build --preset default -j && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`.

Result: build passed; backend CTest returned nonzero because the existing
unrelated `backend_riscv_prepared_edge_publication` test still fails. The
positive `backend_dump_riscv64_nested_select_store_source_publication` test,
the pointer-typed select dump/codegen/runtime neighbor tests, and the rest of
the backend subset passed in `test_after.log`.
