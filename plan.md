# Prepared MIR View Contract Research Runbook

Status: Active
Source Idea: ideas/open/683_prepared_mir_view_contract_research.md

## Purpose

Activate the research step that defines a narrow `PreparedMirView` contract
before any BIR-to-MIR interface implementation cleanup starts.

## Goal

Produce the required eight documentation files under
`docs/prepared_mir_view_contract_research/`, with concrete evidence from live
backend, prepared/prealloc, and MIR consumer code.

## Core Rule

This runbook is research and architecture documentation only. It must not edit
implementation files, tests, expectations, unsupported markers, allowlists,
runtime behavior, active lifecycle history, or the future implementation idea.

## Read First

- `ideas/open/683_prepared_mir_view_contract_research.md`
- `src/backend/prealloc/module.hpp`
- current backend entry surfaces that create and pass `PreparedBirModule`
- current MIR target consumer surfaces under `src/backend/mir/`,
  `src/backend/x86/`, `src/backend/riscv/`, and `src/backend/aarch64/`
- related context only as needed:
  - `ideas/open/678_lir_to_bir_adapter_boundary_umbrella.md`
  - `ideas/closed/585_target_abi_contract_and_value_consumption_research.md`
  - `ideas/open/589_direct_edge_publication_move_freshness_ownership.md`
  - `ideas/open/590_branch_stack_load_freshness_contract.md`

## Current Targets

- `docs/prepared_mir_view_contract_research/index.md`
- `docs/prepared_mir_view_contract_research/01_current_mir_dependencies_on_prepared_bir.md`
- `docs/prepared_mir_view_contract_research/02_prepared_mir_core_view_shape.md`
- `docs/prepared_mir_view_contract_research/03_feature_views_and_optional_contracts.md`
- `docs/prepared_mir_view_contract_research/04_debug_proof_and_diagnostic_boundaries.md`
- `docs/prepared_mir_view_contract_research/05_old_bir_new_bir_equivalence_strategy.md`
- `docs/prepared_mir_view_contract_research/06_incremental_migration_plan_for_mir_consumers.md`
- `docs/prepared_mir_view_contract_research/07_open_questions_and_followup_implementation_ideas.md`

## Non-Goals

- Do not create or edit implementation files, build files, tests,
  expectations, unsupported markers, allowlists, runtime harness policy, or
  baseline acceptance policy.
- Do not expose the whole current `PreparedBirModule` under a new name and
  call it a view contract.
- Do not start idea 684 implementation work before this documentation is
  complete.
- Do not rewrite old BIR, build new BIR, change ABI classification, freshness
  authority, branch or edge-publication semantics, object output, or runtime
  behavior.
- Do not let diagnostic, proof, route, local-array, or provenance artifacts
  become semantic MIR codegen authority without an explicit required-fact
  promotion in the research.

## Working Model

Current MIR targets consume prepared backend facts through broad
`PreparedBirModule` access. The research must identify the smallest stable
MIR-facing contract that old BIR and a future new BIR can both produce:

```text
Old BIR -> current prepare ------\
                                  -> PreparedMirView -> MIR
New BIR -> new/compat prepare ---/
```

The documents should separate core codegen inputs from optional feature views,
verifier facts, and observational diagnostic/debug facts.

## Execution Rules

- Start by tracing live code paths before proposing interface shapes.
- Keep each answer file limited to its assigned research question.
- Cite concrete source surfaces and distinguish live implementation consumers
  from markdown-only legacy artifacts.
- Preserve the exact required answer-file count and filenames from the source
  idea.
- Record progress, blockers, and proof notes in `todo.md`; do not rewrite this
  plan for routine documentation progress.
- If implementation work appears necessary, stop and report it as follow-up
  evidence for idea 684 or a new source idea.

## Ordered Steps

### Step 1: Trace Current MIR Dependencies

Goal: produce the evidence baseline for current MIR consumption of
`PreparedBirModule`.

Primary target:

- `docs/prepared_mir_view_contract_research/01_current_mir_dependencies_on_prepared_bir.md`

Actions:

- Trace the current x86 MIR entry path from backend entry to prepared-module
  consumption.
- Inventory every direct `PreparedBirModule` field family touched by live MIR
  code.
- Distinguish live implementation dependencies from docs-only or legacy
  references.
- Classify each dependency as required for codegen, feature-specific,
  diagnostic-only, or accidental.
- State the smallest dependency set a first `PreparedMirView` must expose.

Completion check:

- The answer file exists, cites concrete code surfaces, and identifies the
  minimum first-view dependency set.

### Step 2: Define Core And Optional View Contracts

Goal: document the first-cut core view shape and feature-specific optional
contracts without freezing the entire prepared module as ABI.

Primary targets:

- `docs/prepared_mir_view_contract_research/02_prepared_mir_core_view_shape.md`
- `docs/prepared_mir_view_contract_research/03_feature_views_and_optional_contracts.md`

Actions:

- Propose `PreparedMirCoreView` fields and accessors in C++ interface terms.
- Explain why each core field is required by MIR rather than target-local
  convenience.
- Identify current `PreparedBirModule` fields that are not core.
- Decide whether the core exposes BIR traversal directly, an instruction
  cursor abstraction, or both.
- Table feature contracts for calls, variadic entry, i128/f128 carriers,
  atomics, intrinsics, inline asm, and object data.
- Name optional feature views, required presence checks, and fail-closed
  behavior.

Completion check:

- Both answer files exist, name concrete view surfaces, and separate core
  requirements from optional or target-local contracts.

### Step 3: Bound Proof, Diagnostic, And Equivalence Facts

Goal: prevent proof and diagnostic artifacts from becoming MIR semantic
authority while defining old/new BIR equivalence at the view boundary.

Primary targets:

- `docs/prepared_mir_view_contract_research/04_debug_proof_and_diagnostic_boundaries.md`
- `docs/prepared_mir_view_contract_research/05_old_bir_new_bir_equivalence_strategy.md`

Actions:

- Define fact categories such as `RequiredFact`, `VerifierFact`, and
  `DiagnosticFact`.
- Classify publication, freshness, provenance, local-array, and select-chain
  route families using those categories.
- Identify any current target-consumer paths where proof or diagnostic data
  could influence emission.
- Propose naming or type boundaries that keep diagnostic-only facts
  observational.
- Define old/new producer equivalence at the `PreparedMirView` boundary.
- Propose structural comparison, golden dumps, and verifier checks that compare
  old and new producers without freezing the full `PreparedBirModule`.

Completion check:

- Both answer files exist, state rejectable diagnostic-authority boundaries,
  and explain how old and new producers can be compared at the same view.

### Step 4: Plan Migration And Follow-Up Ideas

Goal: document the least risky migration sequence and the implementation or
discussion work that should follow the research.

Primary targets:

- `docs/prepared_mir_view_contract_research/06_incremental_migration_plan_for_mir_consumers.md`
- `docs/prepared_mir_view_contract_research/07_open_questions_and_followup_implementation_ideas.md`

Actions:

- Propose a phased migration that starts with reference-only adapter views.
- Identify the first x86 entry or lowering surface to migrate.
- Define compile-time or review-time checks that block new unrestricted
  `PreparedBirModule` dependencies in MIR.
- State how RV64 and AArch64 follow after x86.
- Name rollback points and proof categories for each phase.
- Separate unresolved architecture decisions from narrow implementation ideas.
- Identify interactions with ideas 589 and 590.
- Recommend the next one to three source ideas, if the evidence supports them.

Completion check:

- Both answer files exist, provide an ordered migration path, and separate
  discussion-required decisions from concrete follow-up ideas.

### Step 5: Assemble Index And Acceptance Audit

Goal: complete the research document set and leave a clear handoff for the
implementation idea that consumes it.

Primary target:

- `docs/prepared_mir_view_contract_research/index.md`

Actions:

- Link all seven numbered answer files from `index.md`.
- Summarize the recommended `PreparedMirView` design without replacing the
  detailed answer files.
- Include a final recommendation table classifying each follow-up as
  documentation, narrow implementation idea, or discussion-required
  architecture work.
- Audit the directory for exactly one `index.md` plus the seven required answer
  files with exact filenames.
- Confirm no implementation files, tests, expectations, unsupported markers,
  allowlists, runtime behavior, active history, or lifecycle source ideas were
  changed by the research.

Completion check:

- The documentation directory contains exactly the required Markdown files.
- Every source-idea acceptance criterion is satisfied or a precise blocker is
  recorded in `todo.md`.
- The research is ready for supervisor lifecycle review before idea 684 is
  considered for activation.
