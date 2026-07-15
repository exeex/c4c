# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.25
Current Step Title: Receive typed PHI incoming authority

## Just Finished

- Closed 787 accepted duplicate typed conditional and switch successor IDs as
  distinct ordered parallel CFG-edge occurrences (`a889ce33f`); it accepted no
  Raw-BIR PHI receiver work. Steps 1 through 7.24 remain accepted historical
  progress. Existing PHI receiver changes and focused 1/1 WIP proof remain
  unaccepted.

## Suggested Next

- Reattempt Step 7.25 only: receive the typed `LirPhiOp` result and ordered
  incoming value/predecessor (including newly authorized SpecialToken) authority
  into Raw BIR with exact ordinary and parallel CFG-edge occurrence validation
  and transactional coverage.

## Watchouts

- Do not recover values, predecessors, edges, or semantics from labels,
  spellings, printer output, LLVM text, or instruction order.
- Preserve parallel edge/incoming multiplicity and order. Do not repeat or
  accept the preserved Raw-BIR WIP merely because its prior interface test
  passed.
- PHI, local/object, memory/va, aggregate/vector, body-parameter, and all
  other later families remain fail-closed outside this packet.

## Proof

- 751 accepted: focused `^frontend_lir_call_type_ref$` proof 1/1; matching
  full regression guard and full after proof 3037/3037.
- 786 accepted: native PHI SpecialToken authority handoff (`91b5bde43`).
- 787 accepted: focused typed parallel CFG-successor coverage (`a889ce33f`);
  no Raw-BIR acceptance follows from that proof.
- Executor: run a fresh build and focused positive/negative Raw-BIR PHI
  receiver proof for Step 7.25, including parallel-edge occurrence coverage.
- Supervisor: select and record broader/full acceptance separately.
