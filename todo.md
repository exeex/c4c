Status: Active
Source Idea Path: ideas/open/10_bir-global-initializer-family-extraction.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Move Initializer Parsing Helpers

# Current Packet

## Just Finished

Completed Step 2 by adding
`src/backend/bir/lir_to_bir/global_initializers.cpp`, registering it in the
explicit `backend_lir_to_bir_notes_test` lir-to-bir source list, and adding the
private `lowering.hpp` declarations needed by the planned initializer moves.
`src/codegen/CMakeLists.txt` already uses the recursive codegen source glob, so
the new implementation file is picked up there without explicit listing.

Compile-only declaration adjustment made in `globals.cpp`:
`IntegerArrayType` now lives in `lowering.hpp`, and the functions declared for
the split have external linkage instead of anonymous-namespace linkage while
their helper functions remain private.

## Suggested Next

Execute Step 3 from `plan.md`: mechanically move the initializer parser and
lowerer implementation family from `globals.cpp` into
`global_initializers.cpp`, preserving behavior and keeping shared declarations
limited to `lowering.hpp`.

## Watchouts

- Preserve behavior; this extraction is mechanical.
- Do not add new headers.
- Keep `lower_minimal_global`, `lower_string_constant_global`, and
  `resolve_known_global_address` in `globals.cpp` unless the implementation
  proves a tiny wrapper is unavoidable.
- Do not rewrite test expectations.
- `lower_scalar_global` and `lower_minimal_global` already call the declared
  initializer functions through `lowering.hpp`; avoid adding duplicate wrappers
  in `globals.cpp`.
- `globals.cpp` currently owns `<array>`, `<charconv>`, and `<cstring>` because
  initializer helpers need them; after the move, `global_initializers.cpp`
  should include what it uses and `globals.cpp` should keep only includes still
  required by the stay set.

## Proof

Proof commands run successfully:

- `cmake --build --preset default --target c4c_codegen`
- `cmake --build --preset default --target backend_lir_to_bir_notes_test`

Proof log path: `test_after.log`.
