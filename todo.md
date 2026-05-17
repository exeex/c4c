Status: Active
Source Idea Path: ideas/open/275_prepared_printer_decomposition.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Extract Core, Function, Frame, Call, and Variadic Printers

# Current Packet

## Just Finished

Completed `plan.md` Step 2 variadic-entry printer extraction:
`append_variadic_entry_plans` and its directly owned private helpers now live
in `src/backend/prealloc/prepared_printer/variadic.cpp`. `prepared_printer.cpp`
still calls the same private printer declaration in the same prepared dump
order, and the direct-source `backend_prepare_phi_materialize_test` target now
lists `prepared_printer/variadic.cpp` alongside the existing split printer
files.

## Suggested Next

Extract the next behavior-preserving Step 2 unit from the same neighborhood:
move the detailed call-plan printer once its helper surface can be kept private
and narrow. For any direct-source test target that names `prepared_printer.cpp`,
add the corresponding private implementation `.cpp` to that test target rather
than including implementation files from production code.

## Watchouts

- Preserve prepared dump output exactly.
- Keep `prepared_printer.hpp` as the small public print API.
- Do not change prepared schema, producer behavior, tests, or snapshots.
- Production `.cpp` files must not include implementation `.cpp` files.
- One existing backend test links `prepared_printer.cpp` directly instead of
  the backend library; its CMake source list now also includes
  `prepared_printer/functions.cpp`, `prepared_printer/frame.cpp`, and
  `prepared_printer/variadic.cpp`.
- Keep promoting only the declarations a moved printer actually needs; avoid
  turning `private.hpp` into a broad helper dump.

## Proof

Ran the delegated proof:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepared_printer|backend_prepare_phi_materialize)$'; } > test_after.log 2>&1`

Result: passed. `git diff --check` passed. Proof log: `test_after.log`.
