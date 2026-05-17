Status: Active
Source Idea Path: ideas/open/275_prepared_printer_decomposition.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Extract Regalloc, Value Location, and Control Flow Printers

# Current Packet

## Just Finished

Completed `plan.md` Step 4 regalloc printer extraction:
`append_regalloc` and its directly owned value lookup, register placement,
spill-slot placement, and occupancy helpers now live in
`src/backend/prealloc/prepared_printer/regalloc.cpp`. `prepared_printer.cpp`
still calls the private printer declaration in the same prepared dump order,
and the direct-source `backend_prepare_phi_materialize_test` target now lists
`prepared_printer/regalloc.cpp`.

## Suggested Next

Run Step 5 completion validation and shrink work: inspect the now-small
`prepared_printer.cpp` for leftover helpers/includes that belong to already
extracted printer families, remove only dead local scaffolding that does not
change dump output, and prove the full prepared-printer split remains linked by
the backend library plus the direct-source phi-materialize test.

## Watchouts

- Preserve prepared dump output exactly.
- Keep `prepared_printer.hpp` as the small public print API.
- Do not change prepared schema, producer behavior, tests, or snapshots.
- Production `.cpp` files must not include implementation `.cpp` files.
- One existing backend test links `prepared_printer.cpp` directly instead of
  the backend library; its CMake source list now also includes
  `prepared_printer/addressing.cpp`, `prepared_printer/atomics.cpp`,
  `prepared_printer/calls.cpp`, `prepared_printer/control_flow.cpp`,
  `prepared_printer/frame.cpp`, `prepared_printer/functions.cpp`,
  `prepared_printer/inline_asm.cpp`, `prepared_printer/intrinsics.cpp`,
  `prepared_printer/regalloc.cpp`,
  `prepared_printer/runtime_helpers.cpp`,
  `prepared_printer/special_carriers.cpp`, `prepared_printer/storage.cpp`, and
  `prepared_printer/value_locations.cpp`, plus `prepared_printer/variadic.cpp`.
- Keep promoting only the declarations a moved printer actually needs; avoid
  turning `private.hpp` into a broad helper dump.
- `prepared_printer.cpp` now calls all extracted Step 4 private printer
  declarations in dump order; remaining cleanup should be shrink-only.

## Proof

Ran the delegated proof:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepared_printer|backend_prepare_phi_materialize)$'; } > test_after.log 2>&1`

Result: passed. `git diff --check` passed. Proof log: `test_after.log`.
