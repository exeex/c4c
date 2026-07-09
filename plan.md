# Stack-Destination Fan-In Authority Decomposition Runbook

Status: Active
Source Idea: ideas/open/655_stack_destination_fan_in_authority_decomposition.md
Supersedes Active Route: ideas/open/647_ordered_or_exclusive_stack_destination_fan_in_authority.md Step 2

## Purpose

Replace the blocked idea 647 Step 2 family search with focused decomposition
probes for non-637 stack-destination register fan-in authority.

## Goal

Split the residual family into owned authority seams, define focused probes,
and select the first legal implementation follow-up only after producer
evidence exists for one seam.

## Core Rule

Do not implement destination fan-in authority or RV64 materialization from GCC
torture testcase identity, move order, source freshness, final assembly,
diagnostic wording, or unrelated select/join facts. Progress requires a named
producer authority fact at the consumer point.

## Read First

- `ideas/open/655_stack_destination_fan_in_authority_decomposition.md`
- `ideas/open/647_ordered_or_exclusive_stack_destination_fan_in_authority.md`
- `docs/destination_fan_in_authority/03_implementation_split.md`
- Refreshed idea 647 evidence under `build/agent_state/647_step2_family_revision/`

## Current Scope

- Decompose residual stack-destination register fan-in rows that are outside
  idea 637's select-materialized preserved-stack-fallback contract.
- Create focused probe definitions under `tests/backend/case/` before more
  producer implementation.
- Preserve the rejected `src/20021204-1.c` mutual-exclusion route and the
  rejected `src/20011109-2.c` idea 637 route as negative route evidence.

## Non-Goals

- Do not reopen idea 637.
- Do not implement RV64 consumption before prepared/prealloc producer facts
  exist.
- Do not rewrite expectations, unsupported markers, allowlists, timeouts, or
  pass/fail accounting.
- Do not use the original GCC torture rows as narrow named-case proof for a
  new family.

## Working Model

The old idea 647 route is parked, not closed. It still describes the desired
non-637 destination-authority capability, but the current first-family search
is too broad. This runbook should turn the residuals into smaller seams:
ordered final-state authority, mutual-exclusion authority, explicit merge
authority, and rejection authority for missing or mismatched facts.

Focused probes should make each seam observable before implementation touches
shared producer code.

## Execution Rules

- Keep routine progress in `todo.md`.
- Prefer probe design and focused diagnostics before implementation changes.
- Name producer fact shape, owner label, negative statuses, and consumer
  program point for every seam.
- Keep source freshness separate from destination authority.
- Escalate to reviewer scrutiny if a route tries to claim progress through
  classification-only changes or testcase-shaped behavior.

## Steps

### Step 1: Establish The Blocked Failure-Family Baseline

Goal: Preserve the useful idea 647 evidence as the baseline for decomposition.

Actions:

- Read `build/agent_state/647_step2_family_revision/summary.md`.
- Record the rejected `src/20021204-1.c` mutual-exclusion route and the
  rejected `src/20011109-2.c` idea 637 route in `todo.md`.
- List the remaining residual row shapes by consumer point, destination, source
  homes, current authority, and fragment status.
- Do not rerun broad diagnostics unless the supervisor requests fresh proof.

Completion check:

- `todo.md` names the blocked baseline and confirms that no Step 3
  implementation packet is currently selected.

### Step 2: Split Residuals Into Authority Seams

Goal: Turn the residual family into independently owned producer seams.

Actions:

- Classify each residual shape as ordered final-state, mutual-exclusion,
  explicit merge, rejection-only, or unknown.
- For each non-unknown seam, name the producer fact that would prove authority
  at the consumer point.
- For each seam, name at least one negative state that must remain fail-closed.
- Mark any row that only fits idea 637's selected contract as out of scope for
  this decomposition.

Completion check:

- `todo.md` contains a seam inventory with positive and negative evidence needs
  for each seam.

### Step 3: Define Focused Probe Files

Goal: Create probe specifications before shared producer implementation.

Actions:

- Draft focused backend/prepared probe intent for:
  - ordered final-state authority
  - mutual-exclusion authority
  - explicit merge authority
  - authority rejection
- Prefer files under `tests/backend/case/` with descriptive names such as:
  - `riscv64_stack_destination_ordered_final_state_authority.c`
  - `riscv64_stack_destination_mutual_exclusion_authority.c`
  - `riscv64_stack_destination_explicit_merge_authority.c`
  - `riscv64_stack_destination_authority_rejection.c`
- Keep each probe tied to one authority contract.

Completion check:

- The next implementation agent can add or update focused probes without
  reverse-engineering the GCC torture rows.

### Step 4: Select The First Follow-Up Implementation Seam

Goal: Pick one legal implementation follow-up after the focused seams are
observable.

Actions:

- Choose one seam only when it has a positive producer fact shape and a
  fail-closed negative probe.
- Record why the selected seam is outside idea 637.
- Record residual seams left out of scope.
- If no seam has positive evidence, leave implementation blocked and request a
  lifecycle decision instead of forcing a packet.

Completion check:

- `todo.md` names exactly one follow-up seam, its first probe, its negative
  proof, and the residual seams intentionally left blocked.

### Step 5: Prepare The Follow-Up Lifecycle Handoff

Goal: Convert the selected seam into the next executable implementation unit.

Actions:

- If the selected seam fits this decomposition idea, update `todo.md` with the
  precise executor packet.
- If the selected seam should become its own implementation idea, request
  plan-owner lifecycle work to create or switch to that idea.
- Do not claim backend/compiler progress from this decomposition until focused
  probes or producer facts are implemented and validated.

Completion check:

- Lifecycle state points to one executable implementation seam or explicitly
  records why implementation remains blocked.
