Status: Active
Source Idea Path: ideas/open/688_initializer_lowering_bridge_isolation.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Narrow Global Declaration Import State

# Current Packet

## Just Finished

Step 2 follow-up completed for the string authority guard regression introduced
by the imported function symbol bridge rename.

Exact files/helpers touched:
- `src/backend/bir/lir_to_bir/lowering.hpp`: added the neutral
  `ImportedFunctionSymbolIndex::no_id_compatibility_link_name_id()` accessor
  and kept the existing `find_raw_symbol_link_name_id()` wrapper for split
  lowerer files outside this packet.
- `src/backend/bir/lir_to_bir/globals.cpp`: moved the out-of-line lookup
  definition and Step 2 globals call sites to the neutral accessor, eliminating
  the declaration-level `find_raw_symbol_link_name_id` guard hit without
  changing storage, lookup order, or fallback behavior.
- `src/backend/bir/lir_to_bir/module.cpp`: updated the initializer symbol
  resolver to use the neutral no-id compatibility accessor.

Behavioral scope: no prepared object-data behavior, target relocation spelling,
diagnostics, runtime behavior, expectations, unsupported markers, tests, guard
classifications, allowlists, or harness policy were changed.

## Suggested Next

Next coherent packet: isolate the string-constant target-id rewrite path after
the declaration/import bridge rename, keeping relocation-slot publication and
prepared object-data behavior unchanged.

## Watchouts

- The split lowering translation units outside this packet still rely on
  `ImportedFunctionSymbolIndex::find_raw_symbol_link_name_id()`; `lowering.hpp`
  keeps that inline wrapper so this packet does not touch unowned implementation
  files.
- `GlobalTypes` remains shared by memory/addressing/provenance code and should
  still be isolated in a separate packet if that becomes the selected route.
- Keep the next packet from changing relocation spelling or prepared object-data
  publication while moving string constant rewrite state.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_|string_authority_guard$)'`

Result: passed. Build completed and 303 selected tests passed with 0 failures,
including `string_authority_guard`. Fresh proof output is preserved in
`test_after.log`.
