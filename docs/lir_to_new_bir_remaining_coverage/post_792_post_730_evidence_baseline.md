# Post-792 / Post-7.30 Evidence Baseline

Status: current baseline for 793 Step 1

## Authority used

- `ideas/closed/792_lir_next_local_operation_receiver_handoff.md`, closed by
  `ecccb195f`, confines 792's completed capability to one VLA
  `LirStackSaveOp` saved-stack-pointer authority handoff.
- `docs/lir_local_operation_authority/handoff_to_734.md` is the structured
  handoff: native current-function result `LirValueId`, matching pointer
  definition, local object and owner, pointer/pointee types, and liveness.
  It rejects presentation recovery.
- `ideas/open/734_lir_to_new_bir_container_completeness.md` records accepted
  Step 7.30 receiver commit `2323afb91`, after 792 producer/handoff commits
  `900a42bfd` and `89f4d7f85`.

This is post-792/post-7.30 evidence. It supersedes an older active-792
snapshot and makes no claim that 792 remains active.

## Accepted bounded work

734 Step 7.30 accepted exactly the selected VLA stack-save authority as a
typed Raw-BIR node with transactional importer dispatch, reachable verifier,
and nearby backend coverage. Its recorded acceptance proof is a fresh build,
`^backend_` 5/5, and a matching non-decreasing canonical guard from 5/5 to
5/5. Steps 1 through 7.30 are historical accepted 734 work; this baseline does
not repeat or extend them.

## Closed and fail-closed boundary

Closed 792 authorizes only the VLA stack-save saved-stack-pointer handoff and
the Step 7.30 receipt of that handoff. It does **not** authorize stack restore,
dynamic VLA allocation, VLA GEP, another/nonselected stack save, other local
load/store/GEP or local-temporary rows, or broader local conversion. Those rows
remain fail closed until separately selected, published, verified, and handed
off through their first owning layer.

The parent 734 source also records the following as still fail closed: remaining
local/VLA rows; memory and `va_list`; aggregate/vector; body-parameter;
module/type/global/metadata; instruction/terminator; and inline-assembly
families. The no-omission matrix, per-row typed
authority/destination/importer/verifier/proof dispositions, complete dispatcher,
whole-module transactional proof, and documentation convergence remain unmet.

## Routing consequence for 793

No receiver work follows directly from this baseline. 793 must next classify
each remaining row by first owner and dependency before any bounded successor
is proposed. Authority must come from the structured sources above, never from
names, rendered operands, printer output, LLVM text, or testcase identity.
