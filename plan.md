# Phase E3 Route 5 Current-Block Join-Source Helper-Oracle Follow-Up

Status: Active
Source Idea: ideas/open/231_phase_e3_route5_current_block_join_source_helper_oracle_follow_up.md

## Purpose

Activate the Route 5 follow-up for one
`PreparedCurrentBlockJoinParallelCopySourceFact` current-block join-source
helper-oracle success row while preserving prepared printer, wrapper,
helper-oracle string, expected-string, and fallback authority.

## Goal

Make the selected helper-oracle row use Route 5 current-block join-source
diagnostic metadata only after agreement with prepared edge/join semantics, and
fail closed to existing prepared behavior for all missing, invalid, conflicting,
or disagreeing route evidence.

## Core Rule

Route 5 may annotate the selected current-block join-source helper-oracle row
only when the prepared row is available and the Route 5 join-source record
agrees with the prepared fact. Prepared edge/join, branch, parallel-copy,
printer, wrapper, and expected-string behavior remain authoritative everywhere
else.

## Read First

- The source idea named in the metadata above.
- `ideas/closed/211_route5_current_block_join_source_semantic_reader.md`
- `ideas/closed/212_route5_edge_join_oracle_printer_row.md`
- `docs/bir_prealloc_fusion/phase_e3_prepared_diagnostic_oracle_replacement_readiness.md`
- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/mir/query.cpp`
- `src/backend/mir/query.hpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
- `tests/backend/bir/backend_prepared_printer_test.cpp`
- `tests/backend/bir/backend_prealloc_block_entry_publications_test.cpp`
- `tests/backend/mir/backend_aarch64_current_block_join_routing_test.cpp`
- wrapper or joined-branch tests that expose the same prepared current-block
  join-source behavior

## Current Targets

- One `PreparedCurrentBlockJoinParallelCopySourceFact` current-block
  join-source helper-oracle success row.
- Route 5 metadata from `Route5CurrentBlockJoinSourceRecord` only after
  prepared edge/join agreement.
- Fallback cases for absent, invalid, duplicate/conflicting, memory-source,
  mismatch, unsupported, branch/parallel-copy, and prepared-only paths.
- No-change proof for prepared-printer output, wrapper output,
  helper-oracle strings, expected strings, and nearby same-feature cases.

## Non-Goals

- Do not migrate the adjacent prepared-printer row.
- Do not replace broad edge-publication, source-producer, move-bundle,
  branch, parallel-copy, wrapper, or prepared-printer ownership.
- Do not change expected strings, refresh baselines, rename helpers, downgrade
  supported paths, or weaken helper-oracle fallback behavior.
- Do not treat Route 5 join-source identity as ownership of scheduling,
  source/destination homes, move bundles, branch spelling, final edge-copy
  records, or emitted-output policy.
- Do not start draft 155, E5, aggregate prepared retirement, or broad
  prepared diagnostic/oracle replacement.

## Working Model

- `PreparedCurrentBlockJoinParallelCopySourceFact` already carries the prepared
  edge-copy source fact state and is the only eligible helper-oracle fact type
  for this plan.
- Route 5 agreement is expected around
  `attach_route5_current_block_join_source_if_agrees(...)`,
  `route5_join_source_record_agrees_with_prepared_fact(...)`, and
  `route5_find_current_block_join_source(...)`.
- The accepted row should consume agreement metadata as diagnostic evidence,
  not create a parallel output policy or bypass prepared fallback.
- Existing Route 5 semantic-reader work and edge/join oracle closure notes are
  context only; this plan is still limited to the one helper-oracle row named
  by the source idea.

## Execution Rules

- Start by identifying the exact helper-oracle row and the existing positive
  and fallback tests before editing implementation.
- Keep prepared behavior authoritative for absent route evidence, invalid
  references, duplicate or conflicting records, memory-source status, source or
  destination mismatch, unsupported moves, branch/parallel-copy paths, and
  prepared-only data.
- Prefer consuming existing Route 5 agreement metadata over inventing a new
  diagnostic channel.
- Do not add testcase-shaped matching, fixture-name shortcuts, or expectation
  rewrites to claim progress.
- Every code-changing step needs fresh build or compile proof plus the
  supervisor-delegated test subset. Broader validation is required before
  acceptance if the implementation touches shared helper-oracle, prepared
  printer, wrapper, MIR query, or publication planning behavior beyond the
  selected row.

## Steps

### Step 1: Locate The Row And Agreement Boundary

Goal: identify the selected helper-oracle success row, the available Route 5
agreement metadata, and the tests that currently prove positive and fallback
behavior.

Primary targets:

- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/mir/query.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
- `tests/backend/bir/backend_prepared_printer_test.cpp`

Actions:

- Inspect the `PreparedCurrentBlockJoinParallelCopySourceFact` helper-oracle
  path and name the exact success row eligible for Route 5 diagnostic metadata.
- Trace the Route 5 agreement boundary through
  `attach_route5_current_block_join_source_if_agrees(...)`,
  `route5_join_source_record_agrees_with_prepared_fact(...)`, and the indexed
  Route 5 join-source lookup.
- Inventory existing positive, absent, invalid, duplicate/conflicting,
  memory-source, mismatch, unsupported, branch/parallel-copy, and
  prepared-only fallback tests.
- Record the intended Step 2 target files and proof gaps in `todo.md` before
  implementation edits.

Completion check:

- `todo.md` names the exact helper-oracle row, the agreement boundary, the
  minimal implementation targets, and the fallback/proof gaps for Step 2.
- No code, expected-string, baseline, prepared-printer, wrapper, branch, or
  parallel-copy behavior change is made in this step unless the executor is
  explicitly delegated to start implementation.

### Step 2: Consume Route 5 Metadata Only Under Prepared Agreement

Goal: make the selected helper-oracle row report Route 5 current-block
join-source metadata only when prepared and Route 5 facts agree.

Primary targets:

- The helper-oracle row path found in Step 1
- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/prealloc/publication_plans.hpp`
- Existing prepared fallback helper behavior

Actions:

- Reuse existing Route 5 join-source status, record pointer, and agreement
  metadata where possible.
- Require an available prepared current-block join-source fact before Route 5
  metadata can be used.
- Fail closed to existing prepared authority for absent, invalid,
  duplicate/conflicting, memory-source, mismatch, unsupported,
  branch/parallel-copy, and prepared-only cases.
- Keep helper-oracle strings, row spelling, prepared-printer output, wrapper
  output, and expected strings byte-stable unless the supervisor explicitly
  approves a different contract.

Completion check:

- The selected positive row can be explained by Route 5 current-block
  join-source metadata after prepared agreement.
- All non-agreement paths retain existing prepared helper-oracle behavior.
- The slice builds and the delegated narrow proof passes without expectation
  or baseline rewrites.

### Step 3: Prove Fallback And Nearby Same-Feature Stability

Goal: prove the implementation is semantic and not shaped around one known
fixture.

Primary targets:

- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
- `tests/backend/bir/backend_prepared_printer_test.cpp`
- `tests/backend/bir/backend_prealloc_block_entry_publications_test.cpp`
- `tests/backend/mir/backend_aarch64_current_block_join_routing_test.cpp`
- Nearby wrapper or joined-branch tests selected by the supervisor

Actions:

- Add or tighten focused tests only where coverage is missing for the selected
  row and fallback matrix.
- Cover positive agreement plus absent route evidence, invalid reference,
  duplicate/conflict, memory-source, mismatch, unsupported move,
  branch/parallel-copy, and prepared-only fallback behavior.
- Check nearby same-feature current-block join-source cases so the change does
  not depend on one fixture shape.
- Preserve prepared-printer output, wrapper output, helper-oracle strings,
  expected strings, and supported/unsupported contracts.

Completion check:

- Focused tests or explicit proof cover the positive row and every required
  fallback path.
- Nearby same-feature coverage demonstrates this is not a named-case shortcut.
- No prepared-printer, wrapper, branch/parallel-copy, expected-string,
  baseline, or target-policy change is part of the diff.

### Step 4: Validate And Prepare Acceptance Notes

Goal: prove the completed helper-oracle row slice and leave clear handoff state
for supervisor review.

Primary targets:

- Build or compile target chosen by the supervisor
- Delegated narrow test subset
- Any broader validation the supervisor requests because of touched surfaces
- `todo.md`

Actions:

- Run the exact supervisor-delegated proof command and record the command and
  result in `todo.md`.
- If the change touched shared helper-oracle, prepared-printer, wrapper, MIR
  query, publication planning, branch, or parallel-copy behavior, ask the
  supervisor to choose broader validation before acceptance.
- Summarize the row changed, retained fallback authority, same-feature proof,
  and any residual risk.

Completion check:

- Fresh build or compile proof exists.
- Delegated narrow tests pass, and any supervisor-requested broader validation
  passes.
- `todo.md` records the implementation summary, proof commands, retained
  authority, and residual risks.
