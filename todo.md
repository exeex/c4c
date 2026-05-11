Status: Active
Source Idea Path: ideas/open/162_link_name_id_backend_symbol_authority.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Propagate LinkNameId Through LIR to BIR

# Current Packet

## Just Finished

Completed Step 4 LIR-to-BIR aggregate pointer-initializer and runtime
global-address provenance authority fix across
`src/backend/bir/lir_to_bir/`.

`GlobalAddress` now carries `LinkNameId` provenance. Aggregate pointer
initializer offsets propagate the single available initializer function
`LinkNameId`, function-address checks prefer that ID over raw spelling, and
runtime/local pointer provenance records preserve the resolved function ID when
they must bridge from raw LIR global operands. Focused backend tests cover
drifted aggregate function-field spelling and reject a valid aggregate
initializer function `LinkNameId` miss that raw spelling would otherwise rescue.

## Suggested Next

Continue Step 4 by auditing direct pointer argument/value alias routes where
LIR operands still expose only raw `@symbol` text, then either thread structured
operand identity or keep the bridge documented until LIR supplies it.

## Watchouts

- Compatibility bridge: direct `LirOperandKind::Global` function operands in
  pointer values still enter LIR-to-BIR as raw `@symbol` text. Owner: LIR
  operand metadata. Limitation: LIR-to-BIR can resolve and preserve the
  function `LinkNameId` after raw lookup, but cannot make the operand spelling
  itself non-authoritative until the operand carries structured identity.
  Removal condition: replace raw operand lookup when LIR global operands expose
  `LinkNameId` or equivalent symbol identity.
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
