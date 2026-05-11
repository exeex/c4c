Status: Active
Source Idea Path: ideas/open/162_link_name_id_backend_symbol_authority.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Propagate LinkNameId Through LIR to BIR

# Current Packet

## Just Finished

Continued Step 4 by auditing remaining non-direct pointer `@symbol` carriers in
aggregate copy/materialization and pointer aggregate initializer routes.

No additional local repair was available inside the Step 4 carrier shape:
aggregate initializer parsing already records `GlobalAddress` in
`pointer_initializer_offsets`, but the public BIR pointer value carriers for
aggregate initializer elements, byval aggregate call arguments, and synthesized
dynamic pointer-array selections are still plain `bir::Value::named(ptr,
"@symbol")` values with no per-value `LinkNameId` field.

## Suggested Next

Continue Step 4 by checking whether the remaining raw pointer-value bridges can
be deferred to Step 5 validation/backend preparation unchanged, or whether the
runbook needs a later BIR value identity extension before closure.

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
  global still lower to `bir::Value::named(TypeKind::Ptr, "@global")` after
  validating the global through `global_types`. Owner: BIR call argument value
  model. Limitation: the call arg `bir::Value` has no slot for the target
  global `LinkNameId`, and byval ABI metadata only describes the copied bytes.
  Removal condition: add ID-backed pointer values or an address/provenance field
  to call arguments, then consume `GlobalInfo::link_name_id` for byval globals.
- Compatibility bridge: dynamic pointer-array materialization from aggregate
  pointer initializer offsets selects among raw `@symbol` pointer values even
  though each element was first resolved from `GlobalAddress`. Owner: synthesized
  pointer-array select/BIR value model. Limitation: `synthesize_pointer_array_selects()`
  and the resulting `ValueMap` alias carry only `bir::Value`, so per-arm
  `LinkNameId` provenance cannot survive the selected pointer value. Removal
  condition: represent pointer selects with structured per-arm symbol identity
  or a first-class `GlobalAddress` result that later lowering can consume.

## Proof

Passed:

```sh
{ cmake --build --preset default --target backend_lir_to_bir_notes_test && ctest --test-dir build -j --output-on-failure -R '^backend_lir_to_bir_notes$'; } > test_after.log 2>&1
```

Proof log: `test_after.log`. Result: build succeeded and 1/1 selected test
passed.
