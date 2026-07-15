# Current Packet

Status: Active
Source Idea Path: ideas/open/806_lir_phi_residual_producer_family_authority_trace.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair the postfix-increment old-value producer handoff

## Just Finished

- 806 Step 2 completed: the assignable-load carrier and postfix increment
  handoff retain `LirOperand` authority, so the old-value load reaches the
  ternary PHI with its native current-function `LirValueId`. The focused LIR
  test checks both producer-to-PHI identity and rejection of a missing incoming
  authority.

## Suggested Next

- Supervisor: select the next separately scoped residual producer packet.

## Watchouts

- Direct `20000715-1.c` now prints the repaired postfix PHI with `%t10` and
  `%t12` incoming loads before reaching a later, separately scoped PHI
  producer failure. Do not widen this postfix packet into that residual route,
  floating unary-minus, or bit-not.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure
  -R '^frontend_hir_tests$' > test_after.log 2>&1` passed. The matching
  frontend-HIR before/after guard is non-decreasing (one test binary). The
  focused `build/tests/frontend/frontend_lir_call_type_ref_test` also passed.
  Proof log: `test_after.log`.
