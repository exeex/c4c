# Shared Current-Block Entry Publication Query

Status: Active
Source Idea: ideas/open/135_shared_current_block_entry_publication_query.md

## Purpose

Clarify the shared prepared/prealloc query for current-block entry publication
facts that AArch64 publication and producer code currently wraps locally.

## Goal

Replace one actual AArch64 current-block publication or value-home fact
rediscovery site with a target-neutral shared query while keeping AArch64
register-view selection and emission policy local.

## Core Rule

Do not move AArch64 publication register recording, branch-fusion hooks,
relocation operands, register-view selection, or final instruction spelling
into shared code.

## Read First

- `ideas/open/135_shared_current_block_entry_publication_query.md`
- `src/backend/mir/aarch64/codegen/dispatch_publication.hpp`
- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`
- The selected shared prepared or prealloc query owner files found during
  inspection

## Current Targets

- Current-block entry publication collection
- Prepared value-home register lookup
- The shared prepared/prealloc query surface that owns the target-neutral
  current-block facts

## Non-Goals

- Broad publication rewrites
- File-count-only cleanup
- Replacing prepared facts with local scans
- Moving AArch64-specific register or emission decisions into shared code
- Weakening test expectations or marking supported current-block publication
  paths unsupported

## Working Model

- Shared code should answer target-neutral questions about prepared current-block
  entry publication and prepared value-home facts.
- AArch64 code should consume those facts, choose the register view, record
  publication registers, preserve branch-fusion behavior, and spell final
  operands/instructions locally.
- Progress must be measured by replacing a concrete target-local rediscovery
  site, not by adding a renamed wrapper around the same local scan.

## Execution Rules

- Inspect before editing; identify the exact rediscovery site and the owner
  semantics it should query.
- Prefer the narrowest shared query that matches existing prepared/prealloc
  ownership boundaries.
- Keep AArch64 wrappers thin consumers only when they still add target-local
  register or emission policy.
- Do not introduce BIR-name matching or testcase-shaped shortcuts.
- Use matching `test_before.log` and `test_after.log` for acceptance because
  shared prepared or prealloc query code is in scope.

## Step 1: Locate the Rediscovery Site

Goal: identify the concrete AArch64 wrapper or local scan that should be
replaced by a shared query.

Primary target:

- `src/backend/mir/aarch64/codegen/dispatch_publication.*`
- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`

Actions:

- Trace current-block entry publication collection and prepared value-home
  register lookup call paths.
- Identify which data already exists in prepared current-block or value-home
  facts.
- Select one concrete rediscovery site whose target-neutral part can move to a
  shared prepared/prealloc query.
- Record any AArch64-only policy that must stay local.

Completion check:

- `todo.md` names the selected rediscovery site, the proposed shared owner, and
  the AArch64 behavior that must remain local.

## Step 2: Add the Shared Query

Goal: expose the selected target-neutral fact through the correct shared
prepared or prealloc query owner.

Primary target:

- The shared prepared/prealloc query files selected in Step 1

Actions:

- Add or extend a query with target-neutral naming and inputs.
- Reuse existing prepared/prealloc data structures instead of rescanning local
  AArch64 state.
- Keep return values free of AArch64 register-view or emission policy.
- Add focused unit or backend coverage if an existing test bucket directly
  exercises the query surface.

Completion check:

- Shared code provides the needed current-block entry publication or value-home
  fact without depending on AArch64 codegen details.
- `cmake --build --preset default` succeeds for the changed shared surface.

## Step 3: Replace the AArch64 Rediscovery Path

Goal: switch the selected AArch64 site to consume the shared query.

Primary target:

- The AArch64 file that owns the selected rediscovery site

Actions:

- Replace the target-local rediscovery logic with the shared query result.
- Preserve local publication register recording, branch-fusion hooks,
  relocation operands, and final operand/register spelling.
- Keep any remaining AArch64 wrapper limited to target-local view conversion or
  emission policy.
- Remove obsolete local helper code only when it is no longer referenced.

Completion check:

- The concrete rediscovery site no longer performs the old local scan or
  wrapper-only lookup.
- Nearby AArch64 current-block publication paths still route through their
  existing local emission policy.

## Step 4: Prove the Backend Path

Goal: validate that the shared query and AArch64 consumer preserve behavior.

Actions:

- Capture matching baseline and after logs with the same command when the
  supervisor delegates regression proof.
- Run:
  - `cmake --build --preset default`
  - `ctest --test-dir build -R '^backend_' --output-on-failure`
- If the selected site has a narrower backend test bucket, run it first, then
  keep the broader backend proof for acceptance.

Completion check:

- `test_before.log` and `test_after.log` cover matching backend commands.
- The build and backend tests pass without expectation downgrades.

## Step 5: Final Route Review

Goal: confirm the slice satisfies the source idea instead of only reshaping
files.

Actions:

- Verify the generic query replaced an actual AArch64 rediscovery site.
- Verify AArch64-specific publication and emission behavior stayed local.
- Check for retained local scans, BIR-name matching, testcase-shaped shortcuts,
  or expectation weakening.
- If the runbook is exhausted but the source idea is not satisfied, ask the
  plan owner to rewrite or split the active route instead of closing it.

Completion check:

- The active route meets the source idea acceptance criteria and has fresh
  backend proof, or the remaining blocker is recorded for lifecycle review.
