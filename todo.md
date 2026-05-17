Status: Active
Source Idea Path: ideas/open/275_prepared_printer_decomposition.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Extract Regalloc, Value Location, and Control Flow Printers

# Current Packet

## Just Finished

Completed `plan.md` Step 4 value-location printer extraction:
`append_value_locations` and its directly owned helper functions now live in
`src/backend/prealloc/prepared_printer/value_locations.cpp`.
`prepared_printer.cpp` still calls the private printer declaration in the same
prepared dump order, and the direct-source `backend_prepare_phi_materialize_test`
target now lists `prepared_printer/value_locations.cpp`.

## Suggested Next

Continue `plan.md` Step 4 by extracting `append_regalloc` from
`prepared_printer.cpp`, including only the regalloc-owned lookup and placement
helpers needed by that printer. Keep the helper surface narrow and wire the
direct-source test target that names `prepared_printer.cpp` to the new
implementation file.

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
  `prepared_printer/runtime_helpers.cpp`,
  `prepared_printer/special_carriers.cpp`, `prepared_printer/storage.cpp`, and
  `prepared_printer/value_locations.cpp`, plus `prepared_printer/variadic.cpp`.
- Keep promoting only the declarations a moved printer actually needs; avoid
  turning `private.hpp` into a broad helper dump.
- `prepared_printer.cpp` still owns `append_regalloc` and its helper cluster.

## Proof

Ran the delegated proof:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepared_printer|backend_prepare_phi_materialize)$'; } > test_after.log 2>&1`

Result: passed. `git diff --check` passed. Proof log: `test_after.log`.
