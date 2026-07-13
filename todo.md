# Current Packet

Status: Active
Source Idea Path: ideas/open/731_inline_asm_transport_and_regalloc_contract.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Converge canonical passes P05-P07 and semantic analyses

## Just Finished

- Plan Step 6 is complete. P05-P07 now consume exact predecessor revisions in
  the fixed order, preserve every cumulative earlier property or fail, and use
  one private whole-module occurrence transaction with complete rollback.
- P05 closes target-independent memory/address/effect representation, P06
  closes layout-independent aggregate semantics without recreating earlier
  forms, and P07 freezes the sole final intrinsic candidate admitted by G01.
- Memory-effects, provenance, and call-graph results are immutable analyses with
  explicit schema/dependency keys, exact module/function revision binding,
  deterministic output, stale-handle rejection, invalidation, and atomic cache
  publication.
- Canonical verification now accepts only the exact frozen P07 lineage and full
  stage stamp at G01, checks the cumulative P01-P07 profile on that revision,
  and atomically publishes `CanonicalBir` or publishes nothing.
- All Step 6 facts remain target-independent. `InlineAsm` remains one ordinary-
  value opaque semantic node; its texts are preserved and never interpreted or
  rewritten in P05-P07, semantic analyses, or Canonical verification.

## Suggested Next

- Execute Plan Step 7 in its declared order: converge target layout,
  preparation, ABI, calls, variadics, address, runtime helpers, inline-assembly
  preparation, and allocation-constraint ownership. Bind every derived product
  to immutable `CanonicalBir` plus one exact target fingerprint without writing
  prepared or allocation facts back into Canonical BIR.

## Watchouts

- Do not begin implementation before explicit architecture acceptance.
- Keep Step 7 products external, immutable, and keyed to the complete Canonical
  stage stamp plus target fingerprint; a module revision alone is insufficient.
- Preserve the single owner split: preparation may classify and plan, while the
  allocation-constraints stage alone interprets and binds target constraint
  meaning. Neither may replace the ordinary BIR value graph.
- Do not let preparation select allocation homes or mutate Canonical BIR, and
  do not let allocation constraints become a duplicate target-layout, ABI,
  call-plan, or inline-assembly semantic owner.
- Keep idea 731 open when this docs-only runbook is exhausted.

## Proof

- `git diff --check && ! rg -n 'target opcode|physical register|ABI (location|placement)|frame offset|parse.*(asm|constraint)|constraint.*parse' src/backend/bir/passes/memory/README.md src/backend/bir/analysis/memory_effects/README.md src/backend/bir/analysis/provenance/README.md src/backend/bir/passes/aggregate/README.md src/backend/bir/passes/intrinsics/README.md src/backend/bir/analysis/call_graph/README.md && rg -n 'CanonicalBir|Canonical.*profile|P07|revision|rollback|opaque|InlineAsm' src/backend/bir/passes/memory/README.md src/backend/bir/passes/aggregate/README.md src/backend/bir/passes/intrinsics/README.md src/backend/bir/verify/README.md` — exit 0.
- The supervisor selected a docs-only structural proof that does not produce a
  test log; no `test_after.log` or other regression log was created or
  modified.
