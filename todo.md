Status: Active
Source Idea Path: ideas/open/162_link_name_id_backend_symbol_authority.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Propagate LinkNameId Through LIR to BIR

# Current Packet

## Just Finished

Completed Step 4 LIR-to-BIR initializer-symbol authority fix in
`src/backend/bir/lir_to_bir/module.cpp`.

`resolve_initializer_symbol_link_name_id` now treats a present initializer
function `LinkNameId` as authoritative for the function-designator path. If the
ID misses the known function-symbol set, lowering records no BIR initializer
symbol ID instead of falling through to raw global/function symbol lookup. The
focused backend test adds a raw spelling collision and proves the raw function
symbol cannot override a structured initializer ID miss.

## Suggested Next

Continue Step 4 by auditing the remaining LIR-to-BIR function/global symbol
consumers for any valid `LinkNameId` miss that still reopens raw spelling
lookup.

## Watchouts

- Compatibility raw symbol lookup remains available only when no initializer
  function `LinkNameId` metadata is present.
- This packet intentionally leaves `resolve_known_global_address` raw function
  compatibility behavior unchanged; the owned fix is the BIR
  `initializer_symbol_name_id` resolution route.

## Proof

Passed:

```sh
{ cmake --build --preset default --target backend_lir_to_bir_notes_test && ctest --test-dir build -j --output-on-failure -R '^backend_lir_to_bir_notes$'; } > test_after.log 2>&1
```

Proof log: `test_after.log`. Result: build succeeded and 1/1 selected test
passed.
