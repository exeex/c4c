Status: Active
Source Idea Path: ideas/open/162_link_name_id_backend_symbol_authority.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Propagate LinkNameId Through LIR to BIR

# Current Packet

## Just Finished

Completed Step 4 LIR-to-BIR known-global-address authority fix in
`src/backend/bir/lir_to_bir/globals.cpp`.

`resolve_known_global_address` now treats a present initializer function
`LinkNameId` as authoritative when deciding whether a pointer initializer names
a known function. If the ID is valid but misses the known function-symbol set,
the route no longer falls back to raw function-symbol spelling. The focused
backend test keeps raw compatibility green for no-ID metadata and proves a
valid ID miss with the same raw function spelling does not lower.

## Suggested Next

Continue Step 4 by auditing aggregate pointer-initializer offsets and runtime
global-address provenance for any remaining raw function-symbol lookup that
lacks structured `LinkNameId` authority.

## Watchouts

- Compatibility raw function lookup remains available when no initializer
  function `LinkNameId` metadata is present.
- This packet intentionally targets scalar pointer-global known-address
  resolution; aggregate pointer offsets still use raw `GlobalAddress` spellings
  and may need a separate metadata route before they can enforce the same
  authority rule.

## Proof

Passed:

```sh
{ cmake --build --preset default --target backend_lir_to_bir_notes_test && ctest --test-dir build -j --output-on-failure -R '^backend_lir_to_bir_notes$'; } > test_after.log 2>&1
```

Proof log: `test_after.log`. Result: build succeeded and 1/1 selected test
passed.
