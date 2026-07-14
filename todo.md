# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.16
Current Step Title: Receive the checked builtin-ffs Cttz call/Add-one lhs

## Just Finished

- Step 7.15 complete: received producer-verified i32/i64 builtin-ffs native
  integer Eq-zero comparison results and preserved each exact result as the
  matching `LirSelectOp.cond`; malformed comparison/select authority rolls
  back transactionally, while Cttz-result comparisons remain fail-closed.

## Suggested Next

- Execute `plan.md` Step 7.16: the bounded builtin-ffs Cttz-call/Add-one-lhs
  receiver packet.

## Watchouts

- Do not widen Step 7.16 into prepared-argument, Eq-zero condition, or
  select-false-arm receipt work already bounded to prior packets; do not infer
  intrinsic identity, signature, or value linkage from rendered call text.

## Proof

- `cmake --build --preset default` passed, then focused 2/2
  `^backend_lir_to_bir_interface$|^frontend_lir_call_type_ref$` passed for
  Step 7.15; Step 7.16 requires a fresh build and focused 2/2 proof. The
  supervisor owns canonical regression logs and any broader proof.
