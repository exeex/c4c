# Current Packet

Status: Active
Source Idea Path: ideas/open/768_lir_computed_goto_label_address_table_initialization_authority_decomposition.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Extract direct frontend-LIR producer probes

## Just Finished

- Plan Step 3 disposition: the static-storage initializer row is complete via
  769 (`56d86556a`), including structured owner/target positive and malformed
  verifier coverage; the static-table decay control remains satisfied by 767
  (`403e86afd`). The direct `&&label` automatic-local harness
  (`707062aeb`) legitimately covers both the automatic-scalar-initializer and
  direct-rvalue rows at their shared immediate `LirStoreOp` seam, while the
  automatic-table decay harness (`85ac8d42c`) fixes its local-slot/two-index
  `LirGepOp` seam.
- Step 3 is not complete: both automatic probes intentionally decline the
  required explicit positive/malformed producer-authority contract. No
  production seam has been selected.

## Suggested Next

- Finish Step 3's contract extraction without choosing an implementation seam:
  record the direct-label rvalue/store positive as a typed pointer producer with
  valid enclosing-function and target-label identity plus a valid produced value
  identity, and require rejection of raw, invalid/foreign owner or target,
  non-pointer, and missing/invalid/foreign-value variants. Record the
  automatic-table decay positive as a current-function local-slot base with two
  typed zero indices and a valid GEP result identity, and require rejection of
  raw/non-pointer/invalid/foreign bases, raw or wrongly typed indices, and
  missing/invalid/foreign results. Then re-evaluate all five rows for Step 4.

## Watchouts

- The two automatic probes currently establish source-form/immediate-consumer
  seams only; their green results do not make raw/no-ID operands authority or
  satisfy the Step 3 malformed-contract requirement.
- Do not reinterpret raw label-address or decay operands as semantic authority,
  select a producer seam, or reopen 767/769 while completing this extraction.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_' > test_after.log`
  (fresh build plus the supervisor-selected frontend-LIR subset; log: `test_after.log`).
