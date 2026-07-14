# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.15
Current Step Title: Receive the checked builtin-ffs zero-comparison/select condition

## Just Finished

- Step 7.14 complete: received the checked i32/i64 builtin-ffs native Add-one
  result and its exact `LirSelectOp.false_val` edge in `b0f4c4a93`, with
  focused 2/2 and fresh `^backend_` 4/4 proof.

## Suggested Next

- Execute `plan.md` Step 7.15: the bounded builtin-ffs zero-comparison/select-
  condition receiver packet.

## Watchouts

- Do not receive the prepared argument, Cttz call, or Add-one false arm while
  receiving only the exact Eq-zero comparison-to-condition edge.

## Proof

- Required after implementation: a fresh build and focused 2/2
  `^backend_lir_to_bir_interface$|^frontend_lir_call_type_ref$` proof. The
  supervisor selects any broader proof and owns canonical regression logs.
