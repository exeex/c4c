# Plan: Idea 259 x86 Route 5 prepared edge publication agreement bridge

Status: Active
Source Idea: ideas/open/259_phase_f3_x86_route5_prepared_edge_publication_agreement_bridge.md

## Purpose

Add the narrow x86 agreement bridge that checks Route 5 CFG-edge publication
source identity against the prepared `PreparedEdgePublication` row already
used by the x86 prepared edge-publication consumer.

Goal: x86 treats a prepared edge-publication row as semantically agreed only
when the matching Route 5/BIR edge publication record agrees on the same edge,
destination, source value, and source producer, with Route 3 memory agreement
required for dynamic `LoadLocal` sources.

Core rule: keep prepared lookup/status and x86 target emission policy as
compatibility authority; the new Route 5 agreement bridge may gate agreement
but must not own formatting, register choice, carrier choice, helper naming,
or output spelling.

## Read First

- `ideas/open/259_phase_f3_x86_route5_prepared_edge_publication_agreement_bridge.md`
- Route 5 records and source identity helpers:
  `Route5CfgEdgePublicationRecord` and
  `BirCfgEdgePublicationSourceIdentity`
- x86 prepared publication consumer:
  `consume_edge_publication_move_intent(...)`
- Route 3 `LoadLocal` source-memory agreement facade from idea 258
- Existing prepared edge-publication lookup/status tests and x86 handoff tests

## Scope

- Build one narrow x86-local Route 5/BIR agreement consumer or MIR query facade.
- Join the same predecessor, successor, destination value, source value, and
  source producer from Route 5/BIR to the prepared `PreparedEdgePublication`
  row used by x86.
- Require Route 3 source-memory identity agreement before accepting dynamic
  `LoadLocal` publication sources as agreeing.
- Preserve public prepared `edge_publications` lookup helpers, status names,
  helper/oracle names, fallback rows, x86 dispatch/status rows, module output,
  and target output formatting.

## Non-Goals

- Do not delete, privatize, rename, or bypass public prepared
  `edge_publications` helpers.
- Do not move move selection, register choice, carrier/helper selection,
  layout, instruction spelling, stack/source-home policy, or module-output
  formatting into Route 5/BIR.
- Do not rewrite RISC-V behavior, RISC-V diagnostic fields, x86 output strings,
  Route 4, Route 5 lowering, edge lowering, register allocation, or emission
  policy outside this bridge.
- Do not claim progress through expectation downgrades, helper/status renames,
  classification-only edits, or named-case shortcuts.

## Execution Rules

- Keep implementation changes local to the Route 5 agreement query/facade and
  the x86 prepared edge-publication consumer path.
- Fail closed for duplicate, mismatch, absent or wrong-edge, unsupported,
  prepared-only, Route 5-only, non-agreeing fallback, and dynamic `LoadLocal`
  Route 3 source-memory disagreement cases.
- Preserve compatibility output when agreement is unavailable.
- Add focused backend proof before accepting implementation slices. A single
  target testcase is not sufficient if nearby same-feature rows remain
  unexamined.
- Use `todo.md` for packet progress and blockers. Rewrite this runbook only if
  the route, boundaries, or proof contract changes.

## Step 1: Consumer and Authority Inventory

Goal: locate the concrete x86 consumer path, Route 5/BIR source identity facts,
prepared publication lookup row, and Route 3 dynamic source-memory authority.

Actions:

- Inspect the Route 5 record/source identity definitions and lookup helpers.
- Inspect `PreparedEdgePublication` construction, lookup, status, and public
  helper surfaces used by x86.
- Trace `consume_edge_publication_move_intent(...)` to the exact x86 decision
  point where agreement can be checked without moving target policy.
- Identify existing tests that cover prepared edge-publication lookup/status,
  x86 dispatch/status rows, route-debug output, and dynamic `LoadLocal`
  publication sources.
- Record the fail-closed matrix in `todo.md`, including which rows are
  naturally expressible by existing supported fixtures.

Completion check:

- `todo.md` names the consumer hook point, Route 5 authority fields, prepared
  compatibility surface, Route 3 memory-agreement dependency, and focused proof
  surfaces for Step 2.

## Step 2: Agreement Facade Design

Goal: define the narrow agreement contract before editing the x86 consumer.

Actions:

- Choose the smallest x86-local query/facade shape that accepts the prepared
  edge-publication row and Route 5/BIR lookup context.
- Define exact equality checks for predecessor, successor, destination value,
  source value, and source producer.
- Define duplicate, absent, wrong-edge, unsupported, prepared-only,
  Route 5-only, fallback, and mismatch behavior as fail-closed results.
- Define the `LoadLocal` branch so Route 5 agreement requires the existing
  Route 3 source-memory agreement authority before accepting the publication.
- Record retained compatibility behavior for status strings, helper names,
  fallback rows, x86 output, and RISC-V diagnostics.

Completion check:

- `todo.md` contains the selected contract and rejection matrix, with no
  implementation code changed unless delegated separately.

## Step 3: Implement Narrow x86 Agreement Bridge

Goal: wire the selected Route 5 agreement facade into the x86 prepared
edge-publication consumer path without changing public prepared compatibility
or target emission policy.

Actions:

- Add the x86-local facade or MIR query helper chosen in Step 2.
- Gate the relevant x86 publication consumer path on Route 5/prepared
  agreement.
- Keep non-agreeing rows on the existing compatibility fallback path.
- For dynamic `LoadLocal` source publications, call through the established
  Route 3 source-memory agreement path rather than comparing raw ids or
  prepared-only metadata.
- Avoid renaming public helpers, rewriting statuses, or changing emitted x86
  spelling.

Completion check:

- Focused build and test proof passes for the prepared lookup/status and x86
  handoff/route-debug subset selected by the supervisor.
- `todo.md` records the files changed, proof command, and remaining proof gaps.

## Step 4: Focused Agreement and Rejection Proof

Goal: prove the agreeing path and supported fail-closed rows without synthetic
  stale state or expectation downgrades.

Actions:

- Add or extend focused backend tests for the positive x86 agreement path.
- Cover naturally reachable duplicate, mismatch, absent or wrong-edge,
  unsupported, prepared-only, Route 5-only, non-agreeing fallback, and dynamic
  `LoadLocal` Route 3 disagreement rows.
- Preserve prepared edge-publication lookup/status expectations and x86 output
  spelling.
- If a rejection row cannot be expressed by supported fixtures, record that
  explicitly in `todo.md` instead of hand-building stale prepared state or
  weakening the contract.

Completion check:

- Focused proof passes and `todo.md` records which rejection rows are proved
  and which are intentionally out of scope because no supported surface exists.

## Step 5: Compatibility Sweep and Closure Readiness

Goal: verify the bridge did not move compatibility authority or regress nearby
backend behavior.

Actions:

- Recheck prepared edge-publication lookup helpers, status names, helper/oracle
  names, fallback rows, x86 dispatch/status rows, route-debug output, module
  output, and exact target output formatting.
- Confirm RISC-V behavior and diagnostic fields are unchanged unless the proof
  command explicitly includes them.
- Confirm no expectations were downgraded and no output baselines were rewritten
  to hide behavior changes.
- Run the supervisor-selected broader proof before closure.

Completion check:

- `todo.md` records closure readiness, proof results, compatibility surfaces
  preserved, and any residual non-blocking proof limits.
