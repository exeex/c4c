# LIR/BIR Freeze Closure Gate Runbook

Status: Active
Source Idea: ideas/open/188_lir_bir_freeze_closure_gate.md

## Purpose

Close the LIR/BIR freeze wave as the final pre-backend-restart gate.

## Goal

Produce a freeze ledger for completed LIR/BIR identity-authority work, prove the milestone with broad validation, and decide whether backend restart may proceed or another narrow blocker idea must be opened first.

## Core Rule

Do not begin backend restart work in this runbook. This gate only classifies, validates, and records whether the restart is unblocked.

## Read First

- `ideas/open/188_lir_bir_freeze_closure_gate.md`
- `ideas/closed/183_lir_bir_backend_freeze_authority_audit.md`
- `ideas/closed/184_direct_call_signature_metadata_structured_boundary.md`
- `ideas/closed/185_lir_to_bir_global_typedecl_compatibility_fence.md`
- `ideas/closed/186_bir_direct_symbol_identity_validation_closure.md`
- `ideas/closed/187_bir_memory_provenance_global_handle_cleanup.md`
- `ideas/closed/189_direct_call_no_prototype_variadic_signature_mismatch.md`
- `ideas/closed/190_lir_call_argument_structured_payload_boundary.md`
- `ideas/closed/191_bir_function_signature_byval_metadata_text_retirement.md`
- `ideas/closed/194_bir_global_memory_provenance_linknameid_expansion.md`

## Current Targets

- Direct-call signature authority and no-prototype/variadic compatibility.
- Global/type declaration tables and direct symbol identity.
- Memory provenance global handles, including `LinkNameId` expansion from idea 194.
- BIR/prealloc route-local names and generated metadata-rich paths.
- Retained string boundaries that are display/output, diagnostics, route-local handles, ABI/final spelling, or explicit no-metadata compatibility.

## Non-Goals

- Do not start the backend restart itself.
- Do not rewrite target MIR, assemblers, linkers, or object emission.
- Do not remove final spelling strings from backend output layers unless a narrow blocker is discovered and delegated separately.
- Do not claim freeze progress through testcase expectation downgrades or weaker contracts.

## Working Model

The closed dependency ideas are the source ledger inputs. This runbook should not re-litigate every implementation detail; it should verify that no high-risk generated-path string-authority surface remains unclassified after ideas 183-187, 189-191, and 194.

## Execution Rules

- Keep routine execution notes in `todo.md`.
- If a remaining high-risk string-authority surface is found, create or request a narrow open idea before backend restart instead of expanding this gate into implementation work.
- Treat printer, assembler, diagnostic, ABI, and final output spelling as retained strings only after classifying their role.
- Treat any expectation downgrade or named-case-only proof as a blocker, not freeze progress.
- Use full-suite validation for milestone acceptance unless the supervisor explicitly accepts a different regression-guard scope.

## Step 1 - Build The Freeze Ledger

Goal: summarize the completed dependency ideas into a concrete identity-domain ledger.

Actions:
- Read the closed dependency ideas listed above.
- Record each LIR/BIR identity domain that was repaired, fenced, or classified.
- Record each retained string boundary and why it is acceptable.
- Call out any uncertain generated-path string authority that needs follow-up.

Completion Check:
- `todo.md` contains a freeze ledger covering ideas 183-187, 189-191, and 194.
- Any uncertain surface is named precisely enough for a narrow follow-up idea.

## Step 2 - Audit For Remaining High-Risk Generated-Path Text Authority

Goal: verify the ledger against current code surfaces without turning the gate into a broad implementation task.

Actions:
- Inspect the relevant LIR-to-BIR/backend-prealloc surfaces named by the ledger.
- Confirm structured facts own metadata-rich generated paths.
- Confirm retained raw strings are display/output, diagnostics, route-local handles, ABI/final spelling, or explicit no-metadata compatibility.
- If a concrete blocker remains, stop and record it as a new narrow idea instead of patching it here.

Completion Check:
- `todo.md` records either that no high-risk generated-path string authority remains unclassified, or names the exact blocker idea to open.

## Step 3 - Run Milestone Validation

Goal: prove the freeze gate with broad validation.

Actions:
- Use the supervisor-selected milestone validation command, normally a full build plus full CTest suite.
- Preserve canonical regression logs as `test_before.log` and `test_after.log` when the supervisor asks for regression-guard proof.
- Record any baseline acceptance or failure reason in `todo.md`.

Completion Check:
- Broad validation is green, or any baseline difference is explicitly accepted through the regression guard workflow.
- `todo.md` names the command and result.

## Step 4 - Decide Backend Restart Readiness

Goal: make a lifecycle-quality decision about whether this gate can close.

Actions:
- Compare the ledger, audit result, and validation result against the source idea acceptance criteria.
- If ready, record that backend restart is unblocked by the LIR/BIR freeze gate.
- If blocked, create or request a narrow open idea for the remaining blocker and leave the gate unresolved.

Completion Check:
- `todo.md` states either restart-ready closure evidence or the exact blocker preventing closure.

## Step 5 - Prepare Closure Evidence

Goal: leave the active state ready for plan-owner close review.

Actions:
- Ensure the source idea acceptance criteria are all addressed in `todo.md`.
- Ensure no backend restart implementation work was mixed into this gate.
- Ensure validation evidence points to canonical logs and the selected command.

Completion Check:
- `todo.md` says the source idea is ready for close review, or explains why it is not ready.
