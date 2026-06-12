# Route 4 Block-Entry Publication Acceptance Review

Active source idea: `ideas/open/210_route4_block_entry_publication_printer_debug_row.md`

Chosen base commit: `c0afe6407` (`[plan] Activate Route 4 publication printer row plan`)

Base rationale: this commit creates the active `plan.md`/`todo.md` for idea
210 and is the lifecycle activation checkpoint immediately before the row
selection and implementation commits. The later commits are the complete active
idea slice under review.

Commits since base: 4

## Findings

1. Severity: Medium - acceptance proof is short of the source idea's x86/riscv
   wrapper wording.

   The source idea asks for "No-change checks for x86/riscv wrapper output" and
   its acceptance criteria repeat that wrapper proof should check x86/riscv
   output without migrating wrappers (`ideas/open/210_route4_block_entry_publication_printer_debug_row.md:22`,
   `ideas/open/210_route4_block_entry_publication_printer_debug_row.md:42`).
   The recorded proof in `todo.md` ran
   `backend_prealloc_block_entry_publications`,
   `backend_prepared_printer`, and `backend_x86_publication_plan_reuse` only
   (`todo.md:43`). The test suite has an available riscv publication target,
   `backend_riscv_prepared_edge_publication`
   (`tests/backend/bir/CMakeLists.txt:365`).

   This is not route drift and does not invalidate the implementation path, but
   supervisor-side acceptance should either run the riscv publication target as
   part of the final proof or explicitly record why no riscv wrapper output
   surface is relevant to this selected block-entry row. The local
   `test_after.log` was not present during review, so the final acceptance
   pass should also recreate the canonical proof log before regression-guard
   handling.

## Non-Findings

- The implementation stays on one selected available-register
  `block_entry_publication` row. Route 4 attribution is added only to
  `PreparedCurrentBlockEntryPublicationQueryInputs` /
  `PreparedCurrentBlockEntryPublication` and the agreement helper in
  `prepared_lookups.cpp` (`src/backend/prealloc/value_locations.hpp:246`,
  `src/backend/prealloc/prepared_lookups.cpp:1821`).
- The helper fails closed before attribution unless prepared availability,
  Route 4 reference validation, successor label, destination value/name/type,
  and instruction index agree (`src/backend/prealloc/prepared_lookups.cpp:1824`,
  `src/backend/prealloc/prepared_lookups.cpp:1858`).
- The focused test covers positive attribution, absent route evidence,
  route/prepared mismatch, wrong successor, wrong key, and duplicate reference
  fallback without weakening existing expectations
  (`tests/backend/bir/backend_prealloc_block_entry_publications_test.cpp:489`,
  `tests/backend/bir/backend_prealloc_block_entry_publications_test.cpp:546`,
  `tests/backend/bir/backend_prealloc_block_entry_publications_test.cpp:590`,
  `tests/backend/bir/backend_prealloc_block_entry_publications_test.cpp:616`,
  `tests/backend/bir/backend_prealloc_block_entry_publications_test.cpp:643`,
  `tests/backend/bir/backend_prealloc_block_entry_publications_test.cpp:677`).
- The prepared-printer row text is frozen byte-for-byte without adding an
  attribution column or rewriting an existing expected string
  (`tests/backend/bir/backend_prepared_printer_test.cpp:4890`).
- I did not find wrapper-family migration, CLI dump migration, broad
  printer/debug contraction, prepared API deletion, target policy movement,
  unsupported downgrades, baseline refreshes, or testcase-shaped emission
  shortcuts.

## Judgments

Idea alignment: `matches source idea`

Runbook transcription: `plan matches idea`

Route alignment: `on track`

Technical debt: `acceptable`

Validation sufficiency: `needs broader proof`

Reviewer recommendation: `continue current route`

Recommended supervisor follow-up: run an acceptance proof that includes the
recorded three-test subset plus the available riscv publication proof, or write
the reason the riscv target is not applicable to this selected row, then
recreate `test_after.log` before final regression-guard handling.
