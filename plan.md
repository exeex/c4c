# AArch64 Calls Owner Subresponsibility Audit Runbook

Status: Active
Source Idea: ideas/open/92_aarch64_calls_owner_subresponsibility_audit.md

## Purpose

Audit the remaining ownership boundaries inside `src/backend/mir/aarch64/codegen/calls.cpp` after the earlier call-boundary, aggregate transport, prepared-wrapper, printer, and aggregate-lane cleanup routes.

## Goal

Produce a function-level calls-owner inventory and scoped follow-up ideas only for clusters with a stable owner boundary, narrow API, and credible proof route.

## Core Rule

This is audit-only. Do not edit implementation files. Do not propose broad "shrink calls.cpp" or "move to BIR" routes without naming the missing target-neutral fact and preserving AArch64 ABI/prepared-source authority.

## Read First

- `ideas/open/92_aarch64_calls_owner_subresponsibility_audit.md`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/calls.hpp`
- `ideas/closed/69_aarch64_call_publication_prepared_authority_cleanup.md`
- `ideas/closed/75_shared_aggregate_transport_plan_probe.md`
- `ideas/closed/84_aarch64_prepared_consumer_wrapper_contraction.md`
- `ideas/closed/87_aarch64_call_boundary_record_printer_surface_audit.md`
- `ideas/closed/90_aarch64_aggregate_lane_helper_table_contraction.md`
- `ideas/closed/91_aarch64_call_boundary_aggregate_lane_record_schema_cleanup.md`

## Current Targets

- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/calls.hpp`
- Follow-up idea files under `ideas/open/` only when the audit finds a clear owner boundary.

## Non-Goals

- Do not implement changes in `calls.cpp` or `calls.hpp`.
- Do not move AArch64 ABI register conversion, concrete call spelling, scratch choice, source/destination selection, or call-boundary machine-record construction into shared BIR/prealloc.
- Do not reopen dispatch publication, edge-copy, memory, instruction/printer, aggregate transport, prepared call authority, or aggregate-lane cleanup routes without new evidence.
- Do not split call-boundary record families unless the boundary is traced and semantics-preserving.
- Do not treat line-count reduction as success.

## Working Model

Classify functions and helper clusters with these labels where applicable:

- `prepared-call-plan-consumption`
- `direct-indirect-call-emission`
- `before-after-call-move-bundles`
- `call-boundary-record-construction`
- `call-boundary-abi-binding-records`
- `stack-argument-marshalling`
- `indirect-callee-materialization`
- `value-and-preservation-republication`
- `immediate-argument-publication`
- `f128-carrier-handling`
- `aggregate-byval-lane-transport`
- `sret-and-result-retargeting`
- `callee-saved-preservation`
- `diagnostics-and-selection-status`
- `candidate-local-subowner`
- `needs-shared-authority-evidence`

Clusters should remain in `calls.cpp` when they own AArch64 ABI construction, selected-node facts, prepared-source decisions, scratch/preservation material, call spelling, diagnostics, or machine-record construction.

## Execution Rules

- Keep routine audit findings in `todo.md` until a durable follow-up idea is justified.
- Source idea edits are unnecessary for ordinary audit progress.
- Generated follow-up ideas must include concrete scope, out-of-scope boundaries, proof expectations, and reviewer reject signals.
- Any implementation idea must name focused proof covering the affected call forms, such as argument/return moves, move bundles, indirect calls, stack arguments, f128 carriers, aggregate byval lanes, preservation/republication, sret/result retargeting, and rejection diagnostics.
- No backend tests are required for audit-only lifecycle work unless implementation files are touched accidentally.

## Step 1: Build Calls Owner Inventory

Goal: produce a function-level map of `calls.cpp` and public/private declarations in `calls.hpp`.

Actions:

- Inspect top-level functions, local helpers, structs, and public declarations.
- Record each function's immediate responsibility, primary inputs, emitted records/instructions, and call sites.
- Note which functions are already constrained by ideas 69, 75, 84, 87, 90, or 91.
- Keep the inventory in `todo.md` unless it becomes a durable follow-up idea.

Completion check:

- `todo.md` contains an inventory summary that lets the next step classify all major calls-owner regions without rereading the whole file from scratch.

## Step 2: Classify Responsibility Clusters

Goal: assign the source idea's responsibility labels to the inventory and separate stable local owners from shared-authority candidates.

Actions:

- Group functions into coherent clusters.
- Mark target-local clusters that should stay in `calls.cpp`.
- Mark `candidate-local-subowner` only when the helper boundary is narrow, stable, and does not rederive prepared call, move-bundle, after-call result, preservation, aggregate transport, or publication facts.
- Mark `needs-shared-authority-evidence` when an implementation route would require new target-neutral facts or cross-owner proof.

Completion check:

- `todo.md` names each cluster, its classification labels, and whether it should remain target-local, become a follow-up implementation idea, or first receive an evidence idea.

## Step 3: Draft Scoped Follow-Up Ideas

Goal: create follow-up ideas only for clusters with clear owner boundaries and proof routes.

Actions:

- For each accepted candidate, write a focused `ideas/open/*.md` idea.
- Include goal, why it exists, in-scope work, out-of-scope work, acceptance criteria, proof expectations, and `## Reviewer Reject Signals`.
- For clusters without enough evidence, either leave them intentionally in `calls.cpp` or create a separate evidence-gathering idea.
- Avoid one monolithic calls shrink route.

Completion check:

- Any new idea file is narrowly scoped and rejectable under the source idea's reviewer signals.
- `todo.md` names all generated follow-up ideas and all rejected/deferred candidates.

## Step 4: Close Audit Route

Goal: finish the audit with durable closure notes that identify generated follow-ups and intentionally target-local clusters.

Actions:

- Confirm no implementation files were edited.
- Summarize generated follow-up ideas and intentionally retained calls-owner clusters.
- Ask the lifecycle owner to close this source idea only when the source idea's proof expectations are satisfied.

Completion check:

- The audit can be closed with a note naming follow-up ideas and explaining which calls clusters remain intentionally target-local.
