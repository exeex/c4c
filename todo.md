# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.8
Current Step Title: Receive the checked explicit scalar FPExt result

## Just Finished

- Step 7.8 complete: received only the producer-verified explicit scalar
  `float`-to-`double` `LirCastOp::FPExt` from the admitted scalar FPTrunc
  result and its one exact later double `FMul` use, preserving native cast,
  endpoint types, source IDs, and current-function SSA edges. Other floating
  casts, malformed/implicit/non-SSA/duplicate/unresolved/cross-owner/wrong
  endpoint inputs, and missing or malformed downstream uses reject
  transactionally.

## Suggested Next

- Send the exhausted Step-7 runbook to plan-owner for an explicit close,
  repair, replacement, or conclude decision; do not infer source completion.

## Watchouts

- Step 7.8 admits only the checked scalar float-to-double FPExt boundary;
  all other casts, non-scalar forms, and presentation-derived authority remain
  fail-closed. Runbook exhaustion is not source completion.

## Proof

- Step 7.8 passed the supervisor-selected proof: `cmake --build --preset
  default && ctest --test-dir build -j --output-on-failure -R
  '^backend_lir_to_bir_interface$|^frontend_lir_call_type_ref$'` (2/2);
  proof log: `test_after.log`.
