Status: Active
Source Idea Path: ideas/open/10_bir-global-initializer-family-extraction.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Move Initializer Lowering Helpers

# Current Packet

## Just Finished

Completed Step 4 by mechanically moving the global initializer lowering helper
family from `src/backend/bir/lir_to_bir/globals.cpp` into
`src/backend/bir/lir_to_bir/global_initializers.cpp`.
`lower_global_initializer`, `lower_integer_array_initializer`,
`lower_aggregate_initializer`, `strip_typed_initializer_prefix`, and the
initializer-only byte-string, x86 fp80, zero-fill, recursive array, and
recursive aggregate helpers are now implemented in `global_initializers.cpp`.
`globals.cpp` keeps global entry/address behavior, including
`lower_minimal_global`, `lower_string_constant_global`,
`resolve_known_global_address`, and `parse_integer_array_type`.

## Suggested Next

Execute Step 5 from `plan.md`: prove no behavior change with the relevant
global initializer coverage, escalating only if the supervisor chooses broader
acceptance proof.

## Watchouts

- Preserve behavior; this extraction is mechanical.
- Do not add new headers.
- Do not rewrite test expectations.
- `parse_integer_array_type` remains in `globals.cpp` and is still declared
  through `lowering.hpp` for the moved initializer code.
- `global_initializers.cpp` now owns `<array>`, `<charconv>`, and `<cstring>`
  for the moved byte/floating initializer parsing helpers.

## Proof

Proof command run successfully:

- `cmake --build --preset default --target c4c_codegen && cmake --build --preset default --target backend_lir_to_bir_notes_test`

Proof log path: `test_after.log`.
