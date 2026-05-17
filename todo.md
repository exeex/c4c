Status: Active
Source Idea Path: ideas/open/275_prepared_printer_decomposition.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Extract Core, Function, Frame, Call, and Variadic Printers

# Current Packet

## Just Finished

Completed `plan.md` Step 2 detailed call-plan printer extraction:
`append_call_plans` and the helper functions it needs for detailed call-plan
output now live in `src/backend/prealloc/prepared_printer/calls.cpp`.
`prepared_printer.cpp` still calls the private printer declaration in the same
prepared dump order, and the direct-source `backend_prepare_phi_materialize_test`
target now lists `prepared_printer/calls.cpp` alongside the existing split
printer files.

## Suggested Next

Extract the next behavior-preserving prepared-printer family from
`prepared_printer.cpp` into its own private implementation file, keeping the
helper surface narrow and wiring any direct-source test target that names
`prepared_printer.cpp` to the new implementation file.

## Watchouts

- Preserve prepared dump output exactly.
- Keep `prepared_printer.hpp` as the small public print API.
- Do not change prepared schema, producer behavior, tests, or snapshots.
- Production `.cpp` files must not include implementation `.cpp` files.
- One existing backend test links `prepared_printer.cpp` directly instead of
  the backend library; its CMake source list now also includes
  `prepared_printer/calls.cpp`, `prepared_printer/functions.cpp`,
  `prepared_printer/frame.cpp`, and `prepared_printer/variadic.cpp`.
- Keep promoting only the declarations a moved printer actually needs; avoid
  turning `private.hpp` into a broad helper dump.

## Proof

Ran the delegated proof:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepared_printer|backend_prepare_phi_materialize)$'; } > test_after.log 2>&1`

Result: passed. `git diff --check` passed. Proof log: `test_after.log`.
