Status: Active
Source Idea Path: ideas/open/688_initializer_lowering_bridge_isolation.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Narrow Global Declaration Import State

# Current Packet

## Just Finished

Step 2 implementation completed for the first initializer bridge isolation
packet.

Exact files/helpers touched:
- `src/backend/bir/lir_to_bir/lowering.hpp`: renamed the adapter detail bridge
  type from `FunctionSymbolSet` to `ImportedFunctionSymbolIndex`, preserving
  the same methods, storage fields, and declarations for pointer-initializer
  and known-global-address helpers.
- `src/backend/bir/lir_to_bir/globals.cpp`: updated the renamed type's method
  definitions and the signatures for `is_known_raw_function_symbol()`,
  `is_known_function_global_address()`, `resolve_known_global_address()`, and
  `resolve_pointer_initializer_offsets()` without changing lookup order or
  fallback rules.
- `src/backend/bir/lir_to_bir/module.cpp`: updated module-boundary construction
  and population of the imported-function bridge, plus the initializer symbol
  resolver and `BirFunctionLowerer` constructor boundary.

Behavioral scope: no prepared object-data behavior, target relocation spelling,
diagnostics, runtime behavior, expectations, unsupported markers, tests, or
harness policy were changed.

## Suggested Next

Next coherent packet: isolate the string-constant target-id rewrite path after
the declaration/import bridge rename, keeping relocation-slot publication and
prepared object-data behavior unchanged.

## Watchouts

- The split lowering translation units outside this packet still rely on
  `BirFunctionLowerer::FunctionSymbolSet`; `lowering.hpp` keeps that class-local
  compatibility alias to the new `ImportedFunctionSymbolIndex` so this packet
  does not touch unowned implementation files.
- `GlobalTypes` remains shared by memory/addressing/provenance code and should
  still be isolated in a separate packet if that becomes the selected route.
- Keep the next packet from changing relocation spelling or prepared object-data
  publication while moving string constant rewrite state.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed. Build completed and 302 backend tests passed with 0 failures.
Fresh proof output is preserved in `test_after.log`.
