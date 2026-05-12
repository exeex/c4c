# LIR/BIR Freeze Closure Gate Runbook

Status: Active
Source Idea: ideas/open/188_lir_bir_freeze_closure_gate.md

## Purpose

Close the LIR/BIR freeze wave as the final pre-backend-restart gate.

## Goal

Review the completed dependency set for ideas 183-187, 189-191, and 194, produce a current freeze closure ledger, prove the milestone with broad validation, and decide whether backend restart is clear or still blocked.

## Core Rule

This plan is dependency-gated. Do not recommend `backend restart clear` until ideas 190, 191, and 194 have been completed or the source idea is explicitly narrowed by lifecycle action.

## Read First

- `ideas/open/188_lir_bir_freeze_closure_gate.md`
- `ideas/open/190_lir_call_argument_structured_payload_boundary.md`
- `ideas/open/191_bir_function_signature_byval_metadata_text_retirement.md`
- `ideas/open/194_bir_global_memory_provenance_linknameid_expansion.md`
- The completed dependency records for ideas 183-187 and 189, using closed archive files only as historical input for this gate.
- Current LIR/BIR/backend-prealloc implementation surfaces only as needed to verify classification and validation claims.

## Current Targets

- Direct-call signature metadata and generated direct-call paths.
- Structured LIR call argument payloads for metadata-rich call lowering.
- BIR function-signature byval metadata that avoids semantic parsing of `signature_text`.
- Global and type declaration tables.
- Direct symbol identity validation surfaces.
- Memory provenance global handles, including the additional `LinkNameId` route required by idea 194.
- Backend prealloc route-local naming.
- Retained strings at compatibility, diagnostics, display/output, ABI/final spelling, and explicit no-metadata boundaries.

## Non-Goals

- Do not begin backend restart implementation.
- Do not rewrite target MIR, assemblers, linkers, or object emission.
- Do not try to remove all final spelling strings from output layers.
- Do not accept testcase expectation downgrades as freeze progress.
- Do not create broad implementation rewrites while performing the closure gate.
- Do not treat green validation alone as closure while source-idea dependencies remain open.

## Working Model

- Ideas 183-187 were the first convergence batch gated by this idea.
- Idea 189 closed the direct-call no-prototype and variadic signature mismatch found during milestone validation.
- Ideas 190, 191, and 194 are still part of the gate because the source idea lists them as dependencies and says the closure ledger must review completed 189-191 and 194.
- Idea 191 depends on idea 190, so byval signature text retirement cannot be considered closed before the structured LIR call argument boundary is complete.
- If any generated path still relies on rendered text as semantic authority, the gate is not closed; capture or keep the narrow blocker idea instead of starting backend restart.

## Execution Rules

- Keep routine evidence and intermediate findings in `todo.md`.
- Edit the source idea only if durable source intent changes or a separate blocker initiative must be recorded.
- If a new blocker is discovered, create a new open idea with concrete reviewer reject signals before clearing backend restart.
- Use milestone-level validation after dependency completion. Full suite is the normal expectation unless supervisor baseline policy delegates an equivalent regression-guard workflow.
- Treat narrow-only validation as insufficient for closure unless explicitly justified by the supervisor.
- Treat existing full-suite proof as useful historical evidence, not a substitute for reviewing the current dependency scope.

## Step 1: Dependency Readiness Check

Goal: determine whether the closure gate can proceed or must remain blocked behind open source-idea dependencies.

Concrete actions:
- Confirm the lifecycle status of ideas 190, 191, and 194.
- If any remain open, record that the freeze gate is blocked and identify the next blocker route for supervisor lifecycle handling.
- If all are closed, collect their completion records and continue to Step 2.
- Do not mark backend restart clear from the older 183-187/189 evidence alone.

Completion check:
- `todo.md` records either `blocked by open dependencies 190/191/194` with the next lifecycle target, or confirms that 190, 191, and 194 are closed and ready for ledger review.

## Step 2: Collect Completed Dependency Evidence

Goal: inspect the completed dependency records and identify what each one claims about LIR/BIR identity authority.

Concrete actions:
- Locate the closed or historical records for ideas 183-187, 189-191, and 194.
- Extract the claimed closure facts for direct-call signatures, structured call arguments, function-signature byval metadata, global/type declarations, direct symbol identity, memory provenance handles, and prealloc route-local names.
- Note any explicit compatibility fences, fallback boundaries, and validation evidence already produced.
- Record findings in `todo.md` without editing implementation files.

Completion check:
- `todo.md` names each dependency and summarizes the identity domain, retained boundary, and proof status it contributes to the freeze gate.

## Step 3: Build The Freeze Closure Ledger

Goal: create the ledger that classifies every in-scope LIR/BIR identity domain and retained string boundary.

Concrete actions:
- Map each generated metadata-rich path to its structured fact authority.
- Classify retained strings as display/output, diagnostics, route-local handles, ABI/final spelling, or explicit no-metadata compatibility.
- Flag any high-risk generated path where rendered text still appears to decide semantic identity.
- Keep the ledger concise enough for reviewer use and supervisor closure decisions.

Completion check:
- `todo.md` contains a freeze closure ledger covering all current targets and no unclassified high-risk generated-path string authority remains hidden.

## Step 4: Run Milestone Validation

Goal: prove the closure gate with broad validation appropriate for a milestone after all dependency evidence has been reviewed.

Concrete actions:
- Use the supervisor-delegated broad validation command, normally the full suite or regression-guard equivalent.
- Preserve canonical validation state according to supervisor policy.
- If the baseline differs, record the regression-guard acceptance path or blocker details.
- Do not downgrade expectations or narrow contracts to make the gate pass.

Completion check:
- Broad validation is green, or any baseline difference is explicitly accepted through the regression guard workflow and recorded in `todo.md`.

## Step 5: Closure Decision

Goal: decide whether backend restart can proceed or whether a new blocker idea is required first.

Concrete actions:
- Compare the ledger and validation result against the source idea acceptance criteria.
- If clear, record the closure recommendation for supervisor/plan-owner close handling.
- If blocked, create or request a narrow `ideas/open/` blocker that names the failing authority path and concrete reject signals.
- Do not treat runbook exhaustion alone as source-idea completion.

Completion check:
- `todo.md` contains an explicit `backend restart clear` or `blocked by new/open idea` decision, with enough evidence for supervisor review.
