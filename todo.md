# Current Packet

Status: Active
Source Idea Path: ideas/open/731_inline_asm_transport_and_regalloc_contract.md
Source Plan Path: plan.md
Current Step ID: 10
Current Step Title: Converge shared liveness and allocation

## Just Finished

- Plan Step 10 is complete. E1 publishes immutable liveness/interference facts
  only for the exact fully reverified D5 or E3-retry revision and remains
  separate from allocation, eviction, and spill policy.
- E2 is the sole shared RV64, AArch64, and x86 pseudo-home allocator. Reviewed
  layout data supplies target differences; one legality relation covers
  groups, ties, aliases, early-clobbers, calls, copy bundles, eviction, and
  retry.
- E3 alone owns abstract spill objects and explicit `Spill`/`Reload` placement.
  Each rewrite advances and fully reverifies the candidate, invalidates prior
  E1/E2 state, and retries from fresh facts under a finite monotone budget.
- The candidate gate fails closed unless every allocatable identity has a
  legal abstract home or verified explicit spill state and every `Reload`
  result is assigned. Downstream MIR cannot repair ordinary allocation.

## Suggested Next

- Execute Plan Step 11: converge the `AllocatedBir` publication profile and
  prove `MirReadyBirView` is a read-only view of that same immutable revision.

## Watchouts

- Do not begin implementation before explicit architecture acceptance.
- Step 11 must define the final allocated verifier rules without weakening the
  E1/E2/E3 candidate gate or introducing a second instruction graph.
- Preserve the strict distinction between E3 retry-candidate reverification
  and final `AllocatedBir` publication; the former cannot mint the latter.
- Keep final machine naming, frame layout, and instruction encoding downstream
  while requiring MIR to consume the verified abstract assignments and spill
  transitions without an ordinary allocation repair escape hatch.
- Keep idea 731 open when this docs-only runbook is exhausted.

## Proof

- `git diff --check && ! rg -n 'concrete register|physical register|frame offset|target opcode|MIR.*allocat|allocator.*MIR|backend.*ordinary.*spill' src/backend/bir/analysis/liveness/README.md src/backend/bir/regalloc/README.md src/backend/bir/regalloc/constraints/README.md src/backend/bir/regalloc/spill_reload/README.md && rg -n 'E1|E2|E3|revision|interference|tie|clobber|group|evict|Spill|Reload|retry|terminat|failure' src/backend/bir/analysis/liveness/README.md src/backend/bir/regalloc/README.md src/backend/bir/regalloc/constraints/README.md src/backend/bir/regalloc/spill_reload/README.md` — exit 0.
- The supervisor selected a docs-only structural proof that does not produce a
  test log; no `test_after.log` or other regression log was created or
  modified.
