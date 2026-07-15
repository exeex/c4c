# Current Packet

Status: Active
Source Idea Path: ideas/open/763_lir_composite_type_ref_model.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish structured composite representation

## Just Finished

- Step 1: added structured `LirTypeRef` array element/length facts and named
  struct/union kind facts, including structural equality and focused coverage;
  existing text mirrors remain compatible.

## Suggested Next

- Execute Step 2 only: add the rendering-boundary helper and migrate the
  selected struct-layout padding/storage array construction.

## Watchouts

- `runtime_text` keeps legacy scalar-kind classification for verifier
  compatibility, but does not populate the new array composite facts. Do not
  parse rendered type text to recover semantic structure.

## Proof

- Passed: `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_'` (5/5); log: `test_after.log`.
- Passed focused direct coverage: `build/tests/frontend/frontend_lir_extern_decl_type_ref_test`.
