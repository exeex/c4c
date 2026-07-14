# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.8
Current Step Title: Receive the checked explicit scalar FPExt result

## Just Finished

- Step 7.7 complete: received only the producer-verified explicit scalar
  `double`-to-`float` `LirCastOp::FPTrunc` from the admitted double `FMul`
  result and its one exact later float `FMul` use, preserving native cast,
  endpoint types, source IDs, and current-function SSA edges. Other floating
  casts, malformed/implicit/non-SSA/duplicate/unresolved/cross-owner/wrong
  endpoint inputs, and missing or malformed downstream uses reject
  transactionally.

## Suggested Next

- Execute Step 7.8 as one bounded receiver packet: import only the
  producer-verified explicit scalar float-to-double `LirCastOp::FPExt` from an
  admitted floating source and its exact later double FMul use. Do not widen
  floating casts or receive implicit, no-op, pointer/bitcast/vector/complex/
  aggregate, monostate-source, or text-derived forms.

## Watchouts

- This is an in-scope runbook repair, not source completion. Step 7.8 admits
  only the checked scalar float-to-double FPExt boundary; all other casts,
  non-scalar forms, and presentation-derived authority remain fail-closed.

## Proof

- Step 7.7 passed the supervisor-selected proof: `cmake --build --preset
  default && ctest --test-dir build -j --output-on-failure -R
  '^backend_lir_to_bir_interface$|^frontend_lir_call_type_ref$'` (2/2);
  proof log: `test_after.log`. The matching regression guard passed 2/2
  before/after, and fresh broader `^backend_` proof passed 4/4.
