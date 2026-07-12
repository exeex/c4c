# Current Packet

Status: Active
Source Idea Path: ideas/open/716_prepared_call_plan_cursor_complete_production.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Produce one cursor-exact plan per supported call

## Just Finished

- Implemented plan Step 2's common producer rule: semantic `CallInst::args`
  now provide the base boundary identity without optional `arg_sources`;
  unique compatible relationships refine it, while duplicate, out-of-range,
  and operand-contradicting relationships reject the call. Added direct-extern
  coverage for exact cursors 0 and 1 plus positive refinement and all three
  negative relationship states.
- The owned direct-extern tests build and advance past the new assertions, but
  the delegated whole `backend_x86_handoff_boundary` executable later fails in
  the unowned joined-branch fixture with `x86 module route did not emit
  register-source shared-publication edge moves`.
- Follow-up repaired two general semantic boundary gaps: a valid
  `callee_link_name_id` now makes an ID-only call available, and absence of an
  optional argument relationship is accepted before relationship-only name
  checks, including semantic symbol-pointer operands with an empty raw name.
  `backend_prepare_frame_stack_call_contract` now advances beyond the
  LinkNameId and call-argument source-shape contracts to its unrelated
  block-entry publication identity assertion.
- Acceptance follow-up after `80a5388a8` repaired the remaining call-owned
  regressions. The narrow call-view fixture now supplies a compatible explicit
  relationship, and a unique relationship's `source_value_id` refines the
  prepared call argument identity. This restores Route 6's agreed prepared
  source while leaving absent metadata semantic-first and malformed evidence
  fail closed.

## Suggested Next

- Supervisor should determine whether the joined-branch failure is a known
  baseline failure or delegate its diagnosis to that fixture's owner, then
  rerun the exact Step 2 proof.

## Watchouts

- The proof blocker is outside this packet's owned files:
  `backend_x86_handoff_boundary_joined_branch_test.cpp:8350`. The failing
  module contains no calls, so the direct semantic-call producer route does
  not directly explain the missing edge-publication moves.
- `backend_prepared_lookup_helper` independently fails in the same prepared
  MIR join/block-entry identity family. Inspection found no causal path from
  call-plan production, so its test was not edited and ownership was not
  expanded.
- The acceptance subset has no earlier or new call-owned failure: call view
  and route debug are green, and x86 handoff reaches only the known later
  joined-branch edge-publication failure.

## Proof

- Ran `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R
  '^(backend_prepare_frame_stack_call_contract|backend_x86_handoff_boundary)$'`;
  build succeeded. The call contract advanced to the unrelated prepared
  block-entry publication identity assertion, and x86 retained the same
  unrelated joined-branch assertion. Complete output is in `test_after.log`.
- Acceptance follow-up ran `cmake --build --preset default && ctest --test-dir
  build -j --output-on-failure -R
  '^(backend_bir_call_comparison_view_contract|backend_x86_route_debug|backend_x86_handoff_boundary)$'`.
  Call comparison and route debug passed; x86 handoff failed only at the known
  later joined-branch assertion. Complete combined output is in
  `test_after.log`.
