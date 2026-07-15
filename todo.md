# Current Packet

Status: Active
Source Idea Path: ideas/open/778_lir_logical_rhs_result_authority_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Opt the selected logical RHS cast into native result authority

## Just Finished

- Plan Step 1 allocation fact accepted in `3b716c12d`: the logical integer RHS
  non-`i1` `zext` allocates its `LirCastOp.result` through `fresh_value(ctx)`
  and retains that native `LirOperand` at the producer. Its focused proof is
  intentionally incomplete because the selected test still asserts the old
  no-authority state; PHI result/incoming remains unchanged.

## Suggested Next

- Dispatch Plan Step 2: at construction of only the selected logical RHS
  non-`i1` conversion, set existing `LirCastOp.requires_native_result_authority`
  to `true`. Retain `fresh_value(ctx)`; do not change verifier/IR contracts.

## Watchouts

- This packet changes only the selected RHS cast's existing opt-in flag. The
  verifier contract is already accepted: it requires a result and selects
  duplicate/foreign ownership checks only for flagged casts. PHI
  result/incoming, final logical consumer, generic expression APIs, and all
  other producer families remain excluded. Step 3, not this packet, repairs
  the obsolete no-authority assertion and adds malformed coverage.

## Proof

- Remaining focused proof for Steps 2–3: `cmake --build --preset default &&
  ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$'`.
  Current `test_before.log` is the expected 1/1 failure at the obsolete
  assertion. Do not accept a full-suite candidate until that assertion is
  repaired and the candidate adds no failure; this runbook directs no root-log
  writes for the focused proof.
