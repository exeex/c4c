# Structured Identity Completion Audit and HIR Plan

Status: Active
Source Idea: ideas/open/97_structured_identity_completion_audit_and_hir_plan.md

## Purpose

Audit parser and sema structured-identity completion after ideas 95 and 96, then create the follow-on planning artifacts needed for HIR structured identity migration.

## Goal

Produce an evidence-backed review artifact, decide whether parser/sema leftovers require idea 98, and create idea 99 as the HIR dual-lookup starting plan.

## Core Rule

This is an audit and planning runbook only. Do not implement cleanup, delete string-keyed maps, downgrade expectations, or add testcase-shaped exceptions.

## Read First

- `AGENTS.md`
- `todo.md`
- `ideas/open/97_structured_identity_completion_audit_and_hir_plan.md`
- Parser, sema, and HIR lookup surfaces discovered by `rg` inventory
- Existing validation logs only when needed to understand 95/96 closure context

## Current Scope

- Parser lookup identity leftovers after the parser structured lookup cleanup.
- Sema lookup identity leftovers after the sema structured lookup cleanup.
- HIR string-keyed identity surfaces that can start a dual-write / dual-read migration.
- Follow-on idea creation under `ideas/open/` for parser/sema leftovers, if needed, and for HIR migration.

## Non-Goals

- Do not change implementation files.
- Do not change tests or expectations.
- Do not reopen ideas 95 or 96 unless the audit proves they were closed with known blockers.
- Do not start HIR migration before idea 99 exists.
- Do not collapse parser/sema completion work into HIR migration work.
- Do not treat bridge-required, diagnostic-only, or legacy-proof strings as accidental leftovers without evidence.

## Working Model

- Classify remaining parser strings as `bridge-required`, `diagnostic-only`, `legacy-proof`, `parser-leftover`, or `blocked-by-downstream`.
- Classify remaining sema strings as `bridge-required`, `diagnostic-only`, `legacy-proof`, `sema-leftover`, or `blocked-by-hir`.
- Classify HIR surfaces by upstream structured-key availability, mirror feasibility, downstream rendered-name requirements, proof coverage, and first-slice dependency shape.
- Any parser/sema follow-up belongs in idea 98 only when meaningful leftovers remain.
- HIR migration starts in idea 99 and must preserve rendered names for codegen, diagnostics, and link names.

## Execution Rules

- Keep audit evidence in `review/97_structured_identity_completion_audit.md`.
- Create `ideas/open/98_*.md` only if parser/sema leftovers justify it.
- Always create `ideas/open/99_*.md` for the HIR starting strategy.
- Use `rg` / `git grep` inventories before drawing conclusions.
- Use focused proof commands only to ground audit claims; do not make code changes.
- If a proof run is needed, record the exact command and outcome in the review artifact and `todo.md`.
- If evidence shows a separate initiative outside idea 97, report it instead of silently expanding this runbook.

## Ordered Steps

### Step 1: Audit setup and inventory

Goal: Build the current evidence map for parser, sema, and HIR lookup identity usage.

Primary target: source inventory only.

Actions:
- Confirm `plan.md` and `todo.md` point to idea 97.
- Read idea 97 acceptance criteria and audit classifications.
- Run targeted `rg` / `git grep` inventory for parser lookup tables, sema lookup state, HIR module maps, HIR lowerer lookup, compile-time registries, template instantiation maps, enum/const-int bindings, and link/codegen handoff names.
- Record candidate files, symbols, and search patterns for later review sections.

Completion check:
- `todo.md` names the inventory commands and the main surfaces found.
- No implementation files or tests are modified.

### Step 2: Parser completion audit

Goal: Classify remaining parser string lookup paths after idea 95.

Primary target: parser-owned lookup and helper APIs.

Actions:
- Inspect value binding lookup and `using` value import paths.
- Inspect `UsingValueAlias` resolution.
- Inspect NTTP default-expression cache keys.
- Inspect template instantiation de-dup keys.
- Inspect parser string-first helper overloads and tests that still prove only legacy paths.
- Separate rendered-name bridge output from semantic lookup input.
- Classify each finding as `bridge-required`, `diagnostic-only`, `legacy-proof`, `parser-leftover`, or `blocked-by-downstream`.

Completion check:
- The review artifact has parser findings grouped by classification with file/symbol evidence.
- Parser leftovers, if any, are specific enough to become idea 98 scope without including HIR work.

### Step 3: Sema completion audit

Goal: Classify remaining sema string lookup paths after idea 96.

Primary target: sema-owned lookup and validation state.

Actions:
- Inspect scoped local symbol tables.
- Inspect global symbol and function signature maps.
- Inspect C++ and reference overload sets.
- Inspect enum and const-int bindings.
- Inspect consteval function registry and interpreter locals.
- Inspect NTTP/type bindings, canonical symbol helpers, struct completeness, and member/static-member lookup mirrors.
- Classify each finding as `bridge-required`, `diagnostic-only`, `legacy-proof`, `sema-leftover`, or `blocked-by-hir`.

Completion check:
- The review artifact has sema findings grouped by classification with file/symbol evidence.
- Sema leftovers, if any, are specific enough to become idea 98 scope without including HIR work.

### Step 4: HIR inventory and first-slice analysis

Goal: Define the HIR structured-identity migration starting point.

Primary target: HIR-owned string-keyed identity surfaces.

Actions:
- Inspect HIR module maps keyed by rendered struct, function, and global names.
- Inspect struct layout and `TypeSpec::tag` consumers.
- Inspect HIR lowerer symbol lookup plus method/member lookup.
- Inspect compile-time engine template and consteval definition registries.
- Inspect template instantiation work queues and instance maps.
- Inspect enum/const-int bindings used during HIR normalization.
- Inspect link-name and codegen handoff boundaries.
- For each surface, identify upstream structured-key availability, mirror feasibility, downstream rendered-name requirements, focused proof, and whether parser/sema metadata is blocking.

Completion check:
- The review artifact has HIR inventory grouped by subsystem.
- The recommended first HIR slice is narrow, behavior-preserving, and compatible with rendered codegen/link-name output.

### Step 5: Follow-on idea drafting

Goal: Materialize the audit conclusions as source ideas.

Primary target: `ideas/open/98_*.md` when needed, and `ideas/open/99_*.md`.

Actions:
- Decide whether idea 98 is necessary based on parser/sema `parser-leftover` and `sema-leftover` findings.
- If needed, create idea 98 for parser/sema post-cleanup leftovers only, preserving bridge-required and downstream-blocked distinctions.
- If not needed, state in the review artifact that idea 98 is intentionally unnecessary.
- Create idea 99 for HIR dual-write / dual-read migration, preserving rendered names for codegen, diagnostics, and link names.
- Include acceptance criteria and focused validation guidance in each generated idea.

Completion check:
- Idea 99 exists under `ideas/open/`.
- Idea 98 either exists under `ideas/open/` or the review artifact explicitly explains why it is unnecessary.
- Follow-on ideas do not request implementation changes as part of idea 97.

### Step 6: Final audit consistency check

Goal: Make the audit ready for supervisor review and lifecycle closure decision.

Primary target: review artifact and follow-on source ideas.

Actions:
- Re-read the review artifact against idea 97 acceptance criteria.
- Confirm parser/sema leftovers are separated from bridge-required, diagnostic-only, legacy-proof, and blocked classifications.
- Confirm HIR cleanup remains separate from parser/sema completion work.
- Record any proof commands run and their outcomes.
- Update `todo.md` with final packet state and suggested supervisor next action.

Completion check:
- `review/97_structured_identity_completion_audit.md` satisfies the Required Outputs from idea 97.
- Follow-on idea files satisfy the Required Outputs from idea 97.
- No implementation files, tests, or expectations are modified.
