# Current Packet

Status: Active
Source Idea Path: ideas/open/768_lir_computed_goto_label_address_table_initialization_authority_decomposition.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Extract direct frontend-LIR producer probes

## Just Finished

- Plan Step 3: added the focused `frontend_lir_label_address_rvalue_probe`
  diagnostic for an automatic local scalar initialized directly by `&&label`.
  The harness fixes the immediate structured `LirStoreOp` pointer-store seam;
  producer authority remains incomplete and is not accepted by this probe.

## Suggested Next

- Select and implement a producer capability only after preserving the probe's
  required contract: typed pointer operand with valid current-function authority,
  plus nearby malformed invalid, foreign, and non-pointer rejection coverage.

## Watchouts

- The four-case missing-`addr_value` family predates 769 and is 768 producer
  evidence, not a global-initializer regression or a Raw-BIR/importer boundary.
- Do not reinterpret the raw direct label-address operand observed by this probe
  as semantic authority. The probe intentionally leaves that producer boundary
  unasserted until a typed authority capability is selected.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_' > test_after.log`
  (fresh build plus the supervisor-selected frontend-LIR subset; log: `test_after.log`).
