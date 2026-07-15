# Current Packet

Status: Active
Source Idea Path: ideas/open/782_lir_vaarg_phi_input_result_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove the bounded handoff and return to 751

## Just Finished

- Step 3 bounded handoff proof is complete: focused LIR coverage names the
  selected AArch64 GP `reg_addr` GEP and `stack_ptr` load; AArch64 FP `reg_addr`
  GEP and `stack_ptr` load plus `aligned_stack_ptr` ptrmask call (>8-byte) and
  `inttoptr` cast (<=8-byte); and AMD64 `reg_value` plus `stack_value` final
  i32 loads (never the overflow pointer load). The FP malformed module cases
  reject missing, invalid, duplicate, and foreign selected result IDs. This is
  producer-only proof; it neither inspects nor changes PHI construction.

## Suggested Next

- Supervisor: accept the Step 3 test/todo slice, commit it with the focused
  test change, then send 782's exhausted runbook to plan-owner for the explicit
  handoff/closure decision required before 751 resumes Step 1.

## Watchouts

- `LirVaArgOp.result` remains a later result, not a helper-PHI input identity.
- The accepted producer map is: AArch64 GP `reg_addr` selected GEP and
  `stack_ptr` selected load; AArch64 FP `reg_addr` selected GEP, `stack_ptr`
  selected load, and `aligned_stack_ptr` ptrmask call (>8) or `inttoptr` cast
  (<=8); AMD64 `reg_value` final register load and `stack_value` final overflow
  load. Keep 751 limited to its PHI carrier/verifier scope.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^frontend_lir_call_type_ref$'` passed 1/1 after a
  fresh build. Per packet instruction, no root regression log was created;
  this focused test output is the executor evidence.
