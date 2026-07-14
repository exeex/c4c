# Current Packet

Status: Active
Source Idea Path: ideas/open/768_lir_computed_goto_label_address_table_initialization_authority_decomposition.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Extract direct frontend-LIR producer probes

## Just Finished

- Plan Step 3: added and registered
  `frontend_lir_label_address_table_decay_probe` for an automatic local table
  of label addresses consumed as a pointer before indexing. The harness fixes
  the immediate structured `LirGepOp` table-decay seam without accepting the
  current raw/no-ID producer result.

## Suggested Next

- Select a capability that gives the local table slot and its zero-index decay
  GEP/result typed authority: a valid current-function pointer base, typed zero
  indices, and nearby malformed raw, invalid, foreign, and non-pointer rejection
  coverage.

## Watchouts

- The table-decay harness intentionally asserts only the local table slot and
  structured GEP shape, not emitted text or the present raw/no-ID result.
- Do not reinterpret raw label-address or decay operands as semantic authority;
  retain malformed raw, invalid, foreign, and non-pointer rejection coverage
  beside the selected capability.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_' > test_after.log`
  (fresh build plus the supervisor-selected frontend-LIR subset; log: `test_after.log`).
