# Current Packet

Status: Active
Source Idea Path: ideas/open/731_inline_asm_transport_and_regalloc_contract.md
Source Plan Path: plan.md
Current Step ID: 8
Current Step Title: Converge pseudo lowering, schema, and pseudo verification

## Just Finished

- Plan Step 8 is complete. D1 now has a fail-closed disposition for every
  Canonical instruction, exact Step 7 product keys, stable-ID/new-revision
  rules, atomic derived-fact rebuilding, and no allocation authority.
- The admitted pseudo table is closed and stage-qualified. D1 owns generic
  pseudo formation, D2 alone owns shared ABI-aware call transport, D3
  atomically publishes the first `PseudoBir`, D4 owns required target
  legalization plus reviewed optional entries, D5 alone adds out-of-SSA copy
  pseudos, and E3 alone adds `Spill`/`Reload`.
- The `Pseudo` verifier profile has stable rule IDs and rejects semantic
  leftovers, machine forms, allocation facts, malformed `InlineAsm`, stale or
  mixed products, incomplete call lowering, failed direct realizability, and
  partial publication.
- D4 is an always-on realizability and full-reverification gate. Every mutation
  advances the exact revision and invalidates affected products; optional
  target optimizations cannot bury values, temporaries, or assignments outside
  ordinary BIR allocation.

## Suggested Next

- Execute Plan Step 9: converge BIR-owned out-of-SSA, including explicit
  `ParallelCopy`/`EdgeCopy`, exact predecessor-edge placement, critical-edge
  handling, stable identity/revision behavior, analysis invalidation, and full
  post-mutation Pseudo reverification before E1.

## Watchouts

- Do not begin implementation before explicit architecture acceptance.
- Preserve the closed pseudo stage intervals: D-stage publication forbids
  `ParallelCopy`, `EdgeCopy`, `Spill`, and `Reload`; Step 9 admits only the copy
  families, while capacity spill/reload remains E3-owned.
- Out-of-SSA must consume the exact fully reverified D4 revision. It cannot
  reuse pre-D4 CFG, dominance, SSA, def-use, liveness, constraint projections,
  or realizability facts without an explicit preservation proof.
- Step 9 cannot reintroduce a semantic or one-to-many target-lowering
  requirement, move phi destruction into MIR, or create hidden CFG authority.
- Keep idea 731 open when this docs-only runbook is exhausted.

## Proof

- `git diff --check && ! rg -n 'concrete register|physical register|target opcode|frame offset|ordinary allocation|hide.*alloc|allocator fallback' src/backend/bir/passes/pseudo_lowering/README.md src/backend/bir/pseudo/README.md src/backend/bir/passes/target/README.md && rg -n 'closed|admitted|revision|transaction|reverif|Pseudo|InlineAsm|reject|failure' src/backend/bir/passes/pseudo_lowering/README.md src/backend/bir/pseudo/README.md src/backend/bir/passes/target/README.md src/backend/bir/verify/README.md` — exit 0.
- The supervisor selected a docs-only structural proof that does not produce a
  test log; no `test_after.log` or other regression log was created or
  modified.
