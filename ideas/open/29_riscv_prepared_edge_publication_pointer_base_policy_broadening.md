# RISC-V Prepared Edge Publication Pointer-Base Policy Broadening

## Goal

Broaden RISC-V prepared edge-publication support for
`PointerBasePlusOffset -> Register` beyond the focused register-base,
signed-12-bit `addi` policy closed by idea 26.

## Why This Exists

Idea 26 intentionally proved the narrow semantic route first: shared
`edge_publications` authority, a pointer base resolved through prepared
value-home lookup to a register home, and a present delta that fits RISC-V
`addi`. That leaves real pointer-base materialization policy questions that
should be handled as a separate initiative instead of being silently absorbed
into the closed scope.

## In Scope

- Decide whether and how RISC-V should support pointer-base homes whose base
  resolves to non-register homes.
- Define large-delta materialization for offsets that do not fit signed
  12-bit `addi`, including any scratch-register or multi-instruction policy.
- Preserve shared prepared `edge_publications` as the only semantic authority
  for the edge move.
- Keep RISC-V address materialization, instruction selection, and scratch
  policy target-local.
- Add focused positive and negative coverage for each newly supported
  pointer-base policy.

## Out Of Scope

- Source-to-`StackSlot` destination moves; those belong to the stack
  destination idea.
- Stack-source policy broadening; that belongs to the stack-source policy
  idea.
- Local rediscovery of edge-copy facts from RISC-V predecessor or successor
  block shape.
- Treating missing base names, unresolved base names, or absent deltas as
  valid moves without a separate source-model decision.
- Broad pointer analysis, memory-layout rewrites, or target-neutral prepare
  rewrites.

## Acceptance Criteria

- Any newly supported pointer-base form is driven by shared
  `edge_publications` and prepared value-home lookup authority.
- Large-delta or non-register-base support uses an explicit semantic RISC-V
  materialization rule instead of fixture-shaped matching.
- Unsupported pointer-base forms remain explicit and fail closed.
- Existing register-base signed-12-bit behavior remains covered.
- Validation covers focused RISC-V pointer-base publication tests, relevant
  shared prepared lookup tests, and an appropriate backend bucket.

## Reviewer Reject Signals

- The patch matches named tests, edge labels, fixture offsets, value ids, or
  hard-coded large constants instead of implementing a general pointer-base
  policy.
- The patch scans predecessor or successor blocks or builds a RISC-V-local
  edge-publication table instead of consuming shared `edge_publications`.
- The patch claims support for missing base names, unresolved bases, or absent
  deltas without a source-model change and explicit tests.
- The patch folds stack destination or stack-source broadening into this
  pointer-base policy idea.
- The patch downgrades expectations or marks already-supported pointer-base
  register-base behavior unsupported to claim progress.
- The patch is only a helper rename, expectation rewrite, or classification
  change while retaining the same pointer-base fail-closed behavior.
