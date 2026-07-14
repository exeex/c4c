# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.7
Current Step Title: Receive the checked explicit scalar FPTrunc result

## Just Finished

- Step 7.7 complete: received only the producer-verified explicit scalar
  `double`-to-`float` `LirCastOp::FPTrunc` from the admitted double `FMul`
  result and its one exact later float `FMul` use, preserving native cast,
  endpoint types, source IDs, and current-function SSA edges. Other floating
  casts, malformed/implicit/non-SSA/duplicate/unresolved/cross-owner/wrong
  endpoint inputs, and missing or malformed downstream uses reject
  transactionally.

## Suggested Next

- Supervisor to select the next bounded active-plan packet.

## Watchouts

- This is an in-scope runbook repair, not source completion. The FPTrunc
  boundary remains limited to the checked scalar double-to-float producer/use
  chain; all other casts, non-scalar forms, and presentation-derived authority
  remain fail-closed.

## Proof

- Step 7.7 passed the supervisor-selected proof: `cmake --build --preset
  default && ctest --test-dir build -j --output-on-failure -R
  '^backend_lir_to_bir_interface$|^frontend_lir_call_type_ref$'` (2/2);
  proof log: `test_after.log`.
