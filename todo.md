# Current Packet

Status: Active
Source Idea Path: ideas/open/731_inline_asm_transport_and_regalloc_contract.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Converge pipeline/pass/analysis infrastructure

## Just Finished

- Plan Step 3 is complete. `pipeline/README.md` now anchors its local
  `S02`-`S09` canonical occurrence sequence beneath the root BIR README's
  normative `S00`-`S29` order instead of claiming independent order authority.
- `passes/README.md` now scopes the target-independent canonical framework to
  `P01`-`P07`/`S02`-`S09` and requires later target-aware preparation, pseudo,
  allocation, spill/reload, publication, and MIR phases to use their own
  reviewed capability and verifier contracts.
- `analysis/README.md` now defines exact epoch/module/function/digest/schema and
  later target/preparation keys, stable-ID-only result identity, transactional
  cache publication, verifier-on-commit behavior, checked preservation, and
  transitive invalidation. `LEGACY_COVERAGE.md` now assigns allocation
  liveness, abstract homes, and spill/reload to root stages `S23`-`S25` rather
  than a second MIR allocator.

## Suggested Next

- Execute Plan Step 4 in its declared order: converge
  `passes/legalize/README.md` (`P01`), `passes/scalar/README.md` (`P02`), and
  `analysis/comparison/README.md`, keeping their output target-independent and
  closed under the root `S02`-`S03` contracts.

## Watchouts

- Do not begin implementation before explicit architecture acceptance.
- Do not let a local pass or analysis document invent ordering outside the root
  README or treat an analysis dependency as a serial stage.
- Keep canonical analysis keys target-free. Target/layout/preparation keys are
  legal only in the distinct later allocation domain, first consumed at `S23`.
- Keep inline-asm text and constraint strings opaque throughout `P01` and
  `P02`; root `S18` remains the sole parsing/typing/binding owner.
- Keep idea 731 open when this docs-only runbook is exhausted.

## Proof

- `git diff --check && ! rg -n 'authority for BIR stage order|authoritative
  (global|total) order' src/backend/bir/pipeline/README.md
  src/backend/bir/passes/README.md src/backend/bir/analysis/README.md && rg -n
  'root.*README|normative.*order|revision|invalidat|transaction|verif'
  src/backend/bir/pipeline/README.md src/backend/bir/passes/README.md
  src/backend/bir/analysis/README.md` — exit 0.
- The supervisor selected a docs-only structural proof that does not produce a
  test log; no `test_after.log` or other regression log was created or
  modified.
