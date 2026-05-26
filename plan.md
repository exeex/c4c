# AArch64 Calls File Consolidation Runbook

Status: Active
Source Idea: ideas/open/11_aarch64_calls_file_consolidation.md

## Purpose

Consolidate the remaining AArch64 calls file family now that call source
selection, preservation republication, and printing/effect ownership have been
moved behind clearer owner surfaces.

## Goal

The AArch64 calls code is materially smaller and easier to scan, build metadata
has no stale calls entries, and behavior is unchanged.

## Core Rule

This is behavior-preserving file consolidation. Do not move new semantic
authority, change ABI classification, alter dispatch behavior, weaken tests, or
claim progress from line-count reduction alone.

## Read First

- `ideas/open/11_aarch64_calls_file_consolidation.md`
- `src/backend/mir/aarch64/codegen/calls.hpp`
- AArch64 calls translation units under `src/backend/mir/aarch64/codegen/`
- Build metadata that lists AArch64 calls sources
- Recent closed calls ownership ideas for boundary context

## Current Targets

- Remaining small AArch64 calls files whose responsibilities are already
  emission-only or thin target adapters.
- `calls.hpp` declarations that can stay minimal and target-emission oriented.
- Build metadata, includes, and stale translation-unit entries affected by file
  moves or deletions.
- Focused call/backend tests that prove consolidation preserved behavior.

## Non-Goals

- Do not move new semantic authority during consolidation.
- Do not perform dispatch cleanup; dispatch value materialization belongs to
  idea `12`.
- Do not rewrite ABI classification, call-plan preparation, or preservation
  source selection.
- Do not weaken diagnostics, assembly-output coverage, backend expectations, or
  c_testsuite contracts.

## Working Model

- Prepared facts, preservation/republication authority, and printing/effect
  ownership have already been separated enough for calls files to be merged
  safely.
- Consolidation should merge only files whose remaining responsibilities are
  compatible and easy to prove behavior-preserving.
- `calls.hpp` should expose the minimal target-emission contract needed by
  clients; private helpers should move into implementation files when possible.
- Any diff that changes lowering behavior is outside this idea unless first
  split into a separate authority-cleanup initiative.

## Execution Rules

- Audit before merging: classify each calls file by current owner role and
  dependency direction.
- Keep each file move/deletion small enough to pair with build proof and a
  focused call/backend test subset.
- Update includes, declarations, and build metadata in the same packet as the
  file consolidation that requires them.
- Preserve generated machine instructions, diagnostics, and assembly output.
- Treat expectation rewrites, unsupported downgrades, and named-test shortcuts
  as route failures.
- Escalate to broader call/backend validation before treating the consolidation
  as complete.

## Ordered Steps

### Step 1: Audit Remaining Calls File Roles

Goal: identify which AArch64 calls files are safe consolidation candidates and
which surfaces must remain separate.

Primary targets:

- `src/backend/mir/aarch64/codegen/calls*.cpp`
- `src/backend/mir/aarch64/codegen/calls*.hpp`
- `src/backend/mir/aarch64/codegen/calls.hpp`
- build metadata that lists AArch64 calls translation units

Actions:

- List each remaining calls file, its public declarations, and its active
  clients.
- Classify each file as emission-only, target adapter, shared helper, semantic
  lowering, or dispatch bridge.
- Identify small files that can merge without changing owner boundaries.
- Identify declarations in `calls.hpp` that should stay public, become private,
  or move with their implementation owner.
- Record the focused proof set needed for each proposed merge/deletion.

Completion check:

- The executor can name the consolidation candidates, files that must not be
  merged in this idea, declarations to keep or hide, and the tests that prove
  behavior preservation.

### Step 2: Merge Safe Emission-Only Calls Helpers

Goal: merge the smallest emission-only calls helpers into their natural
implementation owner without changing behavior.

Primary targets:

- calls helper translation units classified as emission-only in Step 1
- affected private declarations and includes
- build metadata for removed translation units

Actions:

- Move compatible helper definitions into the selected implementation owner.
- Make helpers file-local where no external client requires them.
- Remove stale source entries, includes, and declarations in the same packet.
- Run build proof and focused call/backend tests selected by the supervisor.

Completion check:

- The merged helpers have one clear emission owner, removed files no longer
  appear in build metadata, and focused proof shows unchanged behavior.

### Step 3: Simplify `calls.hpp`

Goal: keep `calls.hpp` minimal and target-emission oriented after helper
consolidation.

Primary targets:

- `src/backend/mir/aarch64/codegen/calls.hpp`
- clients that include `calls.hpp` only for retired helper declarations
- private helper declarations that can move into implementation files

Actions:

- Remove stale or private declarations from `calls.hpp`.
- Redirect clients to narrower headers only when an existing narrower owner
  already exists.
- Avoid creating a new broad calls umbrella while simplifying includes.
- Rebuild and run the focused tests that cover affected public calls APIs.

Completion check:

- `calls.hpp` exposes only active target-emission APIs needed by clients, and
  no stale includes or declarations remain.

### Step 4: Retire Remaining Stale Calls Build Entries

Goal: finish deletion and metadata cleanup for retired calls files.

Primary targets:

- build metadata
- retired calls translation units and headers
- stale include references

Actions:

- Delete retired calls files that no longer own behavior.
- Remove all build entries for deleted translation units.
- Use repository search or build diagnostics to remove stale include paths and
  declarations.
- Keep any file that still owns semantic lowering, dispatch bridge behavior, or
  ABI classification instead of forcing consolidation.

Completion check:

- Build metadata has no stale calls entries, removed files have no remaining
  references, and files left behind have clear responsibilities.

### Step 5: Validate Calls Consolidation

Goal: prove the consolidation preserved behavior and did not hide semantic
changes inside file moves.

Primary targets:

- focused AArch64 call/backend tests
- machine-printer or target-instruction tests affected by calls output
- broader backend subset selected by the supervisor

Actions:

- Run build proof after the final file consolidation packet.
- Run focused call/backend tests that cover call lowering, call-boundary moves,
  preservation/republication, and target instruction output.
- Escalate to the supervisor-selected broader backend validation before closure.
- Review the diff for behavior changes, stale metadata, expectation rewrites,
  and hidden authority moves.

Completion check:

- Build proof and focused/broader validation pass, no behavior-changing diff is
  mixed into consolidation, and the calls file family is materially smaller and
  easier to scan.
