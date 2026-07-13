# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 4.5.3
Current Step Title: Receive function linkage and elision facts

## Just Finished

- Completed Plan Step 4.5.2 inventory and selected only native
  `LirFunction::is_internal` plus `can_elide_if_unreferenced` for the next
  receiver packet; both are producer-populated and currently dropped by BIR.
- Confirmed CFG receipt remains blocked by raw branch/switch targets and a
  producerless indirect-branch ID form despite structured block/entry facts.
- Confirmed stack/local-object receipt cannot yet unlock a production-success
  path because allocation/result/ownership bindings remain raw; body parameter
  uses also remain raw.

## Suggested Next

- Execute Plan Step 4.5.3 by receiving exactly the two structured function
  linkage/elision booleans through core, builder/view, verifier, importer, and
  declaration/definition merge proof.

## Watchouts

- Do not infer either boolean from function names, signature rendering, body
  presence, source order, or testcase identity; import native LIR facts only.
- Preserve producer-valid declaration/definition merge behavior and reject
  contradictory duplicates transactionally; do not silently OR or clear
  metadata.
- Keep CFG, block targets/edges, stack slots, allocas, local objects, body
  parameter binding, and lifetime state outside this packet.
- Keep `long` and `unsigned long` fail-closed pending inactive idea 743; do not
  change I686 width semantics inside idea 734.
- Signature `ParameterDef` ordinals are BIR storage order only. Body parameter
  use still lacks native source identity and must not be reconstructed from
  names, raw operands, signature text, or ABI position.
- Production `param_slot.c` now passes signature receipt and rejects later at
  `UnsupportedAllocaInstructions`; pointer parameters still reject at stable
  `UnsupportedFunctionParameters`.

## Proof

- Step 4.5.2 was a read-only authority inventory; no code, test, or regression
  proof was generated for the runbook transition.
