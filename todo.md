# Current Packet

Status: Active
Source Idea Path: ideas/open/731_inline_asm_transport_and_regalloc_contract.md
Source Plan Path: plan.md
Current Step ID: 11
Current Step Title: Converge AllocatedBir and MIR-ready publication

## Just Finished

- Plan Step 11 is complete. E4 freezes one stable E3 candidate and reruns the
  cumulative graph, Pseudo, direct-realizability, out-of-SSA, assignment, and
  explicit `Spill`/`Reload` rules in one fail-closed transaction.
- Every allocatable definition/result, fixed home, copy/call/inline-asm role,
  use, spill object, reload result, target mapping, and revision-bound product
  fingerprint must be present, legal, unique, and fresh before publication.
- Success atomically mints the owning `AllocatedBir`, a `PreparedBir` readiness
  capability, and borrowing read-only `MirReadyBirView` instances over one
  exact immutable graph revision. None copies graph storage.
- MIR is limited to same-key concrete mapping and one-to-one selection. It
  cannot change assignments, add capacity spill/reload or allocatable
  temporaries, reinterpret constraints, expand nodes, or repair a failed map.

## Suggested Next

- Execute Plan Step 12: converge cross-cutting observation and quarantine
  contracts so diagnostics, rendering, compatibility, and coverage helpers
  remain read-only and non-authoritative.

## Watchouts

- Do not begin implementation before explicit architecture acceptance.
- Step 12 must not let diagnostics, renderers, compatibility adapters, legacy
  coverage, caches, or audit views mint stage tokens, override verifier facts,
  or become alternate semantic/product authority.
- Preserve the Step 11 rule that only E4 can mint the three same-revision
  allocated/readiness/view capabilities; observational helpers may borrow but
  cannot extend lifetime, copy storage, refresh keys, or repair publication.
- Keep idea 731 open when this docs-only runbook is exhausted.

## Proof

- `git diff --check && ! rg -n 'duplicate instruction graph|second instruction graph|MIR.*ordinary.*allocat|MIR.*repair.*allocat|backend.*ordinary.*spill|copy.*allocated.*graph' src/backend/bir/allocated/README.md src/backend/bir/verify/README.md src/backend/bir/preparation/README.md src/backend/bir/pipeline/README.md src/backend/bir/README.md && rg -n 'AllocatedBir|PreparedBir|MirReadyBirView|same.*revision|read.only|assignment|Spill|Reload|fingerprint|transaction|publish|failure' src/backend/bir/allocated/README.md src/backend/bir/verify/README.md src/backend/bir/preparation/README.md src/backend/bir/pipeline/README.md src/backend/bir/README.md` — exit 0.
- The supervisor selected a docs-only structural proof that does not produce a
  test log; no `test_after.log` or other regression log was created or
  modified.
