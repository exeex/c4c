# LIR/BIR Freeze Closure Gate Runbook

Status: Active
Source Idea: ideas/open/188_lir_bir_freeze_closure_gate.md

## Purpose

Close the LIR/BIR freeze wave as the final pre-backend-restart gate.

## Goal

Produce a freeze closure ledger, prove the milestone with broad validation, and decide whether backend restart is clear or a narrow blocker idea must be opened first.

## Core Rule

This plan is a gate, not backend restart work. Do not start target MIR, assembler, linker, object emission, or backend restart implementation inside this runbook.

## Read First

- `ideas/open/188_lir_bir_freeze_closure_gate.md`
- The completed dependency records for ideas 183-187, using closed archive files only as historical input for this gate.
- Current LIR/BIR/backend-prealloc implementation surfaces only as needed to verify classification and validation claims.

## Current Targets

- Direct-call signature metadata and generated direct-call paths.
- Global and type declaration tables.
- Direct symbol identity validation surfaces.
- Memory provenance global handles.
- Backend prealloc route-local naming.
- Retained strings at compatibility, diagnostics, display/output, ABI/final spelling, and explicit no-metadata boundaries.

## Non-Goals

- Do not begin backend restart implementation.
- Do not rewrite target MIR, assemblers, linkers, or object emission.
- Do not try to remove all final spelling strings from output layers.
- Do not accept testcase expectation downgrades as freeze progress.
- Do not create broad implementation rewrites while performing the closure gate.

## Working Model

- Ideas 183-187 are the convergence batch being gated.
- The ledger must distinguish semantic identity authority from final spelling, printer, diagnostic, route-local, and no-metadata compatibility boundaries.
- If a generated path still relies on rendered text as semantic authority, the gate is not closed; capture a narrow blocker idea instead of starting backend restart.

## Execution Rules

- Keep routine evidence and intermediate findings in `todo.md`.
- Edit the source idea only if durable source intent changes or a separate blocker initiative must be recorded.
- If a new blocker is discovered, create a new open idea with concrete reviewer reject signals before clearing backend restart.
- Use milestone-level validation. Full suite is the normal expectation unless supervisor baseline policy delegates an equivalent regression-guard workflow.
- Treat narrow-only validation as insufficient for closure unless explicitly justified by the supervisor.

## Step 1: Collect Dependency Evidence

Goal: inspect the completed 183-187 records and identify what each one claims about LIR/BIR identity authority.

Concrete actions:
- Locate the closed or historical records for ideas 183-187.
- Extract the claimed closure facts for direct-call signatures, global/type declarations, direct symbol identity, memory provenance handles, and prealloc route-local names.
- Note any explicit compatibility fences, fallback boundaries, and validation evidence already produced.
- Record findings in `todo.md` without editing implementation files.

Completion check:
- `todo.md` names each dependency and summarizes the identity domain, retained boundary, and proof status it contributes to the freeze gate.

## Step 2: Build The Freeze Closure Ledger

Goal: create the ledger that classifies every in-scope LIR/BIR identity domain and retained string boundary.

Concrete actions:
- Map each generated metadata-rich path to its structured fact authority.
- Classify retained strings as display/output, diagnostics, route-local handles, ABI/final spelling, or explicit no-metadata compatibility.
- Flag any high-risk generated path where rendered text still appears to decide semantic identity.
- Keep the ledger concise enough for reviewer use and supervisor closure decisions.

Completion check:
- `todo.md` contains a freeze closure ledger covering all current targets and no unclassified high-risk generated-path string authority remains hidden.

## Step 3: Run Milestone Validation

Goal: prove the closure gate with broad validation appropriate for a milestone.

Concrete actions:
- Use the supervisor-delegated broad validation command, normally the full suite or regression-guard equivalent.
- Preserve canonical validation state according to supervisor policy.
- If the baseline differs, record the regression-guard acceptance path or blocker details.
- Do not downgrade expectations or narrow contracts to make the gate pass.

Completion check:
- Broad validation is green, or any baseline difference is explicitly accepted through the regression guard workflow and recorded in `todo.md`.

## Step 4: Closure Decision

Goal: decide whether backend restart can proceed or whether a new blocker idea is required first.

Concrete actions:
- Compare the ledger and validation result against the source idea acceptance criteria.
- If clear, record the closure recommendation for supervisor/plan-owner close handling.
- If blocked, create or request a narrow `ideas/open/` blocker that names the failing authority path and concrete reject signals.
- Do not treat runbook exhaustion alone as source-idea completion.

Completion check:
- `todo.md` contains an explicit `backend restart clear` or `blocked by new open idea` decision, with enough evidence for supervisor review.
