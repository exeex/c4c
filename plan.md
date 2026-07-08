# Branch Stack-Load Freshness Contract Runbook

Status: Active
Source Idea: ideas/open/590_branch_stack_load_freshness_contract.md
Activated After: idea 589 closed in commit 6ec92b5c4

## Purpose

Define the branch-point freshness contract for stack-loaded branch sources and
migrate one representative branch stack-load authority route to explicit shared
freshness authority.

## Goal

Make one bounded branch stack-load route fail closed unless the source is fresh
for the branch terminator use point, while preserving existing 587, 588, and
589 freshness behavior.

## Core Rule

A complete stack home, stack object, branch payload, or clobber-safety fact is
necessary context but never sufficient freshness authority for a branch
condition, lhs, or rhs source.

## Read First

- ideas/open/590_branch_stack_load_freshness_contract.md
- current shared-prealloc freshness authority code around
  `plan_prepared_branch_stack_load_authority`
- existing freshness vocabulary and tests from ideas 587, 588, and 589

## Current Scope

- Audit the branch stack-load consumer set around
  `plan_prepared_branch_stack_load_authority`.
- State the branch-point freshness ownership rule in implementation-facing
  terms.
- Add or reuse the narrowest freshness use/source vocabulary needed for the
  representative branch stack-load route.
- Migrate one representative branch stack-load authority route to require
  selected freshness authority before accepting a source.
- Add focused proof that valid branch stack loads carry explicit freshness and
  invalid or stack-home-only authority fails closed.

## Non-Goals

- Do not implement typed or aggregate stack-source producer/publication facts.
- Do not migrate every branch, select, edge-publication, RV64, AArch64, or x86
  consumer.
- Do not redesign control-flow lowering, branch instruction selection, or
  terminator fragment emission.
- Do not change target ABI classification, `TargetProfile`, or physical
  register identity policy.
- Do not claim progress through expectation rewrites, unsupported-marker edits,
  allowlist edits, or runtime output changes.

## Working Model

- The branch use point is the branch terminator boundary: condition, lhs, and
  rhs sources must be fresh before target emission accepts the branch source.
- Freshness must be selected by a shared authority query using the correct
  branch use/source vocabulary.
- Missing, ambiguous, stale, wrong-value, wrong-use, future-producer, and
  stack-home-only authority must produce precise fail-closed status or
  diagnostics.
- Existing dependency or edge-publication terminology can be reused only when
  it accurately describes the branch terminator use point.

## Execution Rules

- Keep each step narrow enough for a build plus targeted proof.
- Prefer semantic freshness authority migration over target-local branch
  lowering edits.
- Preserve existing supported-path expectations unless the supervisor
  explicitly approves a temporary tactical exception.
- Record typed or aggregate producer/publication gaps for closure inventory,
  not as implementation scope for this runbook.
- For every code-changing step, run the build proof required by the supervisor
  and at least the focused freshness tests or dump checks touched by the step.

## Steps

### Step 1: Audit Branch Stack-Load Consumers

Goal: identify the representative branch stack-load route and the nearby
consumer set.

Primary target: shared-prealloc branch stack-load authority code around
`plan_prepared_branch_stack_load_authority`.

Actions:

- Inspect the condition, lhs, and rhs stack-load paths that can reach branch
  lowering.
- List which consumers already have prepared stack object, dependency or value
  identity, clobber-safety, and fail-closed status context.
- Pick one representative route for migration and explicitly mark the other
  consumers as protected, blocked, deferred, or out of scope.

Completion check:

- `todo.md` names the audited consumers, the selected representative route, and
  the reason it is the narrowest valid first migration target.

### Step 2: Define Branch-Point Freshness Ownership

Goal: make the branch freshness rule concrete before changing acceptance
behavior.

Actions:

- Determine the freshness use kind for branch stack loads: branch condition,
  branch lhs/rhs, or a shared branch stack-load source.
- Determine accepted source kinds and the program point or ordering fact that
  proves freshness before the branch terminator.
- Decide whether existing 587/589 vocabulary fits or whether a distinct
  branch-specific use kind is required.
- Keep structural stack-home checks as necessary but insufficient prerequisites.

Completion check:

- The implementation surface states or encodes the ownership rule, and the
  chosen vocabulary cannot be confused with dependency or edge-publication
  freshness unless that reuse is semantically justified.

### Step 3: Wire Freshness Authority Into One Route

Goal: migrate the selected branch stack-load authority route from structural
acceptance to explicit freshness acceptance.

Actions:

- Add or reuse the freshness query needed by the selected route.
- Require selected freshness authority before accepting the branch source.
- Preserve fail-closed behavior for missing, ambiguous, stale, wrong-value,
  wrong-use, future-producer, and stack-home-only authority.
- Keep diagnostics or printer output precise enough to show selected or
  missing branch freshness.

Completion check:

- The selected route cannot accept a branch stack source solely because the
  stack home and local branch payload are complete.

### Step 4: Add Focused Proof

Goal: prove the migrated route is freshness-authorized and invalid authority
fails closed.

Actions:

- Add or update focused tests or prepared dump assertions for an accepted branch
  stack load with explicit freshness authority.
- Add negative proof for missing or invalid freshness.
- Include a stack-home-only case that remains rejected.
- Keep existing 587, 588, and closed-589 freshness tests green.

Completion check:

- Focused proof shows accepted branch stack loads are authorized by explicit
  source freshness, and invalid branch authority fails closed with visible
  status or diagnostics.

### Step 5: Closure Inventory and Follow-Up Decision

Goal: prepare the source-idea closure answers without broadening this runbook.

Actions:

- Summarize audited consumers and the branch-point freshness ownership rule.
- Identify which consumers remain unwired and why.
- Decide whether typed or aggregate stack-source producer facts should become
  the next idea or remain deferred.
- Identify any RV64, AArch64, x86, or architecture-contract tail ready for a
  follow-up idea.

Completion check:

- `todo.md` contains the closure inventory needed to answer every closure note
  requirement from the source idea.

## Acceptance Check

- One representative branch stack-load route consults shared freshness
  authority before accepting a source.
- Missing, ambiguous, stale, wrong-value, wrong-use, future-producer, and
  stack-home-only authority fail closed.
- Focused tests or prepared dumps prove explicit branch freshness is visible.
- Existing 587, 588, and 589 freshness tests continue to pass.
- The final closure note answers whether typed or aggregate stack-source
  producer facts should become a follow-up idea.
