# Current Packet

Status: Active
Source Idea Path: ideas/open/744_lir_remaining_ordinary_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Extract and bind four focused one-contract probes

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

- Execute Plan Step 2: extract and bind the four focused one-contract probes
  named by the runbook and authority matrix.

## Watchouts

- Keep one primary authority contract per focused probe; do not combine the
  four first bad facts into a monolithic case.
- Observe each production boundary and bind its exact carrier transition plus
  verifier obligations before any producer implementation.
- The SSA-argument probe must reuse the argument carrier established by the
  immediate-argument seam; do not create a duplicate SSA-only carrier.
- Keep CFG/terminator targets, stack/local/alloca/object ownership, and body
  parameter identity outside Step 2.

## Proof

- Docs-only packet; no build or test was required and no regression log changed.
- Mechanical extraction found 38 current source alternatives and 38 unique
  matrix rows with empty `comm -3`: 12 producerless legacy + 26 active modern.
- Current-source spot checks confirmed `emit_call_with_result` uses
  `fresh_tmp`, call args lose authority through `emit_rval_id` and raw-string
  `OwnedLirTypedCallArg`, representative `LirBinOp` chains use `fresh_tmp`, and
  the only two `fresh_value` calls remain the idea-741 load/GEP neighbors.
- `git diff --check` passed for the documentation and packet-state changes.
