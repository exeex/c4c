Status: Active
Source Idea Path: ideas/open/162_link_name_id_backend_symbol_authority.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Add BIR Pointer Value Identity for Symbol-Carrying Values

# Current Packet

## Just Finished

Step 5 aggregate/store pointer-value identity slice is implemented.

Aggregate pointer initializer element values now reuse the resolved
`GlobalAddress::link_name_id` through a byte-offset to initializer-element index
bridge, so covered function and global pointer fields publish an ID-backed
`bir::Value` instead of remaining display-only `@symbol` values. Direct pointer
stores of known function and global symbols now build ID-backed stored pointer
values before local/global/provenance store routes consume them, and local
pointer-slot reload aliases recover the same carrier for subsequent pointer
value uses.

`backend_lir_to_bir_notes_test` now covers aggregate initializer element
carriers for one function pointer field and one global pointer field, direct
local pointer stores for function and global symbols, and store/load recovery
into a later call argument. The assertions include equality checks proving these
values are not equivalent to raw display-only pointer values.

## Suggested Next

Continue Step 5 by adding verifier coverage for pointer-value
`LinkNameId`/display-name mismatches on ID-backed `bir::Value` carriers, then
decide whether any remaining compatibility bridges are narrow enough to carry
into Step 6 backend preparation.

## Watchouts

- Compatibility bridge: direct `LirOperandKind::Global` function operands used
  as pointer argument values now produce ID-backed BIR pointer values when the
  raw operand resolves through `FunctionSymbolSet`. Limitation: the LIR operand
  still enters as textual `@symbol`; drifted raw operand names are not fixed
  until LIR operands carry `LinkNameId` directly.
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
- Compatibility bridge: aggregate initializer element values now receive
  `LinkNameId` through `GlobalInfo::pointer_initializer_value_indices`, which is
  derived while parsing the current flattened initializer element vector. Owner:
  BIR aggregate initializer value model. Limitation: this is still a bridge
  from byte offsets to flattened value indices; a future initializer element
  model with explicit offsets would remove that coupling.
- Compatibility bridge: byval aggregate call arguments that pass an addressable
  global now lower to `bir::Value::named_symbol_pointer()` with the target
  global `LinkNameId`. Limitation: this is proven for direct byval global
  arguments; local aggregate aliases and byval copies remain route-local
  handles by design.
- Compatibility bridge: dynamic pointer-array materialization from aggregate
  pointer initializer offsets now materializes ID-backed arm values from
  `GlobalAddress::link_name_id`. Limitation: the selected value is still a
  plain `SelectInst` result without per-arm semantic validation in the verifier.
- Compatibility bridge: direct pointer stores of function/global LIR operands
  now preserve `LinkNameId` on the BIR stored value when the raw operand can be
  resolved through `FunctionSymbolSet` or `GlobalTypes`. Limitation: the initial
  LIR store operand is still textual; drifted raw store values cannot be
  repaired until LIR operands carry symbol identity directly.

## Proof

Passed:

```sh
{ cmake --build --preset default --target backend_lir_to_bir_notes_test && ctest --test-dir build -j --output-on-failure -R '^backend_lir_to_bir_notes$'; } > test_after.log 2>&1
```

Proof log: `test_after.log`. Result: build succeeded and 1/1 selected test
passed.
