# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.7
Current Step Title: Receive the checked explicit scalar FPTrunc result

## Just Finished

- Step 7.6 complete: received only the producer-verified ordinary scalar
  floating `double` `LirCmpOp` `OLt` result whose lhs is the admitted `FMul`
  result, preserving native mode, predicate, type, and current-function source
  ID. The exact `ZExt i1 to i32` is validated as one downstream compatibility
  use but is not received as a Raw-BIR result; malformed/missing/invalid/
  duplicate/unresolved/cross-owner/mode/predicate/type/use-linkage inputs
  reject transactionally.

## Suggested Next

- Execute Step 7.7 as one bounded receiver packet: import only the
  producer-verified explicit scalar double-to-float `LirCastOp::FPTrunc` from
  an admitted floating source and its exact later float FMul use. Do not widen
  floating casts, infer source authority from text, or receive implicit,
  pointer/bitcast/vector/complex/aggregate, or monostate-source forms.

## Watchouts

- This is an in-scope runbook repair, not source completion. Step 7.7 admits
  only the checked FPTrunc boundary; all other casts, non-scalar forms, and
  presentation-derived authority remain fail-closed.

## Proof

- Step 7.6 passed the supervisor-selected proof: `cmake --build --preset
  default && ctest --test-dir build -j --output-on-failure -R
  '^backend_lir_to_bir_interface$|^frontend_lir_call_type_ref$'` (2/2);
  proof log: `test_after.log`. The matching regression guard passed 2/2
  before/after, and fresh broader `^backend_` proof passed 4/4.
