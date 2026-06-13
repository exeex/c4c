# Phase E5 Route 4/5 Edge Publication Identity Adapter Runbook

Status: Active
Source Idea: ideas/open/241_phase_e5_route45_edge_publication_identity_adapter_follow_up.md

## Purpose

Implement one narrow Route 4 or Route 5 edge-publication identity adapter while
preserving prepared publication construction, target/prepared policy,
diagnostics, fallback, wrappers, expected strings, and baselines.

## Goal

Route exactly one selected Route 4 or Route 5 publication, edge, or join-source
identity read through an agreement-gated adapter that consumes semantic identity
only when it matches the existing prepared lookup or publication answer.

## Core Rule

Treat Route 4/5 as semantic identity only. Do not move publication
construction, move scheduling, storage/home policy, block order, branch/output
spelling, wrapper output, formatting, instruction selection, or emission policy
into Route 4, Route 5, or target-neutral BIR ownership.

## Read First

- `ideas/open/241_phase_e5_route45_edge_publication_identity_adapter_follow_up.md`
- `docs/bir_prealloc_fusion/phase_e5_prepared_bir_module_demotion_or_retirement_gate.md`
- `docs/bir_prealloc_fusion/prepared_function_lookups_ownership_readiness_map.md`
- `docs/bir_prealloc_fusion/residual_route_view_consumer_migration_map.md`
- `ideas/closed/239_phase_e5_prepared_bir_module_demotion_or_retirement_gate.md`
- `ideas/closed/240_phase_e5_route3_memory_source_identity_adapter_follow_up.md`

## Current Scope

- One Route 4 or Route 5 publication, edge, or join-source identity read inside
  `edge_publications` or `edge_publication_source_producers`.
- One agreement-gated adapter for that selected read.
- Prepared fallback for absent, invalid, duplicate/conflict, ambiguous,
  mismatch, unsupported, prepared-only, policy-sensitive, and non-agreement
  cases where they apply to the selected reader.
- Unchanged prepared publication construction and target/prepared policy for
  diagnostics, helper-oracle tests, wrappers, compatibility consumers, and
  debug/printer paths.
- Nearby same-feature edge or join-source proof that the adapter is semantic
  rather than fixture-shaped.

## Non-Goals

- Do not delete, privatize, or replace all of `edge_publications` or
  `edge_publication_source_producers`.
- Do not delete, privatize, rename, or hide `PreparedFunctionLookups` or
  `PreparedBirModule`.
- Do not open draft 155 or claim aggregate retirement readiness.
- Do not move publication construction, move scheduling, storage/home policy,
  block order, wrapper output, branch/output spelling, formatting, instruction
  selection, or emission policy into Route 4, Route 5, or target-neutral BIR
  ownership.
- Do not rewrite expectations, helper names, supported-path status, wrapper
  output, or baselines as proof.
- Do not downgrade supported tests to unsupported or weaken prepared fallback,
  diagnostic/oracle, debug/printer, wrapper, or helper contracts.

## Working Model

- Existing Route 4/5 records may answer selected publication, edge, or
  join-source identity questions.
- Prepared publication and lookup surfaces remain authoritative for target
  policy, public compatibility, oracles, diagnostics, fallback, and output.
- A valid adapter first obtains the current prepared answer, then accepts a
  route/BIR semantic answer only when the route fact agrees with that prepared
  answer.
- Any missing, ambiguous, mismatched, invalid, unsupported, policy-sensitive,
  or non-agreement state must fall back to the existing prepared path.

## Execution Rules

- Name the selected reader before implementation. If the reader consumes
  target policy instead of semantic identity, stop and select a narrower read
  or return to the supervisor.
- Keep the implementation to one reader and one adapter boundary.
- Preserve public prepared lookup APIs and prepared debug/printer/helper
  behavior unless a separate source idea explicitly changes them.
- Add capability proof around the adapter and nearby same-feature cases; do not
  rely on one named fixture.
- Treat expectation rewrites, baseline refreshes, helper renames, unsupported
  downgrades, facade-only moves, and named-case shortcuts as reject signals.
- For code-changing steps, run a fresh build or compile proof plus the
  supervisor-selected narrow test subset. Escalate to broader validation if the
  diff touches shared prepared lookup, diagnostic/oracle, wrapper, or
  target-policy surfaces.

## Steps

### Step 1: Select The Route 4/5 Publication Identity Reader

Goal: identify exactly one semantic Route 4 or Route 5 publication, edge, or
join-source identity read that is eligible for an agreement-gated adapter.

Primary targets:

- Route 4/5 `edge_publications` readers.
- Route 4/5 `edge_publication_source_producers` readers.
- Existing prepared publication, edge, and join-source helper/oracle surfaces.
- Candidate source files and tests discovered from those helper names.

Actions:

- Inspect candidate readers and choose one that asks only for publication,
  edge, or join-source identity, not construction, move/home/storage policy,
  block order, wrapper output, formatting, or emission policy.
- Record the chosen reader, route fact, prepared answer, fallback path, and
  nearby same-feature cases in `todo.md`.
- Identify the exact tests or oracle rows that prove positive agreement,
  missing route fact, mismatch, ambiguity/conflict if applicable, and prepared
  fallback.

Completion check:

- `todo.md` names one selected reader and explains why it is semantic identity
  only, with no target/prepared policy ownership transfer.

### Step 2: Implement The Agreement-Gated Adapter

Goal: route the selected reader through one adapter that accepts Route 4/5
semantic identity only after agreement with the prepared result.

Primary targets:

- The selected reader from Step 1.
- The Route 4/5 semantic identity helper or record used by that reader.
- The existing prepared fallback path.

Actions:

- Add the smallest local adapter or helper needed to compare the Route 4/5 fact
  with the prepared answer.
- Preserve the prepared fallback path for absent, invalid, ambiguous,
  mismatch, unsupported, prepared-only, policy-sensitive, and non-agreement
  cases relevant to the selected reader.
- Keep publication construction, move scheduling, storage/home policy, block
  order, wrappers, diagnostics, formatting, and emission policy on the existing
  prepared/target path.
- Avoid public API hiding, aggregate reshaping, and rename-only changes.

Completion check:

- The selected positive path can consume the route/BIR semantic identity after
  prepared agreement, while every non-agreement path keeps existing prepared
  behavior.

### Step 3: Prove Fallback And Public Compatibility

Goal: prove the adapter preserves prepared fallback, diagnostics, public lookup
delivery, and same-feature behavior.

Primary targets:

- Existing Route 4/5 edge publication, edge, and join-source tests.
- Prepared lookup helper/oracle tests.
- Prepared printer/debug, route-debug, wrapper, and target-output surfaces
  touched by the selected reader.

Actions:

- Add or update focused tests for the selected positive agreement case.
- Cover absent, invalid, duplicate/conflict, ambiguous, mismatch, unsupported,
  prepared-only, policy-sensitive, and non-agreement fallback where applicable.
- Include nearby same-feature publication, edge, or join-source cases so the
  proof is not tied to one named fixture.
- Verify helper-oracle names and status labels, route-debug output, prepared
  printer/debug output, wrapper output, expected strings, supported-path
  contracts, and baselines remain byte-stable unless separately approved.

Completion check:

- The narrow proof demonstrates semantic adapter behavior and preserved public
  prepared compatibility without expectation downgrades or baseline refreshes.

### Step 4: Validate Target-Policy No-Change Surfaces

Goal: prove the adapter did not move target-policy or output ownership into
Route 4/5.

Primary targets:

- Adjacent block order, move/home/storage, branch/output spelling, wrapper,
  formatting, instruction selection, and emission surfaces.
- The supervisor-selected build and test subset.

Actions:

- Run fresh build or compile proof after implementation.
- Run the delegated narrow CTest subset for the selected reader and nearby
  publication/edge/join-source cases.
- Inspect the diff for expectation rewrites, baseline refreshes, unsupported
  downgrades, helper renames, facade-only moves, aggregate reshuffles, or
  named-case shortcuts.
- Escalate to broader validation if shared prepared lookup, wrapper,
  diagnostic/oracle, or target-policy surfaces were modified.

Completion check:

- `todo.md` records passing proof and confirms target-policy, wrapper,
  expected-string, and baseline behavior remained unchanged.

### Step 5: Prepare Closure Evidence

Goal: make the completed Route 4/5 adapter slice reviewable for lifecycle
closure without broadening the source idea.

Primary targets:

- `todo.md`
- The selected code and test diff.
- Canonical proof logs chosen by the supervisor.

Actions:

- Summarize the selected reader, adapter boundary, fallback cases, nearby
  same-feature proof, and no-change target-policy evidence.
- Record exact validation commands and log paths.
- State explicitly that no whole `edge_publications`,
  `edge_publication_source_producers`, `PreparedFunctionLookups`,
  `PreparedBirModule`, or draft 155 retirement readiness is claimed.

Completion check:

- `todo.md` contains closure-ready evidence for the plan owner to decide
  whether source idea 241 is complete.
