# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.14
Current Step Title: Receive the checked builtin-ffs add-one/select false arm

## Just Finished

- Step 7.13 complete: received the producer-verified operand-free i64 ffs
  `LirSelectOp`, its exact i64-to-i32 `LirCastOp::Trunc`, and later ordinary
  i32 `Add` in `f88276157`, with focused 2/2 and fresh `^backend_` 4/4 proof.

## Suggested Next

- Execute `plan.md` Step 7.14: the bounded builtin-ffs add-one/select false-arm
  receiver packet.

## Watchouts

- Do not receive the ffs Cttz lhs, equality-to-zero condition, or select
  condition while receiving only the exact Add-one false-arm edge.

## Proof

- Required after implementation: a fresh build and focused 2/2
  `^backend_lir_to_bir_interface$|^frontend_lir_call_type_ref$` proof. The
  supervisor selects any broader proof and owns canonical regression logs.
