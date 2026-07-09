# Prepared Stack-Destination Fan-In Authority Producer Runbook

Status: Active
Source Idea: ideas/open/637_prepared_stack_destination_fan_in_authority_producer.md

## Purpose

Define one explicit prepared/prealloc destination authority contract for
non-parallel register-source fan-in into a single stack destination.

Goal: publish producer-side destination authority facts so later RV64 consumer
work can accept only proven fan-in shapes.

Core Rule: do not let RV64 choose among multiple register sources for one stack
destination from source freshness, move order, filenames, block labels, or
final assembly. Destination fan-in legality must come from explicit
prepared/prealloc authority.

## Read First

- ideas/open/637_prepared_stack_destination_fan_in_authority_producer.md
- docs/destination_fan_in_authority/03_implementation_split.md
- Prior related ideas named by the source idea only as needed for local
  evidence: 607, 584, and 630.

## Current Scope

- Refresh the six idea-630 spillover rows:
  - `src/20011109-2.c`
  - `src/20021204-1.c`
  - `src/920429-1.c`
  - `src/930429-1.c`
  - `src/pr34415.c`
  - `src/ptr-arith-1.c`
- Include at least one wider destination fan-in representative from the idea
  607 evidence if needed to select a contract.
- Choose exactly one first producer authority family:
  ordered final-state authority, mutual-exclusion authority, or semantic merge
  authority.
- Publish authority facts on prepared/prealloc move bundles and participating
  moves.
- Keep source freshness separate from destination fan-in authority.

## Non-Goals

- Do not implement RV64 target materialization for newly authorized fan-in.
- Do not infer destination legality from testcase names, source order, value
  ids, block labels, final assembly, move-vector order, or diagnostics.
- Do not reuse move-bundle source freshness as destination authority.
- Do not reopen string-constant local-memory policy.
- Do not change ABI, runtime, expectations, unsupported markers, allowlists,
  timeouts, or accounting.

## Working Model

Prepared/prealloc owns the first explicit destination authority. RV64 remains a
fail-closed consumer until a later idea teaches it to materialize newly
authorized fan-in. The first route should prove one legal authority family and
one missing-authority rejection without broadening unrelated local-memory,
freshness, or string-label paths.

Negative states must stay observable for:

- `authority=none`
- stale or missing source freshness
- missing final-state or predicate evidence
- unsupported authority kinds
- bundle-versus-move mismatches

## Execution Rules

- Work in small packets and update `todo.md` after each executor packet.
- Preserve source-idea intent; do not edit the source idea for routine
  execution notes.
- Prefer producer-side semantic facts over RV64-side selection.
- Treat testcase-shaped matching as route drift.
- For code-changing steps, use the supervisor-delegated proof command exactly
  and record it in `test_after.log` unless delegated otherwise.
- Escalate to broader validation when a packet changes shared prepared,
  prealloc, or move-bundle authority surfaces.

## Ordered Steps

### Step 1. Refresh Fan-In Evidence And Select Contract

Goal: prove the current blocker family and select exactly one producer
authority contract before implementation.

Primary targets:

- Current diagnostics and dumps for the six spillover rows.
- Destination fan-in evidence from docs or closed idea 607 as needed.

Actions:

- Reproduce the representative rows with current diagnostics.
- Confirm they are blocked by missing destination authority, not by
  string-constant local-memory admission or source freshness.
- Compare ordered final-state, mutual-exclusion, and semantic merge authority
  against the current evidence.
- Select one first contract and record the reasoning in `todo.md`.
- Identify one legal proof shape and one missing-authority rejection shape for
  the next packet.

Completion check:

- `todo.md` names the selected authority family, rows refreshed, first legal
  target, first negative target, and proof command/output path.

### Step 2. Publish Prepared Destination Authority Facts

Goal: represent the selected destination authority family in prepared/prealloc
facts with explicit ownership and negative states.

Primary targets:

- Prepared/prealloc move-bundle authority production code discovered in
  Step 1.
- Focused tests or probes for the selected legal and missing-authority shapes.

Actions:

- Add the selected authority fact to move bundles and participating moves.
- Keep source freshness modeled and checked separately.
- Preserve diagnostics for missing, unsupported, ambiguous, stale, and
  mismatched authority states.
- Avoid RV64 materialization changes beyond any fail-closed consumer checks
  needed to observe the new producer state.

Completion check:

- A complete-authority prepared fan-in shape publishes the new fact.
- Missing or mismatched authority remains rejected with precise diagnostics.
- Fresh build or delegated compile proof is recorded.

### Step 3. Prove Legal And Negative Fan-In Behavior

Goal: show the selected contract moves at least one supported producer-side
fan-in shape past the missing-authority owner while preserving fail-closed
behavior.

Primary targets:

- The focused legal row or unit test selected in Step 1.
- The focused negative row or unit test selected in Step 1.

Actions:

- Run the supervisor-selected narrow proof.
- Inspect logs for authority ownership rather than expectation-only movement.
- If no legal first packet exists under the selected contract, record the exact
  missing producer evidence and ask for lifecycle review instead of widening
  scope.

Completion check:

- Legal and negative proof results are recorded in `todo.md`.
- The route does not claim RV64 materialization progress unless a downstream
  consumer idea has been activated separately.

### Step 4. Broader Consistency Check And Handoff

Goal: leave the source idea in a coherent state for either further producer
packets, downstream RV64 consumer work, or closure judgment.

Primary targets:

- Spillover rows and nearby fan-in representatives.
- Any touched prepared/prealloc shared authority surface.

Actions:

- Rerun the agreed broader subset for the changed surface.
- Verify unrelated local-memory, string-label, source freshness, ABI, and
  runtime owners did not move through expectation rewrites or relaxed
  diagnostics.
- Record remaining producer gaps or recommend a separate downstream RV64
  consumer idea if producer authority is complete.

Completion check:

- `todo.md` contains final proof, residuals, and a closure or follow-up
  recommendation for the supervisor.
