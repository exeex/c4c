Status: Active
Source Idea Path: ideas/open/113_backend_struct_decl_layout_table_dual_path.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Convert Initializer And Aggregate Load/Store Consumers

# Current Packet

## Just Finished

Step 5 - Convert Initializer And Aggregate Load/Store Consumers converted
backend-owned global aggregate size/layout, aggregate initializer recursion, and
global GEP initializer parsing to prefer the backend structured layout table
when the module has structured facts available, while preserving explicit
fallback behavior.

Changed files:

- `src/backend/bir/lir_to_bir/global_initializers.cpp`
- `src/backend/bir/lir_to_bir/globals.cpp`
- `src/backend/bir/lir_to_bir/lowering.hpp`
- `src/backend/bir/lir_to_bir/module.cpp`
- `todo.md`
- `test_after.log`

Converted consumers:

- `lower_module()` now passes the module `BackendStructuredLayoutTable` into
  `lower_minimal_global()`.
- `lower_minimal_global()` now uses structured-aware aggregate layout lookup for
  aggregate-backed global size, alignment, storage size, and initializer layout.
- `parse_global_address_initializer()` has a structured-aware overload used by
  scalar pointer globals, pointer aggregate fields, and GEP initializer parsing.
- `lower_aggregate_initializer()` has a structured-aware overload threaded
  through aggregate recursion, zero aggregate fill, nested array element sizing,
  struct field offsets, and pointer field address parsing.

Remaining fallback-only paths: legacy public overloads for
`parse_global_address_initializer()`, `lower_aggregate_initializer()`, and
`lower_minimal_global()` still route through the null-table layout path for
callers without module structured layout state. Integer-array global lowering
continues to use its existing integer-array parser because it does not consult
aggregate struct layout.

ABI classification files, memory/addressing.cpp, unrelated MIR files, test
expectations, `plan.md`, and the source idea were not changed.

## Suggested Next

Next coherent packet: send Step 5 to review, or delegate the next plan step the
supervisor selects after confirming this completes the structured-aware globals
and initializer consumer conversion.

## Watchouts

The structured lookup helper still intentionally accepts structured layouts only
when the table has checked matching legacy parity for a named `%` type; otherwise
these consumers fall back to legacy computed layout.

## Proof

Delegated proof passed and wrote `test_after.log`:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_|positive_split_llvm_|abi_)' && cmake --build build-backend --target c4c_backend -j) > test_after.log 2>&1`

Result: default build completed, selected ctest subset passed 8/8, and the
backend `c4c_backend` target rebuilt successfully.
