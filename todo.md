Status: Active
Source Idea Path: ideas/open/162_link_name_id_backend_symbol_authority.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Add BIR Pointer Value Identity for Symbol-Carrying Values

# Current Packet

## Just Finished

Step 5 BIR verifier pointer-value identity slice is implemented.

`bir::validate()` now validates `Value::pointer_symbol_link_name_id` on every
named-value validation path, including call args/callee values, local and
global store values, phi/select/cast operands, terminator values, and global
initializer values/elements. ID-backed pointer values must be named pointer
symbol values, reference a known `LinkNameId`, resolve to a declared global or
function by that ID, and must not pair the ID with a different declared
global/function visible name when the display spelling names one. Raw
compatibility pointer values without metadata still pass.

`backend_lir_to_bir_notes_test` now covers compatibility/no-metadata pointer
values, unknown `LinkNameId`, empty symbol names, drifted but undeclared display
spelling with a valid ID, declared global/function display-name mismatches, and
initializer element pointer-value mismatches.

## Suggested Next

Review whether Step 5 has any remaining pointer-value identity surfaces outside
BIR validation; if not, prepare the Step 6 backend handoff around replacing the
remaining raw LIR global/function operand compatibility bridges with structured
symbol identity.

## Watchouts

- BIR validation now rejects an ID-backed pointer `bir::Value` if the ID is
  valid in the module name table but no declared global/function carries it.
  That is intentional for symbol-carrying values, but the supervisor should
  check future string-pool or runtime placeholder work before attaching
  `LinkNameId` to non-module-declared values.
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
