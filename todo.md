# Current Packet

Status: Active
Source Idea Path: ideas/open/778_lir_logical_rhs_result_authority_publication.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair focused assertions and add malformed authority coverage

## Just Finished

- Plan Step 2 complete: only the selected logical integer RHS non-`i1` `zext`
  now sets its existing `requires_native_result_authority` opt-in to `true` at
  construction, retaining `fresh_value(ctx)`, its operand, and result type.
  The focused build succeeds; its selected test is expected to fail at the
  obsolete pre-Step-3 no-authority assertion. PHI result/incoming remains
  unchanged.

## Suggested Next

- Dispatch Plan Step 3: update the focused assertion for the selected cast's
  authority opt-in and add its malformed authority cases. Step 3 owns those
  test changes; do not widen into other producer families.

## Watchouts

- The selected test's current no-authority assertion is deliberately obsolete
  after Step 2; Step 3 owns both its repair and malformed authority coverage.
  The verifier/IR contracts, PHI result/incoming, final logical consumer,
  generic expression APIs, and all other producer families remain excluded.

## Proof

- Step 2 proof: `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^frontend_lir_call_type_ref$'`; build passes and the
  test is expected to fail at its obsolete pre-Step-3 assertion. The supervisor
  owns `test_after.log`; this packet directs no root-log writes.
