# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.23
Current Step Title: Receive typed switch authority
你該做code review了

## Just Finished

- Closed 756 after accepted Step 1: `LirSwitch.selector` now carries verified
  current-function value authority; its default and ordered case successor
  carriers remain the authority accepted by 750.

## Suggested Next

- Execute Step 7.23 only: receive typed `LirSwitch` selector/default/case
  authority into Raw-BIR with transactional validation.

## Watchouts

- The receiver reads only `selector`, `default_successor`, and
  `case_successors`; selector/label display shadows remain outside semantic
  lowering.
- `LirIndirectBrOp`, PHI, and all other unreceived families remain unsupported
  and fail closed.

## Proof

- 756 accepted: fresh build; focused `^frontend_lir_` 4/4 with matching
  non-decreasing guard; broader `^(frontend_cxx_|positive_sema_)` 35/35.
- Executor: run a fresh build and focused receiver proof for Step 7.23.
- Supervisor: select and record broader/full acceptance separately.
