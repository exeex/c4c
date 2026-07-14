# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.4
Current Step Title: Receive the checked fixed-void external double call result

## Just Finished

- Step 7.4 complete: received only the resolved external `double target(void)`
  direct-call result and its exact later double `FAdd` lhs use. The importer
  requires one bodyless module declaration with the shared LinkNameId, native
  double return, and structured fixed-void signature; it preserves the caller
  result/source ordering and rejects non-double, unresolved, duplicate,
  malformed, or cross-owner call/use authority without admitting FAdd broadly.

## Suggested Next

- Send the exhausted runbook to plan-owner for its explicit completion,
  repair, replacement, or conclusion decision after the supervisor-owned
  regression gate; do not infer source-idea completion from this packet.

## Watchouts

- Step 7.32 remains deliberately narrow: only the resolved fixed-void external
  native-double call and its typed downstream FAdd lhs are admitted. FAdd
  itself, `float`/other floating, indirect, variadic, argument-bearing, ABI,
  aggregate/object, ffs comparison/Select, and presentation recovery remain
  fail-closed.

## Proof

- Step 7.4 passed: `cmake --build --preset default && ctest --test-dir build
  -j --output-on-failure -R
  '^backend_lir_to_bir_interface$|^frontend_lir_call_type_ref$'` (2/2);
  proof log: `test_after.log`. The focused proof is sufficient for this packet;
  the supervisor owns the regression gate and runbook disposition.
