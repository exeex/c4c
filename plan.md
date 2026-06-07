# AArch64 Comparison Fused-Compare Publication Contract

Status: Active
Source Idea: ideas/open/117_aarch64_comparison_fused_compare_publication_contract.md

## Purpose

Make fused-compare and materialized-compare publication facts explicit enough
that `comparison.cpp` consumes shared prepared authority instead of owning
current-block publication lookup or branch-fusion source routing.

## Goal

Move or expose target-neutral prepared comparison facts through shared query or
prepared-record owners while keeping AArch64 condition-code spelling, compare
opcode choice, branch emission, and operand emission local to AArch64 lowering.

## Core Rule

Do not claim progress through renamed comparison-local routing, expectation
downgrades, or one-case branch-fusion fixes. Every code-changing step must
either consume a shared prepared fact, expose a missing shared fact with proof,
or document with concrete evidence why a remaining decision is genuinely
target-local.

## Read First

- `ideas/open/117_aarch64_comparison_fused_compare_publication_contract.md`
- `src/backend/mir/aarch64/codegen/comparison.cpp`
- Shared prepared/BIR prealloc query owners under `src/backend/mir/`
- Existing backend dump or route tests covering fused compare, materialized
  compare conditions, branch conditions, and current-block entry publications
- Idea 115 Step 2 notes in current `todo.md` history if needed to recover the
  helper names that motivated this follow-up

## Current Scope

Primary comparison helper families:

- fused-compare prepared operand producer lookup
- materialized compare join-target lookup
- current-block entry publication collection
- prepared publication register lookup

Candidate shared fact owners:

- prepared/BIR prealloc query code that can expose branch-condition,
  fused-compare operand, materialized compare, current-block entry
  publication, or join-target facts

Allowed AArch64 consumers:

- narrow call-site updates in AArch64 comparison lowering needed to consume a
  new shared query or clarified prepared fact

## Non-Goals

- Do not reopen idea 53's old same-block cast/load scan, constant producer
  scan, or materialized condition hook matching repair unless current code has
  regressed to that failure mode.
- Do not change condition-code spelling, compare opcode selection, branch
  emission, `cset`/`csel`/`cbnz` fallback shape, register printing, or scratch
  register policy except where a call site must receive prepared facts.
- Do not fold ALU, dispatch value materialization, or memory publication
  cleanup into this plan.
- Do not perform mechanical `comparison.cpp` file splitting or line-count
  cleanup before the shared contract is reviewable.
- Do not move target-local AArch64 compare or branch spelling into shared
  prepared code.

## Working Model

- Shared prepared/BIR prealloc code should own target-neutral facts about
  fused-compare operands, materialized compare join targets, branch-condition
  sources, and current-block entry publications.
- AArch64 comparison lowering should own machine instruction selection,
  condition-code spelling, operand emission, and target-specific fallback
  shape after those prepared facts are known.
- Existing helper names are investigation anchors, not proof that the helper's
  authority belongs in `comparison.cpp`.

## Execution Rules

- Start by characterizing the named comparison helper call graph before moving
  code.
- Prefer adding or tightening dump/test visibility for prepared facts before
  removing comparison-local ownership.
- Keep routine progress and proof in `todo.md`; do not edit the source idea
  unless durable source intent changes.
- Reject same-block producer rescans, raw terminator recovery, BIR-name
  matching, or named fused-compare shortcuts as permanent authority.
- Keep AArch64 branch emission and condition-code spelling out of shared
  prepared owners.
- Proof must cover both fused-compare and materialized/current-block
  publication neighbors before closure.

## Step 1: Characterize Comparison Publication Contract Residue

Goal: Build a current map of which `comparison.cpp` helpers own target-neutral
prepared publication facts and which already consume shared prepared authority.

Primary targets:

- `src/backend/mir/aarch64/codegen/comparison.cpp`
- shared prepared/BIR prealloc query owners under `src/backend/mir/`
- existing dump or route tests near fused compare, materialized compare, branch
  conditions, and current-block entry publications

Actions:

- Inspect helper call graph around
  `find_prepared_materialized_compare_join_targets`,
  `find_prepared_fused_compare_operand_producer_facts`,
  `collect_prepared_current_block_entry_publications`, and
  `prepared_current_block_entry_publication_register`.
- Identify which facts are target-neutral prepared facts and which decisions
  are genuinely AArch64 emission or fallback details.
- Identify existing shared prepared queries already available to these helpers.
- Identify missing or too-private shared facts that force `comparison.cpp` to
  shape routing locally.
- Recommend the narrow proof commands and tests that cover at least one
  fused-compare path and one materialized/current-block publication path.

Completion check:

- `todo.md` contains a helper-to-contract map, candidate shared facts,
  target-local keep-local notes, and supervisor-ready proof recommendations for
  the first implementation packet.

## Step 2: Add Or Tighten Prepared Fact Visibility

Goal: Make fused-compare and materialized/current-block publication facts
reviewable before changing ownership.

Primary targets:

- existing backend dump or route tests for fused compare
- existing backend dump or route tests for materialized compare conditions,
  branch conditions, and current-block entry publications
- shared prepared/BIR prealloc dump surfaces if needed

Actions:

- Add or tighten focused dump/test visibility for the prepared facts selected
  in Step 1.
- Ensure coverage includes fused-compare operand producer facts and at least
  one materialized compare or current-block entry publication neighbor.
- Avoid expectation rewrites that weaken supported behavior or hide branch
  fusion.
- Record any no-op visibility decision in `todo.md` with concrete existing
  evidence.

Completion check:

- Reviewable proof exists for the prepared facts that later steps will migrate
  or keep local, and no supported-path expectation was weakened.

## Step 3: Expose Fused-Compare Operand Producer Facts

Goal: Move or expose target-neutral fused-compare operand producer facts so
`comparison.cpp` consumes shared prepared authority instead of owning source
routing.

Primary targets:

- fused-compare operand producer helpers in `comparison.cpp`
- shared prepared/BIR prealloc query owners that can expose fused-compare
  operand or branch-condition source facts

Actions:

- Add or clarify shared query APIs or prepared records for target-neutral
  fused-compare operand producer facts where Step 1 proves comparison-local
  ownership.
- Update AArch64 comparison lowering to consume the shared prepared facts.
- Keep condition-code spelling, compare opcode selection, branch emission, and
  operand emission local.
- Prove the migrated route with the focused fused-compare coverage from Step 2.

Completion check:

- Fused-compare lowering no longer owns target-neutral operand producer routing
  for the migrated helper family, and proof shows the shared fact drives the
  branch-fusion path.

## Step 4: Expose Materialized Compare And Current-Block Publication Facts

Goal: Move or expose materialized compare join-target and current-block entry
publication facts so `comparison.cpp` consumes shared prepared authority or
documents any truly target-local residue.

Primary targets:

- materialized compare join-target helpers in `comparison.cpp`
- current-block entry publication collection and prepared publication register
  helpers in `comparison.cpp`
- shared prepared/BIR prealloc query owners that can expose join-target,
  current-block entry publication, or prepared publication register facts

Actions:

- Add or clarify shared query APIs or prepared records for materialized compare
  join targets and current-block entry publication facts where Step 1 proves
  they are target-neutral.
- Update AArch64 comparison lowering to consume those prepared facts.
- Document keep-local decisions with concrete AArch64 emission or
  machine-register evidence, not file-size inference.
- Prove the migrated or documented route with materialized/current-block
  publication coverage from Step 2.

Completion check:

- Materialized compare and current-block publication lowering no longer own
  target-neutral prepared lookup for the migrated helper family, or remaining
  local decisions are documented with concrete evidence and tests.

## Step 5: Final Boundary Audit And Closure Package

Goal: Produce the validation package needed for supervisor review and possible
source-idea closure.

Primary targets:

- `src/backend/mir/aarch64/codegen/comparison.cpp`
- shared prepared/BIR prealloc query owners touched by earlier steps
- `todo.md`
- `test_before.log`
- `test_after.log`

Actions:

- Re-audit `comparison.cpp` after Steps 3 and 4 to confirm it owns only target
  compare, branch, condition-code, and operand emission after prepared facts
  are known.
- Summarize migrated shared facts, remaining keep-local decisions, proof
  coverage, and any deferred follow-up in `todo.md`.
- Run the supervisor-selected backend/AArch64 subset covering comparison,
  branch fusion, materialized compare conditions, and prepared publication
  routes.
- Escalate validation if shared query semantics changed beyond AArch64
  comparison consumers.
- Confirm no supported-path expectations were weakened and no route relies on
  same-block rescans, BIR-name matching, raw terminator recovery, or named-case
  shortcuts.

Completion check:

- The supervisor has a concise closure package showing that the source idea's
  acceptance criteria are satisfied or listing exact blockers that remain.
