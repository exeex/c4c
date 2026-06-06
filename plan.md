# BIR/Prealloc Aggregate Call-Boundary Contract Audit

Status: Active
Source Idea: ideas/open/113_bir_prealloc_aggregate_call_boundary_contract_audit.md

## Purpose

Audit the aggregate call-boundary lessons from idea 112 and turn concrete
shared-contract gaps into bounded follow-up ideas.

Goal: decide which repaired aggregate-call responsibilities belong in shared
BIR/prealloc contracts versus target ABI lowering, without changing
implementation files in this analysis idea.

## Core Rule

This is an analysis-only runbook. Do not edit implementation files, weaken test
contracts, or reopen idea 112 unless the audit finds a real current regression.

## Read First

- `ideas/open/113_bir_prealloc_aggregate_call_boundary_contract_audit.md`
- Idea 112 closure notes
- Relevant commits:
  - `5054b6999 Repair local aggregate call address selection`
  - `cdfc33cac Repair AArch64 variadic aggregate call handling`
  - `1362a467b Protect AArch64 recovered stack call targets`
  - `9e78e96e7 Refresh AArch64 byval stack route contracts`
- Nearby closed ideas for historical review only: 97, 101, 102, 104, 106, 110,
  and 112

## Current Targets

- Frame-slot-backed aggregate address materialization
- Local aggregate pointer operand selection
- Outgoing stack argument area reservation and address ownership
- Byval aggregate stack routes
- Variadic aggregate `va_arg` payload and home preservation
- Prepared dump visibility for aggregate call-boundary facts

## Non-Goals

- Do not directly change `src/backend/mir/aarch64/codegen`.
- Do not implement x86 or RISC-V aggregate call lowering.
- Do not perform broad BIR/prealloc rewrites.
- Do not renumber or rewrite older closed ideas.
- Do not create duplicate follow-up ideas for already-closed and proven work.

## Working Model

Classify each repaired behavior into exactly one primary owner layer:

- shared BIR semantic fact
- shared prealloc/planning fact
- target ABI placement decision
- target codegen emission detail

When a behavior spans layers, record both sides of the boundary explicitly. For
example, the shared outgoing-stack area requirement is not the same thing as
AArch64's `x16` scratch addressing convention.

## Execution Rules

- Keep routine findings in `todo.md` until a durable output is needed.
- Create follow-up ideas only for concrete unresolved contract gaps.
- Each follow-up idea must name owner boundary, likely files or modules, proof
  route, and reject signals against testcase-overfit.
- If current coverage and closed ideas already settle a candidate, record
  "no new idea" instead of creating duplicate work.
- Preserve the full-suite clean baseline from commit
  `5daac44dc41119b9ea3d2a8c1d91e00da78f6aec` as the accepted starting point.

## Ordered Steps

### Step 1: Reconstruct Idea 112 Repair Boundaries

Goal: extract the durable boundary lessons from idea 112 and its relevant
commits.

Actions:

- Read idea 112's closure notes.
- Inspect commits `5054b6999`, `cdfc33cac`, `1362a467b`, and `9e78e96e7`.
- For each repair behavior, identify the observed failure, the repaired layer,
  and the proof route that made the repair credible.

Completion check:

- `todo.md` records a concise classification table or notes for the four
  relevant commits and the two repaired testcase families, without changing
  implementation files.

### Step 2: Classify Current Contract Ownership

Goal: map each aggregate call-boundary behavior to the current owner layer.

Actions:

- Audit current BIR, prealloc/planning, and AArch64 lowering surfaces for the
  target list in this runbook.
- Separate shared semantic facts, shared planning facts, ABI placement
  decisions, and emission details.
- Identify where current prepared dumps do or do not expose the facts needed
  for review.

Completion check:

- `todo.md` records owner-layer classifications for every current target and
  highlights only concrete unresolved gaps.

### Step 3: Compare Nearby Closed Ideas

Goal: avoid duplicating work that is already settled by closed ideas and current
tests.

Actions:

- Review closed ideas 97, 101, 102, 104, 106, 110, and 112 only for historical
  overlap with the candidate gaps.
- Mark each candidate as either covered, partially covered, or unresolved.
- Preserve the source idea's scope; do not rewrite closed ideas.

Completion check:

- `todo.md` records the overlap decision for each candidate, including
  explicit "no new idea" notes where current coverage is sufficient.

### Step 4: Create Bounded Follow-Up Ideas

Goal: materialize only the unresolved contract gaps as separate open ideas.

Actions:

- For each unresolved gap, create a focused source idea under `ideas/open/`.
- Include the owner boundary, likely files or modules, proof route, and concrete
  reviewer reject signals.
- Keep AArch64 ABI details separate from shared BIR/prealloc contracts.

Completion check:

- Each created follow-up idea is bounded, non-duplicative, and testable.
- No implementation files are changed.

### Step 5: Close Or Report Remaining Analysis State

Goal: decide whether idea 113 is complete after the audit and follow-up idea
creation.

Actions:

- Confirm all acceptance criteria from the source idea are satisfied.
- If complete, request close through the lifecycle flow.
- If incomplete, leave `todo.md` with the remaining blocker and current step
  metadata aligned to the next needed step.

Completion check:

- The active lifecycle state clearly shows either closure readiness or the
  exact remaining audit blocker.
