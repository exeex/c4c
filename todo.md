# Current Packet

Status: Active
Source Idea Path: ideas/open/741_lir_structured_operand_and_terminator_identity_decomposition.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Extract or confirm focused probes

## Just Finished

- Plan Step 2 produced
  `docs/lir_structured_identity/authority_matrix.md`, exhaustively naming all
  38 `LirInst` and 6 `LirTerminator` alternatives and every direct/nested
  operand, result, immediate, symbol, label, and compatibility field.
- Each row now records current carrier authority, actual producer or explicit
  producerless legacy status, verifier reach/gap, focused coverage/gap, blocked
  idea-734 relevance, and an exact disposition. The checked smallest candidate
  family is the existing `LirValueId`/`LinkNameId`/`LirBlockId`/native-immediate
  conventions, with display text retained only for presentation/parity.

## Suggested Next

- Execute Plan Step 3: confirm whether the load/store integration cases isolate
  one identity contract each, bind the array case to its cast-first/later-GEP
  seams, and extract a minimal scalar non-void return probe if needed.

## Watchouts

- The matrix confirms that modern `LirOperand` is still text plus kind, not a
  stable identity. Compound GEP/PHI/indirect-target/terminator text fields are
  shallow or unverified, while all legacy instruction stubs are ignored by the
  verifier and have no ordinary HIR producer.
- Preserve the cast-first/later-GEP fact for `defined_global_array.c`; do not
  infer focused coverage for matrix rows marked `C-gap—Step 3 candidate`.

## Proof

- Documentation-only proof: `git diff --check` plus a mechanical comparison of
  the current variant lists against the matrix. The inventory is 38/38
  `LirInst` alternatives and 6/6 `LirTerminator` alternatives with no missing
  direct or nested field names.
- No build, tests, schema edits, producer edits, verifier edits, consumer edits,
  or proof-log changes were required for Plan Step 2.
