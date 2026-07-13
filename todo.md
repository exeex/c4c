# Current Packet

Status: Active
Source Idea Path: ideas/open/731_inline_asm_transport_and_regalloc_contract.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Converge canonical passes P01-P02 and scalar analysis

## Just Finished

- Plan Step 4 is complete. `passes/legalize/README.md` now closes P01 over the
  exact verified `RawBir` input and `TypesLegal` output checkpoints, gives every
  admitted Raw form one disposition, and fails atomically for unsupported or
  ownerless semantics.
- `passes/scalar/README.md` now closes P02 as the idempotent function pass from
  `TypesLegal` to `ScalarsCanonical`, including its portable rewrite authority,
  complete special-operation dispositions, and all-or-nothing failure rules.
- `analysis/comparison/README.md` now defines one immutable function-scoped,
  revision-bound canonical analysis with a closed v1 descriptor, stable-ID
  facts, exact invalidation, stale-handle rejection, and no mutation authority.
- Both passes preserve inline-assembly template and constraint payload
  opaquely and exclude post-canonical lowering, placement, allocation, and
  identity decisions.
- `pipeline/README.md` now agrees with those contracts: P01/P02 preserve the
  opaque payload until root stage `S18`, the Raw inventory assigns its first
  interpretation there, and legacy out-of-SSA maps to root stage `D5` before
  `E1` allocation liveness.

## Suggested Next

- Execute Plan Step 5 in its declared order: converge
  `passes/cfg/README.md` (`P03`), `analysis/cfg/README.md`,
  `analysis/dominance/README.md`, `passes/ssa/README.md` (`P04`), and
  `analysis/publication/README.md`, keeping terminators as sole edge authority
  and deferring out-of-SSA to root stage `D5` before `E1` allocation liveness.

## Watchouts

- Do not begin implementation before explicit architecture acceptance.
- Do not let a local pass or analysis document invent ordering outside the root
  README or treat an analysis dependency as a serial stage.
- Keep CFG analysis immutable and revision-bound; CFG mutation belongs only to
  P03. Dominance consumes the exact resulting edge revision rather than
  becoming another stage.
- Keep idea 731 open when this docs-only runbook is exhausted.

## Proof

- `git diff --check && ! rg -n 'structured target-independent asm constraint grammar|unresolved .*constraint encoding|external MIR construction/out-of-SSA|preparation owns.*constraint' src/backend/bir/pipeline/README.md && ! rg -n 'legalize.*(parse|normaliz).*(constraint|asm)|constraint.*(parse|normaliz).*legalize|physical register|target opcode|ABI placement' src/backend/bir/passes/legalize/README.md src/backend/bir/passes/scalar/README.md src/backend/bir/analysis/comparison/README.md` — exit 0.
- `git diff --check && rg -n 'root stage `D5`|before `E1` allocation liveness' src/backend/bir/pipeline/README.md todo.md && ! rg -n 'S23.*out-of-SSA|out-of-SSA.*S23' src/backend/bir/pipeline/README.md todo.md` — exit 0.
- The supervisor selected a docs-only structural proof that does not produce a
  test log; no `test_after.log` or other regression log was created or
  modified.
