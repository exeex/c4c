# Route 5 Edge/Join-Source Identity Adapter Runbook

Status: Active
Source Idea: ideas/open/204_route5_edge_join_source_adapter.md

## Purpose

Activate the Route 5 follow-up by migrating one selected CFG edge,
join-source, edge-copy, or wrapper reader to semantic source identity from BIR
records while keeping prepared scheduling and policy authority intact.

Goal: one selected reader obtains only Route 5 semantic edge or join-source
identity from BIR, with prepared fallback preserved for every policy-sensitive
fact.

Core Rule: Route 5 may provide semantic edge or join-source identity only; it
must not become the authority for move scheduling, storage homes, branch
policy, wrapper formatting, execution-site placement, coalescing, or final
edge-copy behavior.

## Read First

- `ideas/open/204_route5_edge_join_source_adapter.md`
- Existing Route 5 record/index definitions and publication sites
- Existing prepared edge/join-source readers, wrappers, and fallback helpers
- Nearby tests for CFG edge publication, join-source identity, no-source,
  memory-source, duplicate/conflict, and fallback behavior

## Current Scope

- Select exactly one CFG edge, join-source, edge-copy, or wrapper reader that
  needs Route 5 semantic source identity.
- Use Route 5 records or indexes only for predecessor, successor,
  destination/source identity, source value/name/kind, source producer
  block/instruction, optional memory-source identity, no-source status, and
  current-block join incoming source identity.
- Preserve prepared fallback for move scheduling, storage/homes, move bundles,
  branch policy, wrapper formatting, final edge-copy behavior, scratch policy,
  cycle temporaries, execution-site placement, and coalescing.

## Non-Goals

- Do not replace all edge-publication, move-bundle, or current-block join
  helper groups.
- Do not move parallel-copy scheduling, source/destination homes, move order,
  branch spelling, scratch policy, cycle temporary routing, execution-site
  placement, coalescing, final edge-copy records, or wrapper formatting into
  BIR schema.
- Do not perform broad edge/join lookup contraction.
- Do not weaken expected-output or unsupported-test contracts.

## Working Model

- Treat Route 5 as a semantic identity adapter.
- Treat prepared state as the authority for policy, storage, movement, and
  emitted output.
- Fail closed when Route 5 facts are absent, mismatched, duplicated,
  conflicting, or not applicable to the selected reader.
- Keep the first implementation narrow: one reader or wrapper boundary, one
  proof ladder, then supervisor review before broadening.

## Execution Rules

- Keep implementation slices small enough for build plus targeted tests.
- Prefer existing helper APIs and naming near the selected reader.
- Add no testcase-shaped shortcuts, named-fixture branches, or string rewrites
  that mask unchanged capability.
- If wrapper output is touched, prove string authority and adjacent branch or
  parallel-copy sanity.
- After each code-changing packet, run the supervisor-delegated build/test
  command exactly and record proof in `todo.md`.
- Escalate to broader validation before claiming the adapter complete if the
  selected boundary affects shared edge/join or wrapper behavior.

## Ordered Steps

### Step 1: Select The Route 5 Adapter Boundary

Goal: identify exactly one reader or wrapper boundary for the first Route 5
semantic identity adapter.

Primary target: one existing CFG edge, join-source, edge-copy, or wrapper
reader.

Actions:

- Inspect Route 5 publication records and existing prepared reader call sites.
- Choose the smallest boundary that needs semantic edge or join-source
  identity and already has prepared fallback for policy-sensitive behavior.
- Map which facts can come from Route 5 and which facts must remain prepared.
- Identify the targeted tests and any missing negative coverage.

Completion check:

- `todo.md` names one selected boundary, the Route 5 facts it may read, the
  prepared facts it must keep, and the narrow proof command for Step 2.

### Step 2: Add The Fail-Closed Route 5 Reader Adapter

Goal: introduce the adapter path without changing prepared policy authority.

Primary target: the selected reader and its nearest Route 5 lookup helpers.

Actions:

- Add or reuse a helper that validates the selected Route 5 edge/join-source
  identity.
- Reject absent facts, mismatched predecessor/successor or current-block
  identity, duplicate/conflicting facts, and unsupported memory/no-source
  combinations.
- Return to prepared fallback for movement, storage, branch policy, wrappers,
  and emitted output.
- Keep existing helper groups and broad lookup surfaces intact.

Completion check:

- The project builds.
- Targeted proof covers route/prepared agreement, absent or mismatch fallback,
  duplicate/conflict rejection, no-source status, and memory-source status for
  the selected boundary.

### Step 3: Wire The Selected Reader To The Adapter

Goal: make the selected reader consume Route 5 semantic identity when valid and
prepared fallback when invalid or policy-bound.

Primary target: the selected reader or wrapper boundary from Step 1.

Actions:

- Thread the validated Route 5 semantic identity into the selected reader.
- Keep prepared move scheduling, homes, move bundles, branch spelling,
  wrapper formatting, scratch policy, cycle temporaries, final edge-copy
  records, and output authority unchanged.
- Preserve output stability unless the source idea explicitly justifies a
  visible diagnostic/oracle adjustment.
- Add focused tests for success, fallback, conflict, and edge/no-source cases.

Completion check:

- The selected reader obtains only Route 5 semantic edge or join-source
  identity from BIR.
- Prepared fallback remains visibly authoritative for policy and emitted
  output.
- Targeted tests pass with no expected-output weakening.

### Step 4: Prove Adapter Completeness And Route Quality

Goal: establish that the adapter is real capability progress and not a narrow
fixture shortcut.

Primary target: tests and review evidence for the selected adapter boundary.

Actions:

- Run the supervisor-selected build and targeted test subset.
- Add or run nearby same-feature cases to show the adapter is not
  testcase-shaped.
- If wrappers were touched, include adjacent branch or parallel-copy sanity and
  unchanged output-string proof.
- Document remaining non-goals and any proposed next Route 5 boundary in
  `todo.md` without expanding this plan.

Completion check:

- Proof covers success, no-source, memory-source, duplicate/conflict,
  absent/mismatch fallback, and output stability where relevant.
- No broad helper replacement, policy migration, expected-string weakening, or
  unsupported downgrade is present.
- The supervisor can decide whether to accept the slice, request reviewer
  scrutiny, or activate a separate follow-up idea.
