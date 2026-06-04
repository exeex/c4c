# Closure Note Follow-Up Recovery Audit Runbook

Status: Active
Source Idea: ideas/open/101_closure_note_followup_recovery_audit.md

## Purpose

Recover durable follow-up intent from recent closure notes and turn any still-needed work into focused `ideas/open/*.md` files.

## Goal

Audit closure-note follow-ups from ideas 97 through 99, plus any later closure notes that exist at execution time, and produce a disposition map plus narrowly scoped replacement ideas for unfinished work.

## Core Rule

This is an analysis and lifecycle recovery plan. Do not edit compiler implementation files, downgrade tests, or treat broad cleanup notes as recovered work unless the missing authority fact is concrete.

## Read First

- `ideas/open/101_closure_note_followup_recovery_audit.md`
- `ideas/closed/97_bir_prealloc_call_abi_authority_boundary_audit.md`
- `ideas/closed/98_bir_prealloc_memory_pointer_storage_boundary_audit.md`
- `ideas/closed/99_bir_prealloc_control_publication_lookup_boundary_audit.md`
- Any `ideas/closed/*.md` numbered after `99` that exists when this plan runs

## Scope

- Inventory closure-note follow-up claims that are not currently represented under `ideas/open/`.
- Decide whether each missing follow-up is still needed, completed later, superseded, or stale.
- Create new focused `ideas/open/*.md` files only for still-needed or better-scoped recovered work.
- Preserve authority boundaries between BIR semantics, prealloc prepared-plan/storage facts, and backend consumers.
- Keep a short disposition map in `todo.md` during execution so the close note can be written without re-auditing.

## Non-Goals

- Do not implement recovered follow-up ideas.
- Do not edit `src/backend/bir/**`, `src/backend/prealloc/**`, or backend code except for read-only context.
- Do not move old closed ideas back to `ideas/open/`.
- Do not create generic cleanup ideas without exact duplicated, missing, or misplaced authority facts.
- Do not scan `ideas/closed/` beyond the closure notes needed for this audit and later-numbered notes that may contain follow-up claims.

## Working Model

- Closure notes are evidence, not commands to recreate every listed follow-up.
- Existing open ideas and later closed ideas may supersede or complete a note-listed item.
- Recovered ideas should use the next available `ideas/open/` number at creation time and include concrete reviewer reject signals.
- When a note item is not recreated, record why in the disposition map.

## Execution Rules

- Treat `ideas/open/101_closure_note_followup_recovery_audit.md` as the source contract.
- Use `rg --files ideas/open ideas/closed` or equivalent fast listing before assigning any new idea number.
- Before writing a recovered idea, check whether the same work already appears in `ideas/open/` or later closure notes.
- Each new idea must include goal or intent, why it exists, in-scope work, out-of-scope work, acceptance or completion criteria, and `## Reviewer Reject Signals`.
- Proof is analysis-only unless implementation files are accidentally changed.
- If any implementation file changes, stop and hand back for supervisor recovery instead of continuing the audit.

## Steps

### Step 1: Inventory Closure-Note Follow-Up Claims

Goal: Build a complete list of follow-up claims from the target closure notes.

Primary targets:

- `ideas/closed/97_bir_prealloc_call_abi_authority_boundary_audit.md`
- `ideas/closed/98_bir_prealloc_memory_pointer_storage_boundary_audit.md`
- `ideas/closed/99_bir_prealloc_control_publication_lookup_boundary_audit.md`
- Later-numbered `ideas/closed/*.md` files only if they contain closure-note follow-up claims relevant to this audit

Actions:

- Read the target closure notes and extract every named or implied follow-up item.
- Record original numbers or names, claimed purpose, and authority domain.
- List current `ideas/open/*.md` names before judging missing status.
- Write the working inventory into `todo.md` rather than editing the source idea.

Completion check:

- `todo.md` contains a concise inventory of every closure-note follow-up claim and the current open-idea inventory used for comparison.

### Step 2: Classify Each Missing Follow-Up

Goal: Decide the final disposition for each closure-note item before creating replacement ideas.

Actions:

- For each note-listed item, classify it as still needed as-is, completed by later work, superseded by a better-scoped idea, or stale/no-action.
- Use closed ideas after the source note only as read-only evidence of completion or supersession.
- Read relevant `src/backend/bir/**`, `src/backend/prealloc/**`, and `src/backend/mir/aarch64/codegen/**` surfaces only when needed to confirm authority ownership.
- Keep classifications factual and tied to specific authority facts.

Completion check:

- `todo.md` has a disposition table that maps every closure-note item to one classification, supporting evidence, and whether a new idea will be created.

### Step 3: Create Recovered Focused Ideas

Goal: Recreate only the still-needed follow-up work as focused, numbered source ideas.

Primary target:

- New files under `ideas/open/*.md`

Actions:

- Determine the next available idea number at write time.
- Create one narrow idea per distinct authority fact or proof route.
- Preserve BIR semantic authority, prealloc prepared-plan/storage authority, and backend consumer authority as separate ownership boundaries.
- Include concrete reviewer reject signals for testcase-shaped shortcuts, expectation downgrades, helper-only churn, broad rewrites, and renamed versions of the old failure mode.

Completion check:

- Every still-needed item has a new focused `ideas/open/*.md` file or a documented reason why no file was created.
- No existing open idea is overwritten or renumbered.

### Step 4: Finalize Audit Disposition

Goal: Leave the active runbook ready for closure decision with proof of the audit result.

Actions:

- Update `todo.md` with the final disposition map and the list of created idea files.
- Confirm no implementation files changed.
- Confirm the generated ideas are actionable and include reviewer reject signals.
- Prepare a short close-note payload for the supervisor or plan owner to use when closing this source idea.

Completion check:

- `todo.md` records the final mapping from each closure-note item to its disposition.
- `git status --short` shows only expected lifecycle/idea-file changes for this audit.
- The active idea can be closed without re-reading all target closure notes.
