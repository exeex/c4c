# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.16
Current Step Title: Receive the checked builtin-ffs Cttz call/Add-one lhs

## Just Finished

- Step 7.16 complete: proved the existing structured intrinsic receiver retains
  only i32/i64 native defined-zero Cttz results as the exact same-width Add-one
  lhs, with malformed result, callee/signature/behavior, and result-linkage
  authority rolling back transactionally.

## Suggested Next

- Send the exhausted runbook to plan-owner for an explicit source-completion,
  repair, replacement, or conclusion decision.

## Watchouts

- The Step 7.16 coverage is isolated to Cttz-to-Add-one: it adds no
  prepared-argument, Eq-zero condition, or select-false-arm receipt and uses
  structured authority rather than rendered call text.

## Proof

- `cmake --build --preset default` passed, then focused 2/2
  `^backend_lir_to_bir_interface$|^frontend_lir_call_type_ref$` passed for
  Step 7.16; output is in `test_after.log`. The supervisor owns canonical
  regression-log roll-forward and any broader proof.
