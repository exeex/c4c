# Current Packet

Status: Active
Source Idea Path: ideas/open/744_lir_remaining_ordinary_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish the remaining ordinary authority baseline

## Just Finished

- Completed Plan Step 1 with a checked current-source authority matrix covering
  all 38 `LirInst` alternatives exactly once: 12 producerless legacy rows and
  26 active modern rows, with every field family, producer, carrier,
  allocation path, verifier state, dependency, probe, and disposition named.
- Preserved CC-STORE-1, CC-LOAD-1, CC-GEP-1, and CC-RET-1 as closed idea-741
  regression neighbors and classified their unclaimed neighboring shapes
  without reopening those contracts.
- Confirmed all four Step-2 probes have independent production first bad facts;
  the SSA-call-argument implementation should reuse the common argument
  carrier introduced by the immediate-argument seam.

## Suggested Next

- Execute Plan Step 2 only: extract and bind the four focused one-contract
  probes named by the runbook and authority matrix.

## Watchouts

- Keep CFG/terminator targets, stack/local/alloca/object ownership, body
  parameters, and opaque inline-assembly text outside generic ordinary value
  publication.
- Generic `fresh_value` result/use propagation fits scalar call, binary, cast,
  compare, select, and several aggregate/vector result edges, but PHIs need
  `LirBlockId`, call args need a non-string owned carrier, and local pointers
  need a separate object relation.
- Current production `fresh_value(ctx)` remains exactly the two selected-global
  CC-GEP-1/CC-LOAD-1 sites; do not infer authority from other `%t*` results.

## Proof

- Docs-only packet; no build or test was required and no regression log changed.
- Mechanical extraction found 38 current source alternatives and 38 unique
  matrix rows with empty `comm -3`: 12 producerless legacy + 26 active modern.
- Current-source spot checks confirmed `emit_call_with_result` uses
  `fresh_tmp`, call args lose authority through `emit_rval_id` and raw-string
  `OwnedLirTypedCallArg`, representative `LirBinOp` chains use `fresh_tmp`, and
  the only two `fresh_value` calls remain the idea-741 load/GEP neighbors.
- `git diff --check` passed for the documentation and packet-state changes.
