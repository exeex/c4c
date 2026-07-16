# LIR-To-New-BIR Final Coverage Convergence Runbook

Status: Active
Source Idea: ideas/open/797_lir_to_new_bir_final_coverage_convergence.md

## Purpose

Converge the accepted LIR-to-new-BIR authority work into a final no-omission
coverage, dispatcher, proof, and documentation route after the ordered
producer/evidence/receiver prerequisites have accepted dispositions.

## Goal

Prove every valid current-LIR semantic fact has an explicit typed disposition
or accepted no-change/evidence-backed disposition, then close any remaining
dispatcher, verifier, transactional-proof, and documentation gaps.

## Core Rule

Do not publish missing producer authority, invent receiver rows, recover
authority from text, weaken verifier contracts, or claim convergence from a
green subset. If the matrix finds a missing first-owner handoff or missing 734
receipt, stop and route that exact prerequisite as a separate lifecycle owner.

## Read First

- `ideas/open/797_lir_to_new_bir_final_coverage_convergence.md`
- `ideas/open/734_lir_to_new_bir_container_completeness.md`
- `ideas/closed/813_lir_string_semantic_authority_completion_umbrella.md`
- `ideas/closed/847_lir_universal_model_string_escape_hatch_deletion.md`
- `ideas/closed/866_lir_remaining_authority_owner_triage.md`
- `docs/lir_string_semantic_authority_completion/handoff_to_734_and_797.md`
- `docs/lir_string_semantic_authority_completion/row_to_owner_map.md`
- `docs/lir_string_semantic_authority_completion/closure_trace.md`
- `docs/lir_remaining_authority_owner_triage/ordering_and_closure.md`
- `docs/lir_remaining_authority_owner_triage/classification.md`

## Current Scope

- Build the terminal per-row matrix from accepted 734 receipts, closed
  producer/type-model handoffs, closed evidence routes, and closed deletion
  evidence.
- Validate that 848, 849, 850, 813, and 847 dispositions are represented
  without fabricating direct 734 receiver work.
- Close concrete dispatcher, verifier, transactional, and documentation gaps
  only when the matrix shows the authority row already has an accepted owner
  disposition.

## Non-Goals

- Do not implement new producer/schema/verifier authority for unowned rows.
- Do not add new Raw-BIR receiver rows unless an accepted handoff already names
  the exact typed row and lifecycle routing returns to 734.
- Do not absorb open 795, 796, 821, or 822 routes into terminal convergence.
- Do not change expectations, classify unsupported behavior as supported, or
  use testcase-shaped shortcuts.

## Execution Rules

- Treat closed 813 and 847 as terminal evidence inputs, not as proof that 797
  is already complete.
- Treat 734 as paused unless the matrix identifies an exact accepted handoff
  that requires a matching Raw-BIR receipt before 797 can continue.
- Preserve fail-closed behavior for omitted, malformed, text-derived,
  `monostate`, compatibility-mirror, or presentation-derived facts.
- Keep code changes narrow and step-scoped. Each code-bearing step needs a
  fresh build plus focused proof; escalate to broader/full proof before source
  completion.

## Ordered Steps

### Step 1: Build the terminal disposition matrix

Goal: assemble the authoritative row-by-row input for convergence.

Actions:
- Inspect the 734 accepted receiver records and the closed ordered successors
  from 866, including 848, 849, 850, 813, and 847.
- Create or update the terminal matrix documentation used by 797 so every
  current-LIR semantic family has an explicit owner disposition, receiver
  disposition, and proof status.
- Mark rows as accepted receipt, accepted no-change/evidence disposition, or
  blocked prerequisite. Do not silently omit rows.

Completion check:
- The matrix names every valid current-LIR family relevant to 797 and clearly
  separates accepted terminal inputs from rows that must leave 797 as a
  separately scoped blocker.

### Step 2: Repair only matrix-proven dispatcher and verifier gaps

Goal: make dispatcher and reachable verifier behavior match the accepted
disposition matrix.

Actions:
- Compare Raw-BIR importer dispatch, typed destination containers, and verifier
  paths against the matrix.
- Repair only gaps for rows that already have accepted typed receiver or
  evidence-backed no-change dispositions.
- Add neighboring positive and negative coverage for any repaired dispatcher or
  verifier path.

Completion check:
- Focused tests prove the repaired dispatcher/verifier paths, and malformed or
  unauthorized authority still fails closed.

### Step 3: Prove whole-module transactional behavior

Goal: prove convergence does not depend on partial imports or green narrow
  subsets.

Actions:
- Select the narrow transactional proof that exercises complete import failure
  and rollback behavior for the accepted disposition set.
- Run a fresh build, focused transactional tests, and the broader LIR-to-BIR
  backend coverage needed for the changed surfaces.
- If the proof reveals a missing first-owner handoff or missing 734 receipt,
  stop and route that exact prerequisite outside 797.

Completion check:
- Transactional proof is accepted by the supervisor, and no failure is hidden
  by filtering, expectation downgrade, or unsupported classification.

### Step 4: Align documentation and final proof record

Goal: make documentation, code, and proof tell the same terminal convergence
story.

Actions:
- Update the relevant docs so the final matrix, dispatcher/verifier behavior,
  and whole-module proof record agree.
- Run `git diff --check`.
- Run the supervisor-selected focused and broader/full validation gate for
  final 797 acceptance.

Completion check:
- Documentation matches the accepted code route and proof record; no source
  completion criterion from 797 remains unmet.
