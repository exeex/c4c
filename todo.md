# Current Packet

Status: Active
Source Idea Path: ideas/open/773_lir_gep_direct_label_address_constant_contract.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Specify and verify the typed direct-label-address GEP base

## Just Finished

- No blocker execution packet has completed. The outgoing 772 mapping and
  return point are preserved in its source resumption record.

## Suggested Next

- Executor: implement only Step 1's exact verifier contract transition and
  nearby positive/malformed verifier coverage; do not edit the 772
  `emit_indexed_gep` forwarding seam.

## Watchouts

- Admit only a verified current-function direct label-address
  `DirectConstant(LirValueId)`. Do not accept arbitrary direct constants,
  recover text, fabricate SSA, or widen generic GEP authority.

## Proof

- Before acceptance, run a fresh build and the narrow verifier/printer/lowering
  proof selected by the supervisor. The parent `pr70460` mapping baseline is
  retained separately and is not proof of this blocker.
