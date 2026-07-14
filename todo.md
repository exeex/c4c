# Current Packet

Status: Active
Source Idea Path: ideas/open/768_lir_computed_goto_label_address_table_initialization_authority_decomposition.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Extract direct frontend-LIR producer probes

## Just Finished

- Completed plan.md Step 2 producer mapping in
  `docs/lir_computed_goto_label_address_authority/step2_producer_map.md`.
  The source-form-neutral map separates static-storage constant initialization,
  automatic scalar initialization, automatic/static table representation, and
  direct rvalue consumption before the accepted table-element GEP/load result.
  It preserves 765/766/767 and selects no repair.

## Suggested Next

- Step 3: extract one focused frontend-LIR production probe for each mapped
  form, with its structured positive assertion and nearby malformed contract;
  do not use external integration cases, rendered text, carrier publication,
  verifier changes, or any rework of 765/766/767.

## Watchouts

- The static-storage decay row is a structured control form, while local-array
  decay and direct label-address rvalue production reach raw presentation
  earlier. This is a map observation, not a selected seam: Step 3 must prove
  each row with focused positive/malformed contracts before any repair choice.

## Proof

- Documentation/evidence-only proof: ran the AST definition queries recorded
  in `docs/lir_computed_goto_label_address_authority/step2_producer_map.md`
  for `emit_rval_payload`, `emit_const_scalar_expr`,
  `emit_rval_from_access_ptr`, and `emit_control_flow_stmt`, then ran
  `git diff --check`. No build, CTest, or `test_after.log` was required or
  written by this packet.
