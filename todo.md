# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.25
Current Step Title: Receive typed PHI incoming authority

## Just Finished

- Closed 751 accepted typed `LirPhiIncoming` value and predecessor authority
  for ternary, logical, AArch64-vaarg, and AMD64-vaarg producers; Steps 1
  through 7.24 of this receiver route remain accepted historical work.

## Suggested Next

- Execute Step 7.25 only: receive the typed `LirPhiOp` result and ordered
  incoming value/predecessor authority into Raw BIR with exact CFG-edge
  occurrence validation and transactional coverage.

## Watchouts

- Do not recover values, predecessors, edges, or semantics from labels,
  spellings, printer output, LLVM text, or instruction order.
- Preserve parallel edge/incoming multiplicity and order; PHI, local/object,
  memory/va, aggregate/vector, body-parameter, and all other later families
  remain fail-closed outside this packet.

## Proof

- 751 accepted: fresh focused `^frontend_lir_call_type_ref$` proof 1/1; full
  after proof and matching full regression guard both passed 3037/3037.
- Executor: run a fresh build and focused positive/negative Raw-BIR PHI
  receiver proof for Step 7.25.
- Supervisor: select and record broader/full acceptance separately.
