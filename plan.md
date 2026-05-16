# AArch64 Comparison Shard Redistribution Runbook

Status: Active
Source Idea: ideas/open/256_aarch64_comparison_markdown_shard_implementation_redistribution.md

## Purpose

Reconcile the AArch64 `comparison.md` shard into compiled owners
`src/backend/mir/aarch64/codegen/comparison.cpp` and
`src/backend/mir/aarch64/codegen/comparison.hpp`.

Goal: comparison-specific compare, condition, and branch-control construction,
lowering, helper, and spelling behavior should live in the comparison owner
while preserving existing backend behavior.

Core Rule: move ownership, do not expand comparison semantics or change test
contracts to make the redistribution appear complete.

## Read First

- `ideas/open/256_aarch64_comparison_markdown_shard_implementation_redistribution.md`
- `src/backend/mir/aarch64/codegen/comparison.md`
- `src/backend/mir/aarch64/codegen/comparison.cpp`
- `src/backend/mir/aarch64/codegen/comparison.hpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`

## Current Scope

- Own only the comparison shard redistribution.
- Keep `comparison.cpp`/`comparison.hpp` as the compiled comparison-family
  home.
- Move comparison-specific record construction, branch-control lowering
  helpers, condition helpers, and spelling support out of broad owners where
  that can be done behavior-preservingly.
- Delete `src/backend/mir/aarch64/codegen/comparison.md` only after its durable
  content has been reconciled into compiled owners or proven obsolete by the
  implemented comparison model.

## Non-Goals

- Do not redistribute other AArch64 markdown shards.
- Do not redistribute `mod.md`.
- Do not redesign `instruction.hpp` or the instruction type hierarchy solely
  for this shard.
- Do not move broad instruction, dispatch, or printer responsibilities into
  `comparison.cpp`.
- Do not add named-case shortcuts, testcase-shaped matching, or expectation
  rewrites.
- Do not broaden condition, compare, branch, or select semantics except where
  strictly required for behavior-preserving relocation.

## Working Model

`comparison.cpp` already owns prepared branch terminator lowering and block
successor construction. The remaining route is to identify comparison-family
logic still centralized in `instruction.cpp`, `dispatch.cpp`, or
`machine_printer.cpp`, then move only that behavior into the comparison owner
behind narrow declarations. `instruction.cpp` should retain family-neutral
record infrastructure and behavior shared by multiple instruction families.

## Execution Rules

- Preserve behavior at each step; favor small compile-proven moves.
- Keep declarations narrow and local to comparison ownership.
- If a helper is shared by non-comparison families, leave it in the broader
  owner or introduce only the minimum neutral interface needed.
- Keep `dispatch.cpp` as routing glue.
- Keep `machine_printer.cpp` as generic printer orchestration unless a
  comparison-specific spelling helper can be cleanly owned by the comparison
  shard.
- Treat `comparison.md` as source material during execution and delete it only
  in the final reconciliation step.
- For code-changing steps, prove with build or focused backend tests chosen by
  the supervisor.

## Ordered Steps

### Step 1: Inventory Comparison Ownership

Goal: identify the exact comparison-family code that still lives outside
`comparison.cpp`/`comparison.hpp`.

Primary targets:

- `src/backend/mir/aarch64/codegen/comparison.md`
- `src/backend/mir/aarch64/codegen/comparison.cpp`
- `src/backend/mir/aarch64/codegen/comparison.hpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`

Actions:

- Inspect compare, condition, branch-control, and selected-node construction
  functions in the current code.
- Separate comparison-family behavior from family-neutral record plumbing.
- Record in `todo.md` which functions are safe first-move candidates and which
  must remain in neutral owners.

Completion check:

- `todo.md` names a narrow first implementation packet and the proof command
  the supervisor delegated for it.

### Step 2: Move Branch And Condition Record Construction

Goal: relocate comparison-specific branch and condition record construction
from broad instruction owners into the comparison owner.

Primary targets:

- `src/backend/mir/aarch64/codegen/comparison.cpp`
- `src/backend/mir/aarch64/codegen/comparison.hpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`

Actions:

- Move branch-control record builders and compare-branch condition helpers only
  when they are specific to comparison/branch-control selected nodes.
- Keep shared operand, status, and record utility helpers in neutral owners.
- Preserve current diagnostics and fail-closed behavior for unsupported or
  inconsistent prepared facts.
- Build after each narrow move.

Completion check:

- Branch and condition selected-node construction still compiles and produces
  the same selected/deferred status behavior.

### Step 3: Route Dispatch Through The Comparison Owner

Goal: keep `dispatch.cpp` as routing and ensure comparison-family bodies are
owned by comparison helpers.

Primary targets:

- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/comparison.cpp`
- `src/backend/mir/aarch64/codegen/comparison.hpp`

Actions:

- Inspect branch, conditional branch, compare branch, and materialized-bool
  routing.
- Replace any embedded comparison-family body in `dispatch.cpp` with calls
  into comparison-owned functions.
- Do not move unrelated dispatch mechanics.

Completion check:

- `dispatch.cpp` contains routing glue for comparison behavior, not
  comparison-family implementation bodies.

### Step 4: Reconcile Comparison Spelling Ownership

Goal: move comparison-specific spelling helpers out of generic printer code
only when the ownership boundary is clear.

Primary targets:

- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.hpp`
- `src/backend/mir/aarch64/codegen/comparison.cpp`
- `src/backend/mir/aarch64/codegen/comparison.hpp`

Actions:

- Identify spelling for compare, condition, conditional branch, compare branch,
  and related condition-code forms.
- Move only comparison-specific spelling decisions into comparison-owned helper
  APIs if the printer can remain generic.
- Leave broad assembly emission mechanics in `machine_printer.cpp`.

Completion check:

- Generic printer orchestration remains generic and comparison-specific
  spelling bodies are no longer stranded there when they can be owned through
  comparison helpers.

### Step 5: Reconcile `comparison.md` And Validate

Goal: close the shard redistribution without leaving the markdown shard as a
parallel source of truth.

Primary targets:

- `src/backend/mir/aarch64/codegen/comparison.md`
- `src/backend/mir/aarch64/codegen/comparison.cpp`
- `src/backend/mir/aarch64/codegen/comparison.hpp`
- focused backend tests or build targets selected by the supervisor

Actions:

- Check each durable behavior note from `comparison.md` against compiled code.
- Delete `comparison.md` after valid durable content is reconciled or already
  represented by current compiled behavior.
- Run focused backend proof, then request broader validation if multiple
  narrow packets have landed or the touched surface spans instruction,
  dispatch, and printer ownership.

Completion check:

- `comparison.md` is deleted, compiled comparison owners contain the valid
  shard behavior, and proof shows behavior was preserved without unrelated
  feature expansion.

## Acceptance Gate

- `comparison.cpp` and `comparison.hpp` own the valid comparison shard
  behavior.
- `comparison.md` is gone before closure.
- `instruction.cpp` retains only family-neutral instruction core.
- `dispatch.cpp` routes to comparison-owned behavior.
- `machine_printer.cpp` does not retain comparison-specific spelling bodies
  that can be owned through comparison helpers.
- Focused backend proof is green.
- No test expectations are downgraded or weakened.
