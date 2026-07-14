# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.22
Current Step Title: Receive typed conditional-branch authority

## Just Finished

- Step 7.21 received legacy `LirIndirectBr.addr` and ordered current-function
  target IDs into verified Raw-BIR `IndirectJumpTerm` authority, with
  transactional malformed-address/target rollback coverage (`b528dc1`).
- Closed 755 then published `LirCondBr.condition` as the verifier-checked
  current-function `LirValueId`; typed true/false successor authority remains
  the accepted 750 handoff (`c3d759cb8`, `ae006a0f1`).

## Suggested Next

- Execute Step 7.22 only: receive `LirCondBr.condition`,
  `true_successor`, and `false_successor` into one transactional Raw-BIR
  conditional-branch receiver.

## Watchouts

- Never recover semantics from `cond_name`, labels, printer output, or rendered
  operands.
- `LirSwitch`, `LirIndirectBrOp`, PHI, and all other unreceived families remain
  unsupported and fail closed.

## Proof

- Historical Step 7.21: matching `^backend_` guard passed 5/5; prior full
  checkpoint was 3034/3034 passing.
- Closed 755: fresh build, direct frontend proof, matching `^backend_` guard
  5/5 before/after, and broader `^(frontend_cxx_|positive_sema_)` proof 35/35.
- Executor: run a fresh build and focused positive/negative receiver proof.
- Supervisor: select and record regression and broader acceptance separately.
