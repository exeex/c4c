Status: Active
Source Idea Path: ideas/open/10_bir-global-initializer-family-extraction.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Move Initializer Parsing Helpers

# Current Packet

## Just Finished

Completed Step 3 by mechanically moving the global initializer parser helpers
from `src/backend/bir/lir_to_bir/globals.cpp` into
`src/backend/bir/lir_to_bir/global_initializers.cpp`.
`parse_global_address_initializer` is now implemented in
`global_initializers.cpp`, with `parse_global_symbol_initializer` and
`parse_global_gep_initializer` remaining private to that translation unit.
`globals.cpp` continues to compile against the moved parser entry through
`lowering.hpp`.

## Suggested Next

Execute Step 4 from `plan.md`: mechanically move the initializer lowering
helpers from `globals.cpp` into `global_initializers.cpp`, preserving behavior
and keeping global entry behavior in `globals.cpp`.

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
- `globals.cpp` still owns `<array>`, `<charconv>`, and `<cstring>` because
  the Step 4 lowerer helpers remain there.
- `parse_integer_array_type` remains in `globals.cpp` for now and is declared
  through `lowering.hpp`; move it only if the Step 4 lowerer extraction needs
  to take ownership of the integer-array parsing/lowering family together.

## Proof

Proof command run successfully:

- `cmake --build --preset default --target c4c_codegen && cmake --build --preset default --target backend_lir_to_bir_notes_test`

Proof log path: `test_after.log`.
