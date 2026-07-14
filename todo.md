# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.2
Current Step Title: Receive the checked i32 integer Abs result

## Just Finished

- Step 7.1 complete: added one mixed source-order fixture for the existing
  selected-global i32 Load, normalized i32 Add, explicit i32-to-i64 SExt, and
  selected i32 SLT dispatcher branches. It proves typed current-function
  result/use authority and that changing only the final compare predicate
  rejects the entire module without Raw-BIR publication.

## Suggested Next

- Executor packet: receive only the authority-matrix Step-7.4 i32
  `LirAbsOp` subrow whose selected-global i32 Load argument and later i32 Add
  use already have admitted typed authority. Add its Raw-BIR container,
  importer, reachable verifier, and focused positive/transactional-negative
  coverage; do not generalize to `labs`, `llabs`, immediate inputs, other
  builtins/calls, or noninteger/vector/aggregate forms.

## Watchouts

- Step 7.2 is one new, tagged Abs receipt family, not a general call or
  intrinsic route. The existing comparison normalization `ZExt`, `labs`/
  `llabs`, immediate-argument variants, all noninteger/aggregate/vector forms,
  and every other unsupported instruction family remain fail-closed.

## Proof

- Step 6.5 passed: `cmake --build --preset default && ctest --test-dir build
  -j --output-on-failure -R
  '^backend_lir_to_bir_interface$|^frontend_lir_call_type_ref$'` (2/2);
  proof log: `test_after.log`.
- Step 7.1 passed: `cmake --build --preset default && ctest --test-dir build
  -j --output-on-failure -R
  '^backend_lir_to_bir_interface$|^frontend_lir_call_type_ref$'` (2/2);
  no test log was changed by this packet.
