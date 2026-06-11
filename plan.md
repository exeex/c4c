# Route 4 Publication Identity Adapter Runbook

Status: Active
Source Idea: ideas/open/203_route4_publication_identity_adapter.md

## Purpose

Activate the bounded Route 4 follow-up from the Phase B2 schema-readiness
audit: migrate one selected publication identity reader or wrapper boundary to
BIR Route 4 semantic publication identity while retaining prepared publication
mechanics and output compatibility.

## Goal

Land one non-regressive Route 4 publication identity adapter with explicit
success, absence, mismatch, ambiguity, wrong-reference, and output proof.

## Core Rule

Route 4 may provide only semantic publication identity. Prepared publication
lookup remains authoritative for mechanics, move/home policy, storage,
stack-source extension, block-order emission, wrapper formatting, immediate
payload spelling, and emitted/debug output.

## Read First

- `ideas/open/203_route4_publication_identity_adapter.md`
- `docs/bir_prealloc_fusion/phase_b2_selected_route_extension_schema_readiness.md`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir.cpp`

## Current Scope

- Select exactly one current-block, block-entry, edge-publication, or wrapper
  reader that needs publication identity.
- Use Route 4 records or indexes for block/program-point availability,
  destination/source value identity, source producer identity, producer
  instruction/index, and source-producer kind.
- Preserve prepared lookup fallback for publication mechanics, wrapper
  compatibility, and emitted/debug output.
- Add or update proof for absent, mismatch, duplicate, ambiguous, and
  wrong-reference behavior, plus output stability where wrappers or
  diagnostics are affected.

## Non-Goals

- Do not replace all edge-publication lookups or public prepared publication
  helper groups.
- Do not move edge-copy emission, move/home/storage policy, stack-source
  extension, block-order emission, immediate publication payload spelling,
  wrapper formatting, or emitted strings into BIR schema.
- Do not clean up retained prepared policy as a standalone task.
- Do not refresh expected output or baselines as proof of ownership.

## Working Model

- BIR Route 4 records are identity inputs, not publication mechanics or output
  authority.
- The first implementation slice should pick one reader or wrapper boundary
  and prove the route/prepared agreement boundary before any broader migration.
- Missing, conflicting, ambiguous, duplicate, or wrong-reference Route 4 facts
  must fail closed or fall back exactly where the selected reader contract
  requires.

## Execution Rules

- Keep each code slice bounded to one selected reader or wrapper boundary.
- Name retained prepared fallback explicitly in proof notes where that prevents
  accidental promotion of mechanics into BIR.
- Use semantic route/index checks instead of fixture-name or expected-string
  shortcuts.
- Preserve x86/riscv wrapper, route-oracle, and emitted/debug output unless the
  source idea is explicitly updated by the plan owner.
- For code-changing steps, run fresh build or compile proof plus the delegated
  targeted test subset before marking the step complete.

## Step 1: Select Reader And Baseline Proof

Goal: identify one publication identity reader or wrapper boundary and define
the narrow proof command for the first adapter slice.

Primary targets:

- current-block, block-entry, edge-publication, or wrapper consumers that read
  prepared publication identity
- tests covering the selected reader's success and failure modes

Actions:

- Inspect publication identity readers and choose one bounded reader or wrapper
  boundary.
- Record why the reader uses Route 4 semantic identity rather than prepared
  publication mechanics or output policy.
- Identify existing tests or fixtures for success, absence, mismatch,
  duplicate, ambiguity, wrong-reference fallback, and output stability.
- Run or delegate a baseline command for the selected proof subset before code
  changes when the supervisor asks for regression-log pairing.

Completion check:

- `todo.md` names the selected reader or wrapper boundary, retained prepared
  fallback, proof subset, and any missing test coverage required before
  implementation.

## Step 2: Add Route 4 Identity Adapter

Goal: introduce the smallest adapter needed for the selected reader to obtain
Route 4 semantic publication identity from BIR.

Primary targets:

- Route 4 record/index lookup helpers near existing BIR backend lookup code
- the selected reader's immediate publication identity lookup boundary

Actions:

- Add a narrow helper or adapter that validates block/program-point
  availability and publication identity compatibility.
- Return semantic identity only: destination/source value identity, source
  producer identity, producer instruction or index, and source-producer kind.
- Reject absent, mismatched, duplicate, conflicting, ambiguous, or
  wrong-reference Route 4 facts according to the selected reader contract.
- Keep prepared lookup calls in place for publication mechanics, wrapper
  compatibility, and emitted/debug output.

Completion check:

- The adapter compiles, is used only by the selected reader or wrapper path,
  and cannot supply move/home policy, storage, stack-source extension,
  block-order emission, wrapper formatting, payload spelling, or emitted
  strings.

## Step 3: Wire The Selected Reader

Goal: migrate the selected reader or wrapper boundary to consume Route 4
semantic identity while preserving prepared fallback behavior.

Primary targets:

- the selected publication identity reader or wrapper boundary
- any local wrapper needed to compare Route 4 identity with prepared facts

Actions:

- Replace only the selected reader's semantic publication identity source with
  the Route 4 adapter.
- Keep prepared publication lookup authority for all out-of-scope mechanics and
  output facts.
- Fail closed or fall back on missing, mismatched, duplicate, ambiguous, or
  wrong-reference cases.
- Avoid helper privatization, broad edge-publication replacement, wrapper
  policy migration, or unrelated cleanup.

Completion check:

- The selected reader obtains only Route 4 semantic publication identity from
  BIR, and all mechanics, wrapper, and output-sensitive behavior still flows
  through prepared lookup authority.

## Step 4: Prove Success, Rejection, Fallback, And Output

Goal: prove the adapter boundary is real and non-regressive.

Primary targets:

- focused unit or integration tests for the selected reader
- existing wrapper, route-oracle, or target-output checks affected by the
  selected path

Actions:

- Add or update proof for successful Route 4 semantic publication identity use.
- Prove absent facts, mismatches, duplicates, ambiguous facts,
  wrong-reference fallback, and retained prepared fallback behavior.
- Preserve x86/riscv wrapper or route-oracle output when affected.
- Do not use expected-string rewrites, unsupported downgrades, weakened guards,
  or named-case shortcuts as proof.

Completion check:

- The delegated proof subset is green and covers success, absence, mismatch,
  duplicate or ambiguity, wrong-reference fallback, prepared fallback, and
  output stability where relevant.

## Step 5: Broader Validation And Handoff

Goal: decide whether the first Route 4 adapter slice is ready for review,
follow-up expansion, or closure of this source idea.

Actions:

- Run the supervisor-delegated broader check if the adapter touches shared BIR
  lookup behavior, wrappers, route-oracle output, or emitted/debug output.
- Update `todo.md` with the exact retained prepared publication mechanics and
  output fallback that remain after the adapter lands.
- Record whether additional Route 4 readers are intentionally out of scope for
  this idea or need a new open idea.

Completion check:

- The implementation and proof satisfy
  `ideas/open/203_route4_publication_identity_adapter.md`, with no broad
  helper replacement, no publication mechanics migration into BIR, and no
  testcase-overfit evidence.
