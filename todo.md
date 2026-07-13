# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 4.1
Current Step Title: Receive direct selected-global scalar integer stores

## Just Finished

- Closed idea 741 after its exhaustive 38/38 instruction plus 6/6 terminator
  audit, four authoritative producer carrier contracts, fresh focused/full
  proof, and committed exact handoff to idea 734.
- Reactivated idea 734 at the first receiver-owned function-body subrow. No
  new-BIR instruction or non-void terminator receipt is claimed by the handoff.

## Suggested Next

- Implement only the direct selected-global scalar integer `LirStoreOp` row:
  typed Raw-BIR Store payload, `LinkNameId` global mapping,
  `LirIntegerImmediate` ordinary value materialization, ordered use edges,
  reachable verification, neighboring malformed/raw rejection, and
  module-transactional proof.

## Watchouts

- Do not parse the store's value or pointer display, match the focused testcase,
  or add a producer-name side table. Reuse one coherent BIR value/global/
  constant registry.
- Keep SSA/local pointer stores, non-integer values, and every raw/monostate
  compatibility row fail-closed. The idea-741 handoff does not establish
  whole-modern-LIR readiness.
- Store receipt is one bounded subrow; load, GEP, scalar return, remaining
  function/CFG/local-object work, and other instruction families stay later.

## Proof

- Idea 741 closure gate: fresh focused proof 2/2 and exact full suite 3033/3033.
- Current receiver boundaries before this packet remain
  `UnsupportedOrdinaryInstruction` for store/load/GEP and `InvalidVoidReturn`
  for scalar value return.
