# Residual Stack Authority Revisit Prerequisites Runbook

Status: Active
Source Idea: ideas/open/702_residual_stack_authority_revisit_prerequisites.md
Activated from: queue item 702 after closure of `ideas/closed/700_prepared_mir_stack_view_contract.md`

## Purpose

Define the prepared-owned proof threshold required before parked residual stack
destination authority work in ideas 647 or 655 can resume.

## Goal

Produce a narrow prerequisite contract that rejects route-only evidence and
names the positive prepared producer evidence a future residual stack authority
family must show before implementation starts.

## Core Rule

Do not reactivate ideas 647 or 655 from Route 4, Route 5, Route 7,
`RouteIndexReferenceFacade`, dump labels, expectations, allowlists, or other
route compatibility evidence. Revisit is allowed only from positive prepared
producer evidence above route dumps.

## Read First

- `ideas/open/702_residual_stack_authority_revisit_prerequisites.md`
- `ideas/open/647_ordered_or_exclusive_stack_destination_fan_in_authority.md`
- `ideas/open/655_stack_destination_fan_in_authority_decomposition.md`

## Current Scope

- Define the minimum proof threshold for future activation of ideas 647 and
  655.
- Identify which prepared producer families are eligible revisit sources:
  ordered final-state, mutual-exclusion, explicit merge, aggregate stack-source,
  branch stack-load, or another named producer family.
- Keep 647 and 655 parked unless the selected threshold is met.
- Record any durable prerequisite evidence at the correct planning layer only
  when execution proves it is needed.

## Non-Goals

- Do not implement stack destination authority.
- Do not reactivate 647 or 655 during this runbook.
- Do not perform RV64 target materialization for residual stack authority.
- Do not change test expectations, unsupported markers, allowlists, timeout
  policy, or pass/fail accounting.
- Do not treat route dump cleanup or diagnostic wording as authority progress.

## Working Model

Route-numbered facts and dumps can describe historical compatibility, but they
do not authorize stack destinations, move bundles, freshness selection, or MIR
stack-path lowering. A valid residual stack authority prerequisite must name
the prepared producer fact shape, its owner, participating value homes, selected
move or bundle evidence, source and destination storage, freshness or stack
object evidence where applicable, and fail-closed MIR statuses before target
materialization.

## Execution Rules

- Prefer documentation or focused prepared/prealloc probes over implementation
  changes unless a step explicitly identifies a positive producer seam.
- Keep source idea edits rare. Put packet progress and temporary findings in
  `todo.md`; update this runbook only if the prerequisite contract or step
  ordering changes.
- Treat ideas 647 and 655 as parked inventory, not implementation targets.
- Reject named-case shortcuts around the historical residual GCC torture rows.
- For any code-changing probe, require fresh build proof plus the supervisor's
  delegated narrow test subset before accepting the packet.

## Ordered Steps

### Step 1: Establish The Revisit Baseline

Goal: Capture the current activation boundary after idea 700 without reopening
parked residual implementation routes.

Primary targets:
- Planning notes in `todo.md`
- Optional prerequisite documentation or probe design files if delegated by the
  supervisor

Actions:
- Inspect the source idea and the parked outcomes in ideas 647 and 655.
- Identify which evidence from the completed idea 700 is positive prepared
  producer evidence and which evidence remains insufficient for residual stack
  destination fan-in.
- List the route-only evidence classes that must stay rejected as direct stack
  authority.

Completion check:
- A concise baseline states why `PreparedBranchStackLoadAuthority` is not enough
  to resume 647 or 655, and names the missing residual stack-destination fan-in
  prerequisites.

### Step 2: Define The Positive Producer Threshold

Goal: Convert the baseline into an explicit prepared-owned activation threshold
for future residual authority work.

Primary targets:
- Focused prerequisite documentation or prepared/prealloc probe specs selected
  by the executor

Actions:
- Name the required destination value identity, destination home, destination
  storage kind, source value/home, selected move bundle or move resolution,
  selected freshness, stack object or aggregate source authority, and MIR
  fail-closed statuses as applicable.
- Separate eligible producer families from insufficient route compatibility
  evidence.
- Define at least one positive proof surface and at least one negative
  fail-closed proof surface required before a future implementation idea can
  start.

Completion check:
- The prerequisite threshold is concrete enough for a future lifecycle decision
  to say whether 647 or 655 remains parked or may be reactivated.

### Step 3: Bind Follow-Up Routing

Goal: Leave lifecycle state with clear follow-up conditions rather than a broad
implementation invitation.

Primary targets:
- `todo.md`
- `ideas/open/647_ordered_or_exclusive_stack_destination_fan_in_authority.md`
  and `ideas/open/655_stack_destination_fan_in_authority_decomposition.md` only
  if a durable prerequisite note is required

Actions:
- Map any eligible positive seam to one named future producer family.
- Keep unrelated or still-negative seams rejected with precise reasons.
- If the work discovers a separate implementation initiative, route it as a
  future open idea instead of expanding this runbook.

Completion check:
- Ideas 647 and 655 are still parked unless the prepared producer threshold is
  met, and the next lifecycle agent can tell exactly what evidence would
  unblock them.

### Step 4: Validate And Hand Off

Goal: Prove the prerequisite contract without weakening route or test policy.

Primary targets:
- Supervisor-selected proof subset
- `todo.md`

Actions:
- Run the delegated proof command for any changed documentation, probes, or
  code.
- Escalate to broader validation if the slice touches executable prepared,
  MIR, object, object-runtime, or runtime behavior.
- Record proof results in `todo.md`.

Completion check:
- The runbook has a fresh proof result when required, no route-only evidence is
  claimed as authority, and follow-up routing for 647 and 655 is explicit.
