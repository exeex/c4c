Status: Active
Source Idea Path: ideas/open/162_link_name_id_backend_symbol_authority.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Propagate LinkNameId Through LIR to BIR

# Current Packet

## Just Finished

Continued Step 4 by auditing direct pointer argument/value alias routes where
LIR still exposes raw `@symbol` operands.

Known-global provenance records now keep the resolved global `LinkNameId` when
direct raw LIR operands are admitted through local pointer slots, global
provenance stores, pointer-object aliases, direct global GEPs, pointer-to-int
address aliases, and linear addressed global scalar accesses. Direct function
pointer argument/value aliases remain compatibility bridges because the LIR
operand and BIR pointer value carriers do not have per-value `LinkNameId`
metadata.

## Suggested Next

Continue Step 4 by auditing remaining non-direct pointer carriers that build
`bir::Value::named(ptr, "@symbol")`, especially aggregate copy/materialization
paths, and either thread existing `GlobalAddress` identity through them or
record the no-metadata bridge.

## Watchouts

- Compatibility bridge: direct `LirOperandKind::Global` function operands used
  as pointer argument values still enter LIR-to-BIR as raw `@symbol` text.
  Owner: LIR operand metadata and BIR pointer-value identity. Limitation:
  `lower_call_pointer_arg_value()` and
  `resolve_local_aggregate_pointer_value_alias()` can validate the symbol
  against function tables, but `bir::Value::named(TypeKind::Ptr, "@symbol")`
  has no field for `LinkNameId`, so the lowered argument value remains spelling
  carried. Removal condition: add structured symbol identity to LIR global
  operands or BIR pointer values, then consume it in direct pointer argument
  lowering.
- Compatibility bridge: direct pointer stores of function operands still use
  raw LIR global spelling as the lookup key before recording `GlobalAddress`.
  Owner: LIR operand metadata. Limitation: LIR-to-BIR now records the recovered
  function `LinkNameId` in `GlobalAddress`, but the initial store operand
  lookup cannot avoid raw text until the operand itself carries identity.
  Removal condition: replace raw function lookup in
  `resolve_pointer_store_address()` and local-slot pointer stores when LIR
  global operands expose `LinkNameId` or equivalent symbol identity.
- Compatibility bridge: aggregate pointer initializer metadata is a global-level
  vector of function `LinkNameId`s, not a per-field mapping. Owner: LIR global
  initializer metadata. Limitation: this packet can safely apply the ID when
  exactly one aggregate pointer initializer offset and one initializer function
  ID are present; multi-function aggregate initializers still require per-field
  metadata before raw spelling can be fully removed. Removal condition:
  introduce offset-aligned initializer function IDs and consume them when
  populating `pointer_initializer_offsets`.

## Proof

Passed:

```sh
{ cmake --build --preset default --target backend_lir_to_bir_notes_test && ctest --test-dir build -j --output-on-failure -R '^backend_lir_to_bir_notes$'; } > test_after.log 2>&1
```

Proof log: `test_after.log`. Result: build succeeded and 1/1 selected test
passed.
