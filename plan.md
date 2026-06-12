# Phase E3 Route 4 Block-Entry Publication Printer/Debug Follow-Up

Status: Active
Source Idea: ideas/open/230_phase_e3_route4_block_entry_publication_printer_debug_follow_up.md

## Purpose

Activate the Route 4 follow-up for one `block_entry_publication`
available-register printer/debug row while preserving prepared publication
mechanics, wrapper behavior, CLI dump scope, row text, and expected strings.

## Goal

Make the selected available-register row depend on Route 4 publication
attribution only after prepared agreement, and fail closed to prepared
authority for all missing or disagreeing route evidence.

## Core Rule

Route 4 may attribute the selected row only when the prepared
block-entry publication is already available and the Route 4 publication
reference agrees. Prepared publication remains the authority for output,
fallback, wrapper compatibility, block order, and row spelling.

## Read First

- The source idea named in the metadata above.
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/value_locations.hpp`
- `src/backend/prealloc/prepared_printer/value_locations.cpp`
- `tests/backend/bir/backend_prealloc_block_entry_publications_test.cpp`
- `tests/backend/bir/backend_prepared_printer_test.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
- `tests/backend/bir/CMakeLists.txt`
- x86/riscv/AArch64 wrapper tests that expose the same prepared
  `block_entry_publication` row

## Current Targets

- One `block_entry_publication` available-register printer/debug row.
- Route 4 block-entry publication attribution under prepared agreement.
- Negative paths for absent route evidence, wrong reference, duplicate
  reference, mismatch, and prepared-only fallback.
- Wrapper no-change proof where the row is visible.

## Non-Goals

- Do not migrate broad printer/debug, CLI dump, block-order, wrapper,
  publication mechanics, or prepared publication ownership.
- Do not relabel row text, change expected strings, refresh baselines, or
  weaken supported/unsupported contracts.
- Do not use testcase-shaped matching or named fixture shortcuts.
- Do not move emitted-output or target-wrapper policy into Route 4 diagnostic
  ownership.
- Do not start E4 wrapper migration, draft 155, E5, aggregate prepared
  retirement, or broad diagnostic/oracle replacement.

## Working Model

- Existing prepared lookup attribution lives around
  `attribute_route4_block_entry_publication_if_agreeing(...)` and
  `find_prepared_current_block_entry_publication(...)`.
- Existing prepared printer output for block-entry publications lives in
  `src/backend/prealloc/prepared_printer/value_locations.cpp`.
- Existing fail-closed Route 4 and prepared fallback coverage lives in
  `backend_prealloc_block_entry_publications_test` and
  `backend_prepared_lookup_helper_test`.
- The accepted implementation should connect the selected printer/debug row to
  the same agreement boundary, not create a parallel output policy.

## Execution Rules

- Keep each code slice behavior-preserving outside the selected row.
- Prefer threading existing Route 4 attribution metadata through the prepared
  row path over creating a new diagnostic naming scheme.
- Every positive attribution path must have a paired negative/fallback case.
- Preserve byte-stable wrapper output, CLI dump sections, row spelling, and
  expected strings unless the supervisor explicitly approves a different
  contract.
- Use the supervisor-delegated build/test command for executor proof. Broader
  validation is required before acceptance if the implementation touches shared
  printer/debug, wrapper, or lookup behavior beyond the named row.

## Steps

### Step 1: Locate the Exact Row and Agreement Boundary

Goal: identify the single available-register row and the existing prepared
agreement facts it can use.

Primary targets:

- `src/backend/prealloc/prepared_printer/value_locations.cpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/value_locations.hpp`
- `tests/backend/bir/backend_prepared_printer_test.cpp`
- `tests/backend/bir/backend_prealloc_block_entry_publications_test.cpp`

Actions:

- Inspect the prepared printer row emitted for
  `block_entry_publication successor=... status=available ... destination_storage=register`.
- Trace whether the row can consume existing
  `route4_block_entry_publication_*` attribution from
  `PreparedCurrentBlockEntryPublication` or needs a narrow equivalent metadata
  path.
- Identify the smallest code surface that can expose attribution to the row
  without changing row text, section order, CLI dump scope, or wrapper output.
- Record the intended positive and negative proof cases in `todo.md` before
  making implementation edits.

Completion check:

- `todo.md` names the exact row, the chosen attribution boundary, and the
  minimal target files for Step 2.

### Step 2: Thread Route 4 Attribution Under Prepared Agreement

Goal: make the selected row use Route 4 publication attribution only when the
prepared row and Route 4 evidence agree.

Primary targets:

- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/value_locations.hpp`
- `src/backend/prealloc/prepared_printer/value_locations.cpp`

Actions:

- Reuse the existing Route 4 block-entry publication reference validation
  status and route availability status where possible.
- Require the prepared publication to be available before any Route 4
  attribution is considered.
- Fail closed to prepared authority for absent route evidence, wrong
  relationship, stale or mismatched reference, duplicate reference, and
  prepared-only cases.
- Keep row spelling, printer/debug section names, CLI dump scope, and wrapper
  output byte-stable.

Completion check:

- Positive attribution is observable through the selected row path only after
  prepared agreement, and every disagreeing path preserves prepared behavior.

### Step 3: Prove Negative Cases and Nearby Same-Feature Stability

Goal: cover the agreement and fallback matrix without narrowing proof to one
fixture.

Primary targets:

- `tests/backend/bir/backend_prealloc_block_entry_publications_test.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
- `tests/backend/bir/backend_prepared_printer_test.cpp`
- `tests/backend/bir/CMakeLists.txt`

Actions:

- Add or tighten tests for positive Route 4 attribution under prepared
  agreement.
- Cover absent route evidence, wrong-reference, duplicate-reference, mismatch,
  and prepared-only fallback.
- Check nearby same-feature block-entry publication rows so the change is not
  fixture-shaped.
- Preserve existing expected strings and CLI required snippets.

Completion check:

- Narrow tests prove positive attribution plus all negative fallback cases, and
  no expected-string or baseline rewrite is part of the diff.

### Step 4: Wrapper and Broader No-Change Proof

Goal: demonstrate the row change did not move wrapper or emitted-output policy.

Primary targets:

- x86, riscv, and AArch64 tests that expose prepared block-entry publication
  rows or wrapper compatibility.
- Existing backend BIR prepared printer and CLI dump tests.

Actions:

- Run the supervisor-selected proof command for the implementation slice.
- Include wrapper-visible tests for x86/riscv/AArch64 when the row is exposed
  there.
- Escalate to broader backend validation if Step 2 touched shared printer or
  lookup paths outside the selected row.
- Record the exact proof commands and results in `todo.md`.

Completion check:

- Fresh proof shows byte-stable wrapper output, CLI dump scope, row text, and
  expected strings across the selected row and nearby same-feature coverage.
