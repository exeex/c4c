Status: Active
Source Idea Path: ideas/open/162_link_name_id_backend_symbol_authority.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Add BIR Pointer Value Identity for Symbol-Carrying Values

# Current Packet

## Just Finished

Step 5 first narrow slice is implemented.

`bir::Value` now has `pointer_symbol_link_name_id` plus
`Value::named_symbol_pointer()` for named pointer values that denote
link-visible globals/functions. Direct function pointer call arguments now set
the carrier from `FunctionSymbolSet`, byval global aggregate call arguments set
it from `GlobalInfo::link_name_id`, and synthesized dynamic global pointer-array
values preserve the `GlobalAddress::link_name_id` they were resolved from.

`backend_lir_to_bir_notes_test` now covers one direct function pointer value and
one byval global pointer route, including equality checks that prove the
structured carrier is not equivalent to raw display-only `@symbol` spelling.

## Suggested Next

Continue Step 5 by extending the new pointer-value carrier into aggregate
initializer element values and pointer stores, then add verifier coverage for
pointer-value `LinkNameId` mismatches before moving to Step 6 backend
preparation.

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
- Compatibility bridge: aggregate initializer element values still render
  pointer fields as `bir::Value::named(TypeKind::Ptr, "@symbol")` even when
  `lower_aggregate_initializer_recursive()` records the same field in
  `pointer_initializer_offsets`. Owner: BIR aggregate initializer value model.
  Limitation: semantic identity is preserved in the `GlobalAddress` side table
  used by lowering/provenance, but the public initializer element carrier has no
  per-element `LinkNameId`. Removal condition: add structured pointer identity
  to BIR initializer elements or replace raw pointer initializer values with an
  ID-backed carrier consumed by validation and backend preparation.
- Compatibility bridge: byval aggregate call arguments that pass an addressable
  global now lower to `bir::Value::named_symbol_pointer()` with the target
  global `LinkNameId`. Limitation: this is proven for direct byval global
  arguments; local aggregate aliases and byval copies remain route-local
  handles by design.
- Compatibility bridge: dynamic pointer-array materialization from aggregate
  pointer initializer offsets now materializes ID-backed arm values from
  `GlobalAddress::link_name_id`. Limitation: the selected value is still a
  plain `SelectInst` result without per-arm semantic validation in the verifier.

## Proof

Passed:

```sh
{ cmake --build --preset default --target backend_lir_to_bir_notes_test && ctest --test-dir build -j --output-on-failure -R '^backend_lir_to_bir_notes$'; } > test_after.log 2>&1
```

Proof log: `test_after.log`. Result: build succeeded and 1/1 selected test
passed.
