Status: Active
Source Idea Path: ideas/open/10_bir-global-initializer-family-extraction.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Initializer Ownership and Move Boundary

# Current Packet

## Just Finished

Completed Step 1 mapping for the initializer extraction boundary in
`src/backend/bir/lir_to_bir/globals.cpp`; no implementation files were changed.

Move set for `global_initializers.cpp`:

- Initializer parsers/lowerers: `parse_global_symbol_initializer`,
  `parse_global_gep_initializer`, `parse_global_address_initializer`,
  `lower_llvm_byte_string_initializer`, `lower_global_initializer`,
  `lower_integer_array_initializer`, and `lower_aggregate_initializer`.
- Initializer-only helpers to move with them: `parse_hex_digit`,
  `append_x86_fp80_initializer_bytes`, `peel_integer_array_layer`,
  `lower_zero_initializer_value`, `append_zero_integer_array_initializer`,
  `lower_integer_array_initializer_recursive`,
  `strip_typed_initializer_prefix`, `append_zero_aggregate_initializer`, and
  `lower_aggregate_initializer_recursive`.

Stay set for `globals.cpp`:

- Global entry/address behavior: `lower_minimal_global`,
  `lower_string_constant_global`, `resolve_known_global_address`,
  `resolve_pointer_initializer_offsets`, and `is_known_function_symbol`.
- Global classification helpers: `lower_scalar_global_type`,
  `lower_scalar_global`, `IntegerArrayType`, and `parse_integer_array_type`.

`lowering.hpp` declaration needs for the split:

- Add `IntegerArrayType` and `parse_integer_array_type` because
  `parse_integer_array_type` should stay with global classification but the
  moved byte/integer-array initializer lowering still needs it.
- Add declarations for the moved functions still called from `globals.cpp`:
  `strip_typed_initializer_prefix`, `parse_global_address_initializer`,
  `lower_global_initializer`, `lower_integer_array_initializer`, and
  `lower_aggregate_initializer`.
- No declarations are needed for `parse_global_symbol_initializer`,
  `parse_global_gep_initializer`, recursive/zero/hex helpers, or
  `lower_llvm_byte_string_initializer` if they remain internal to
  `global_initializers.cpp`.

Parser ownership decision for Step 2/3:

- Keep `parse_integer_array_type` in `globals.cpp`; it is not exclusively an
  initializer helper because `lower_minimal_global` uses it to classify integer
  array globals before lowering initializer elements.
- Move `peel_integer_array_layer`; it is only used by integer-array
  initializer lowering.

## Suggested Next

Execute Step 2 from `plan.md`: add
`src/backend/bir/lir_to_bir/global_initializers.cpp`, register it in the
existing build source list, and add only the `lowering.hpp` declarations needed
for the Step 3/4 moves.

## Watchouts

- Preserve behavior; this extraction is mechanical.
- Do not add new headers.
- Keep `lower_minimal_global`, `lower_string_constant_global`, and
  `resolve_known_global_address` in `globals.cpp` unless the implementation
  proves a tiny wrapper is unavoidable.
- Do not rewrite test expectations.
- `lower_scalar_global` and `lower_minimal_global` will depend on moved
  initializer helpers through `lowering.hpp`; avoid leaving duplicate wrappers
  in `globals.cpp`.
- `globals.cpp` currently owns `<array>`, `<charconv>`, and `<cstring>` because
  initializer helpers need them; after the move, `global_initializers.cpp`
  should include what it uses and `globals.cpp` should keep only includes still
  required by the stay set.

## Proof

Mapping-only packet; no build required and no `test_after.log` was produced.
Confirmed no implementation files changed.
