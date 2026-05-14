# Backend Test Tree BIR/MIR Split Runbook

Status: Active
Source Idea: ideas/open/220_backend_test_tree_split_bir_mir_and_prune_legacy.md

## Purpose

Restructure `tests/backend` so backend tests live under the compiler layer they
prove, with stale legacy-route tests deleted instead of kept as ambiguous
coverage.

## Goal

Split backend tests into BIR-owned and MIR-owned trees, classify shared fixtures
and legacy corpus entries, and make CMake labels/selectors run `bir` and `mir`
subsets independently without changing backend capability.

## Core Rule

Test location and labels must describe the assertion target. BIR text,
prepared-BIR behavior, and semantic backend-route observations belong under
`tests/backend/bir`; target MIR records, machine nodes, machine printers, and
AArch64 `.c -> .s` smoke proof belong under `tests/backend/mir`.

## Read First

- `ideas/open/220_backend_test_tree_split_bir_mir_and_prune_legacy.md`
- `tests/backend/`
- backend test CMake wiring that defines `backend_*` tests, labels, helper
  functions, and fixture paths
- current backend case corpus under `tests/backend/case`
- disabled MIR dump or trace shell-test placeholders, if still present
- AArch64 markdown-driven backend case documentation from the active repo state

## Current Targets And Scope

- `tests/backend/bir/` for BIR, prepared-BIR, BIR printer/dump, and semantic
  route tests whose assertion target is BIR or prepared BIR.
- `tests/backend/mir/` for AArch64 MIR model, machine-node, machine-printer,
  target-specific smoke, and future markdown-driven MIR case tests.
- Shared `.c` fixtures only when they are explicitly documented as input data,
  not proof ownership.
- Backend CMake helpers, labels, and source paths that currently assume one
  flat backend test namespace.
- Inventory documentation that classifies backend tests as `bir-live`,
  `mir-live`, `shared-fixture`, `legacy`, `deprecated`, or `delete`.

## Non-Goals

- Do not implement new AArch64 backend capability to justify moving a test.
- Do not weaken expectations, mark supported cases unsupported, or preserve
  narrow fixture checks as fake progress.
- Do not treat external `.s` or `.ll` as backend input.
- Do not replace the terminal `.s` printer or implement assembler, object, or
  linker stages.
- Do not flatten x86 or riscv semantic route tests into MIR unless they prove
  target MIR behavior.
- Do not enable new AArch64 case coverage ahead of the markdown owner required
  by the source idea.

## Working Model

Execution should move from audit to mechanical relocation to pruning:

1. inventory and classify current backend tests and fixtures;
2. create the new BIR/MIR tree shape and move BIR-owned tests;
3. move MIR-owned tests and preserve AArch64 smoke coverage;
4. classify shared fixtures and delete stale legacy placeholders;
5. update labels and selectors so BIR and MIR subsets run independently;
6. validate the moved tree and close only when the source idea acceptance
   criteria are satisfied.

Each implementation packet should preserve existing live behavior unless it is
deleting a documented stale legacy-route test.

## Execution Rules

- Keep routine packet progress in `todo.md`.
- Preserve source intent in the idea file; do not edit it for ordinary
  execution notes.
- Prefer mechanical moves plus explicit inventory over expectation rewrites.
- For code-changing steps, run `cmake --build --preset default` or the
  repo-equivalent build command before narrow proof.
- After moving tests, run the narrow backend subset selected by the supervisor
  and record exact commands and results in `todo.md`.
- Treat deletion of live contracts, unsupported downgrades, and testcase-shaped
  rewrites as route drift.
- Keep CMake labels sufficient for independent `bir` and `mir` selection.

## Steps

### Step 1: Inventory Backend Test Ownership

Goal: establish the current test ownership map before moving files.

Primary targets:
- `tests/backend/`
- backend test CMake files
- current backend case corpus

Actions:
- List current backend tests, helper functions, labels, source paths, and
  disabled shell placeholders.
- Classify each live test or fixture as `bir-live`, `mir-live`,
  `shared-fixture`, `legacy`, `deprecated`, or `delete`.
- Confirm the two live AArch64 MIR case files named by the source idea remain
  MIR-owned proof.
- Identify the first mechanical move packet and the exact narrow proof command
  the supervisor should delegate.

Completion check:
- `todo.md` records the ownership inventory, delete/deprecated candidates with
  reasons, and the first implementation packet.
- Search or listing proof is recorded in `test_after.log` or the
  supervisor-delegated proof log if required.

### Step 2: Create BIR Test Tree And Move BIR-Owned Tests

Goal: make BIR/prepared-BIR ownership explicit without changing behavior.

Primary targets:
- `tests/backend/bir/`
- current `backend_prepare_*`, `backend_prepared_printer`,
  `backend_lir_to_bir_*`, `backend_cli_dump_bir*`, and
  `backend_cli_dump_prepared_bir*` tests
- x86/riscv semantic route tests whose assertions observe BIR or prepared BIR
- backend test CMake wiring

Actions:
- Create `tests/backend/bir/` and move BIR-owned unit, CLI dump, printer, and
  semantic route tests there.
- Update source paths, helper functions, labels, and test names only as needed
  to match BIR ownership.
- Keep assertion content behavior-preserving except for path/name updates.
- Record any temporarily shared fixtures as fixture inputs rather than BIR
  proof ownership.

Completion check:
- BIR-owned tests build and run from `tests/backend/bir`.
- CMake can select the BIR subset independently.
- Build plus narrow BIR proof is recorded.

### Step 3: Create MIR Test Tree And Move MIR-Owned Tests

Goal: make target MIR and AArch64 machine proof explicit without enabling new
backend capability.

Primary targets:
- `tests/backend/mir/`
- current `backend_aarch64_*` unit/model tests
- AArch64 machine node and machine printer tests
- live AArch64 `.c -> .s` smoke tests
- backend test CMake wiring

Actions:
- Create `tests/backend/mir/` and move MIR-owned AArch64 model, record, machine
  node, printer, and target-specific smoke tests there.
- Preserve the live AArch64 external `.s` smoke route from earlier backend
  work as MIR-owned proof.
- Keep x86/riscv semantic route tests out of MIR unless they actually prove
  target MIR behavior.
- Do not add new markdown-driven AArch64 case coverage without the owning
  markdown route.

Completion check:
- MIR-owned tests build and run from `tests/backend/mir`.
- CMake can select the MIR subset independently.
- Build plus narrow MIR proof is recorded.

### Step 4: Classify Fixtures And Prune Legacy Route Debris

Goal: prevent source corpus inventory and disabled placeholders from being
mistaken for active backend coverage.

Primary targets:
- `tests/backend/case`
- backend fixture documentation or inventory file
- disabled `backend_cli_dump_mir_*` and `backend_cli_trace_mir_*` placeholders,
  if present

Actions:
- Add or update inventory documentation that classifies current backend cases
  and fixtures as `bir-live`, `mir-live`, `shared-fixture`, `legacy`,
  `deprecated`, or `delete`.
- Convert shared `.c` files into explicit fixtures only when they are input data
  for live tests.
- Delete invalid or disabled MIR placeholders whose only purpose is preserving
  a past failed route.
- Record the deletion reason for each removed test in the inventory or commit
  message.

Completion check:
- Backend case corpus classification is visible and auditable.
- Disabled or misleading legacy placeholders are either active MIR contracts or
  deleted.
- No live contract is hidden by deletion.
- Build plus relevant narrow proof is recorded.

### Step 5: Normalize CMake Labels, Names, And Selectors

Goal: make backend test selection match the BIR/MIR tree split.

Primary targets:
- backend CMake test definitions
- test helper functions and label assignments
- CTest selector documentation, if present

Actions:
- Ensure moved tests use labels or names that allow independent BIR and MIR
  subset selection.
- Keep labels aligned with assertion target, not fixture origin.
- Remove stale references to old flat paths.
- Update narrow proof commands in `todo.md` so future executors and supervisor
  validation can run the intended subsets.

Completion check:
- `ctest` selectors can run BIR and MIR backend subsets separately.
- No CMake references point at removed legacy tests or old file paths.
- Build plus CTest label-selection proof is recorded.

### Step 6: Final Validation And Close Readiness

Goal: verify the tree split satisfies the source idea without drifting into
backend feature work.

Primary targets:
- BIR subset
- MIR subset
- backend case inventory
- `todo.md`

Actions:
- Re-run focused BIR and MIR subset proofs selected by the supervisor.
- Run broader backend or full CTest validation if CMake helpers or shared test
  infrastructure changed across multiple buckets.
- Confirm deleted tests were stale route debris, not live behavior.
- Confirm no new AArch64 case coverage was enabled ahead of its markdown owner.
- Summarize remaining shared fixtures, deprecated items, and close readiness in
  `todo.md`.

Completion check:
- Source idea acceptance criteria are satisfied.
- Regression guard can be run by the plan owner before closure for the chosen
  close scope.
