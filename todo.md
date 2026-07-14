# Current Packet

Status: Active
Source Idea Path: ideas/open/768_lir_computed_goto_label_address_table_initialization_authority_decomposition.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Bind probes and select the narrowest generic producer seam

## Just Finished

- Plan Step 3 is complete: the static-storage initializer row is complete via
  769 (`56d86556a`), including structured owner/target positive and malformed
  verifier coverage; the static-table decay control remains satisfied by 767
  (`403e86afd`). The direct `&&label` automatic-local harness
  (`707062aeb`) legitimately covers both the automatic-scalar-initializer and
  direct-rvalue rows at their shared immediate `LirStoreOp` seam, while the
  automatic-table decay harness (`85ac8d42c`) fixes its local-slot/two-index
  `LirGepOp` seam.
- `22bd5885b` records the remaining automatic-form future positive/malformed
  producer-authority contracts. These records complete contract extraction;
  they do not accept current raw/no-ID behavior or select a production seam.

## Suggested Next

- Bind all focused rows to their direct generic producer/result contracts and
  compare their ownership before choosing any repair. Authorize one narrow
  producer seam only if it satisfies the static initializer, static-table
  control, direct-label rvalue/store, and automatic-table decay maps without
  raw/no-ID authority, carrier changes, or testcase-shaped routing; otherwise
  name the exact separately scoped blocker and proof packet.

## Watchouts

- The automatic probes establish source-form/immediate-consumer seams and their
  future malformed contracts; neither makes current raw/no-ID operands semantic
  authority.
- Step 4 is a selection decision only. Do not implement, reinterpret raw
  label-address or decay operands as authority, or reopen 767/769.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_' > test_after.log`
  (fresh build plus the supervisor-selected frontend-LIR subset; log: `test_after.log`).
