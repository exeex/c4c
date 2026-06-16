# Reopen 286/288 Match Load Local-Memory Admission

Status: Active
Source Idea: ideas/open/292_reopen_286_288_match_load_local_memory_admission.md
Supersedes: exhausted runbook for ideas/open/291_retire_rendered_call_arg_abi_suffix_fallback.md pending close-time validation

## Purpose

Repair the reopened AArch64 286/288 CLI validation blocker that prevents the
rendered call-argument ABI suffix cleanup from closing.

## Goal

The 286/288 AArch64 CLI subset must lower through semantic BIR again without
using rendered call-argument text as semantic authority.

## Core Rule

Do not make progress by weakening CLI expectations, skipping supported paths,
matching named `00204.c` cases, or reintroducing rendered `alignstack(...)`
parsing into structured call-argument lowering.

## Read First

- `ideas/open/292_reopen_286_288_match_load_local_memory_admission.md`
- `ideas/open/291_retire_rendered_call_arg_abi_suffix_fallback.md`
- `ideas/closed/286_aarch64_00204_stdarg_semantic_bir_local_memory_admission.md`
- Current failure output from the 286/288 CLI subset
- Semantic BIR local-memory lowering/admission around the reported `match` load
  route

## Current Targets

- The exact red 286/288 CLI tests:
  - `backend_cli_dump_bir_00204_stdarg_movi_zext_immediate_fold`
  - `backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication`
- Semantic BIR `match` load local-memory admission.
- Any focused backend unit or dump fixture that can prove the repaired
  local-memory shape without relying on the `00204.c` name.
- Idea 291's fenced call-argument ABI suffix boundary.

## Non-Goals

- Do not rewrite broad AArch64 ABI policy.
- Do not retire prepared call-plan surfaces.
- Do not change expected dump text as the main evidence of progress.
- Do not mark supported 286/288 paths unsupported.
- Do not reopen rendered suffix parser cleanup except to prove it remains
  fenced.

## Working Model

The current blocker is separate from the completed suffix-parser cleanup. The
failing path should be admitted through semantic local-memory lowering using
structured route facts, not by recovering facts from rendered BIR or rendered
call-argument text.

## Execution Rules

- Start from a fresh reproduction log for the exact CLI subset.
- Keep any repair semantic and general to the local-memory load shape.
- Add focused proof before relying on the named CLI dump tests.
- If shared semantic BIR lowering changes, include the relevant x86 00204 dump
  coverage in proof.
- Preserve idea 291's structured call-argument metadata precedence.

## Steps

### Step 1: Reproduce and classify the reopened blocker

Goal: establish the exact current failure and confirm it is the separate
semantic BIR `match` load local-memory route.

Primary target: the two red 286/288 CLI tests.

Actions:

- Run the exact 286/288 CLI subset from the source idea acceptance criteria.
- Capture the failure output in `test_after.log`.
- Identify the failing function, semantic family, and local-memory load shape.
- Compare the failure against the closed 286 completion notes.
- Record whether the blocker is still external to idea 291's suffix-parser
  boundary.

Completion check:

- `todo.md` records the exact failing command, failing test names, failure
  family, and first implementation target.

### Step 2: Trace the semantic local-memory admission route

Goal: locate the structured fact or admission gap causing the `match` load
local-memory failure.

Primary target: semantic BIR local-memory lowering and admission code.

Actions:

- Trace the failing LIR/BIR shape through local-memory admission.
- Identify whether type, slot, pointer, aggregate-layout, or prepared metadata
  is missing, stale, or ignored.
- Decide the narrow semantic owner for the missing fact.
- Record any out-of-scope or adjacent follow-up work in `todo.md`.

Completion check:

- The repair target is a semantic lowering/admission rule, not a named-case
  shortcut or expectation change.

### Step 3: Repair the admitted local-memory shape

Goal: make the reopened `match` load local-memory shape lower through semantic
BIR using structured route facts.

Primary target: the narrow semantic BIR local-memory rule identified in Step 2.

Actions:

- Implement the smallest general semantic repair for the failing shape.
- Keep the rendered suffix fallback fence from idea 291 intact.
- Avoid changing CLI expected output unless the semantic output legitimately
  changes and is backed by focused proof.
- Run a build or compile proof for touched targets.

Completion check:

- The focused repaired shape no longer reports the old local-memory admission
  failure.
- No supported path is downgraded or skipped.

### Step 4: Add focused proof and restore CLI subset

Goal: prove the semantic repair independently and restore the 286/288 CLI
validation subset.

Primary target: focused backend proof plus the exact CLI tests.

Actions:

- Add or tighten a focused backend unit/dump assertion for the repaired
  local-memory shape.
- Run the exact 286/288 CLI subset.
- If shared semantic lowering changed, run the relevant x86 00204
  semantic/prepared dump coverage.
- Run a focused suffix-boundary check if the repair touched call-argument
  lowering-adjacent code.

Completion check:

- Focused proof is green.
- The 286/288 CLI subset is green.
- `todo.md` records all proof commands and results.

### Step 5: Handoff back to idea 291 closure

Goal: prepare lifecycle handoff so idea 291 can be reactivated for close-time
validation.

Primary target: `todo.md` lifecycle notes.

Actions:

- Summarize the repaired blocker and proof commands.
- State whether idea 291's suffix-parser boundary remained unchanged.
- Ask the supervisor to run the plan-owner close workflow for this blocker and
  then reactivate or close idea 291 as appropriate.

Completion check:

- The 286/288 CLI blocker is resolved or any remaining blocker is explicit.
- Lifecycle notes identify idea 291 as the follow-up close candidate.
