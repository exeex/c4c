# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.24
Current Step Title: Receive typed computed-goto authority

## Just Finished

- Closed 757 after accepted Step 1: `LirIndirectBrOp.addr_value` now carries
  verified current-function pointer authority; ordered `successors` remains
  the target authority accepted by 750.

## Suggested Next

- Execute Step 7.24 only: receive typed `LirIndirectBrOp` address/successor
  authority into Raw-BIR with transactional validation.

## Watchouts

- The receiver reads only `addr_value` and ordered `successors`; address/label
  display shadows remain outside semantic lowering.
- PHI and all other unreceived families remain unsupported and fail closed.

## Proof

- 757 accepted: fresh build; focused `^frontend_lir_call_type_ref$` 1/1; and
  matching non-decreasing `^backend_` regression guard 5/5 before and after.
- Executor: run a fresh build and focused receiver proof for Step 7.24.
- Supervisor: select and record broader/full acceptance separately.
