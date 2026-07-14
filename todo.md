# Current Packet

Status: Step 1 complete; awaiting plan-owner completion decision
Source Idea Path: ideas/open/758_lir_rvalue_value_identity_preservation_for_computed_goto.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Preserve typed local and parameter rvalue identity

## Just Finished

- Plan Step 1 complete: eligible current-function local and parameter load
  identities now survive the rvalue expression/operand carrier into fixed
  typed call arguments; display spelling remains a compatibility mirror.

## Suggested Next

- Ask plan-owner to decide 758 completion, then resume 757 Step 1 at its
  recorded return point; do not extend 758 into computed-goto work.

## Watchouts

- The preserved result is exact `LirValueId` propagation for local and
  parameter loads through `emit_rval_payload`/`emit_rval_expr`/
  `emit_rval_operand` and the fixed call-argument carrier. Absent, invalid,
  foreign, and non-value authority fail at the typed-call verifier boundary;
  no display text is parsed or recovered. Return to 757 Step 1 only after the
  plan-owner records 758's completion decision.

## Proof

- Passed: `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^frontend_lir_call_type_ref$'`. Per the delegated
  packet, no canonical regression log was written; the supervisor owns those
  logs and broader acceptance.
