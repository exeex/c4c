# Prealloc Publication Accessor Contracts Runbook

Status: Active
Source Idea: ideas/open/prealloc-publication-accessor-contracts.md

## Purpose

Make the codegen-facing publication and prepared-accessor package in
`src/backend/prealloc` explicit without changing prepared-data semantics.

## Goal

Clarify the publication/accessor boundary through small contract, include,
naming, and comment cleanup while preserving backend behavior and prepared dump
meaning.

## Core Rule

Treat `publication_plans.*`, `prepared_lookups.*`,
`decoded_home_storage.*`, `storage_plans.*`, and `value_locations.hpp` as one
prepared-publication API package. Do not change publication semantics, decoded
home behavior, storage planning, value-location meaning, or codegen behavior.

## Read First

- `ideas/open/prealloc-publication-accessor-contracts.md`
- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/decoded_home_storage.hpp`
- `src/backend/prealloc/storage_plans.hpp`
- `src/backend/prealloc/value_locations.hpp`
- `src/backend/prealloc/prepared_printer/private.hpp`

## Current Targets

- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/decoded_home_storage.cpp`
- `src/backend/prealloc/decoded_home_storage.hpp`
- `src/backend/prealloc/storage_plans.cpp`
- `src/backend/prealloc/storage_plans.hpp`
- `src/backend/prealloc/value_locations.hpp`
- `src/backend/prealloc/prepared_printer/value_locations.cpp`
- `src/backend/prealloc/prepared_printer/storage.cpp`
- `src/backend/prealloc/prepared_printer/private.hpp`

## Non-Goals

- Do not split `module.hpp`, `regalloc.hpp`, or broad aggregate prealloc
  contracts.
- Do not change publication, storage, decoded-home, or value-location
  semantics.
- Do not introduce duplicate decode, home-selection, or storage-selection
  logic to justify a new package shape.
- Do not perform broad codegen include churn outside the target package.
- Do not make printer-only taxonomy changes unless they mirror real data-family
  naming or grouping changes.
- Do not rewrite tests, weaken expected behavior, or mark supported paths
  unsupported.

## Working Model

The publication/accessor package is the bridge from prepared facts to later
consumers. It should expose the facts codegen needs through deliberate
publication plans, prepared lookups, decoded home storage, storage plans, and
value locations. Prepared-printer files mirror those data families for debug
dumps; they do not define independent ownership boundaries.

Use this package model while inspecting files:

- publication plans: codegen-facing publication records and lookup-ready facts
- prepared lookups: stable accessor helpers over prepared module data
- decoded home storage: decoded value home and storage-source views
- storage plans: storage and memory-location publication records
- value locations: public value-location data contract
- prepared-printer mirrors: debug dump rendering for the same data families

## Execution Rules

- Start with an inventory of declarations, includes, comments, and call sites
  across the target files before editing.
- Prefer small contract contraction: remove stale comments, tighten includes,
  clarify names, or regroup helpers only where callers prove the boundary.
- Keep implementation changes behavior-preserving and localized to the target
  files unless a required caller adjustment is proven and recorded in
  `todo.md`.
- If a public name changes, update matching prepared-printer labels or helpers
  only when the data-family name actually changed.
- Use `rg` to confirm whether compatibility comments or bridge names still
  describe live call-site relationships before removing them.
- For code-changing packets, require:
  `bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"; } 2>&1 | tee test_after.log'`
- Always run `git diff --check` before handing a packet back.

## Step 1: Audit The Publication Accessor Surface

Goal: identify the current API surface, includes, comments, and prepared-printer
mirrors before changing names or contracts.

Primary targets:
- the target files listed above

Actions:
- List each public declaration in the target headers and assign it to the
  working-model family it serves.
- Record include dependencies that look broader than the declarations require.
- Find comments or helper names containing `bridge`, `compat`, `legacy`,
  `fallback`, `temporary`, old route names, or stale ownership wording.
- Map prepared-printer helpers and labels to the data-family declarations they
  mirror.
- Record the audit in `todo.md`; do not edit implementation files in this step.

Completion check:
- `todo.md` contains the declaration map, include observations, stale-wording
  candidates, and printer mirror map.
- No implementation or header files are changed.

## Step 2: Tighten Local Includes And Ownership Comments

Goal: make the package boundary easier to read without changing exported data
contracts.

Primary targets:
- target headers and matching `.cpp` files

Actions:
- Replace stale comments with current ownership descriptions only where Step 1
  proved the old wording is wrong.
- Remove unnecessary includes or replace them with narrower forward
  declarations only when the file still builds cleanly and local style supports
  the change.
- Keep comments focused on why the publication/accessor boundary exists, not
  on restating obvious field names.
- Record any candidate that is too risky for this step in `todo.md` instead of
  forcing it into the diff.

Completion check:
- The target files describe the publication/accessor package more clearly.
- Build plus backend tests pass with the delegated proof command.
- `todo.md` records changed files and any deferred include or comment
  candidates.

## Step 3: Contract Naming And Helper Grouping Pass

Goal: repair names or helper grouping that obscure the publication/accessor
family, limited to proven local contract cleanup.

Primary targets:
- `publication_plans.*`
- `prepared_lookups.*`
- `decoded_home_storage.*`
- `storage_plans.*`
- `value_locations.hpp`

Actions:
- Rename or regroup helpers only when call sites show the old name is stale or
  misleading for the current boundary.
- Keep compatibility comments or adapters when call sites still depend on the
  old aggregate prepared-module view.
- Avoid broad caller churn; if a rename requires widespread edits, stop and
  record it as a separate follow-up candidate.
- Update related declarations and local implementation together so the package
  remains coherent.

Completion check:
- Any naming or grouping diff is local, mechanical, and justified by Step 1
  evidence.
- Backend tests pass with the delegated proof command.
- `todo.md` explains why each public rename or grouping change was safe.

## Step 4: Align Prepared Printer Mirrors

Goal: keep prepared dump output and helper grouping aligned with real data
family changes from earlier steps.

Primary targets:
- `src/backend/prealloc/prepared_printer/value_locations.cpp`
- `src/backend/prealloc/prepared_printer/storage.cpp`
- `src/backend/prealloc/prepared_printer/private.hpp`

Actions:
- Update printer helper names, grouping, or labels only where Step 2 or Step 3
  changed the corresponding data-family contract.
- Preserve prepared dump meaning; do not invent printer-only ownership
  taxonomy.
- If no data-family naming changed, record that no printer edit was needed.

Completion check:
- Prepared-printer changes mirror actual data-family changes, or `todo.md`
  records that no printer alignment was required.
- Backend tests pass with the delegated proof command.

## Step 5: Final Boundary Review

Goal: decide whether the publication/accessor source idea is complete and ready
for lifecycle closure.

Actions:
- Review the final diff against the source idea acceptance criteria and
  reviewer reject signals.
- Confirm no publication, storage, decoded-home, value-location, or prepared
  dump semantics changed.
- Confirm no broad codegen include churn or aggregate contract split was
  performed.
- Record the final package-boundary summary, proof, and closure recommendation
  in `todo.md`.

Completion check:
- `todo.md` contains the final summary and proof status.
- The source idea can be closed without leaving important package-boundary
  decisions only in chat.
