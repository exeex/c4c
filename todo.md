# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.21
Current Step Title: Receive legacy indirect-branch authority

## Just Finished

- Resumed from accepted Step 7.20 and the closed 750 CFG successor-authority
  handoff; historical steps remain accepted and are not reset.

## Suggested Next

- Execute Step 7.21 only: receive legacy `LirIndirectBr` typed address and
  ordered current-function targets into Raw BIR.

## Watchouts

- Do not receive `LirCondBr`, `LirSwitch`, or `LirIndirectBrOp`; their
  non-target authority remains outside this packet and must fail closed.

## Proof

- Executor: run a fresh build and focused positive/negative receiver proof.
- Supervisor: select and record broader/full acceptance separately.
