# Phase D versus Phase E Lifecycle Naming Cleanup Runbook

Status: Active
Source Idea: ideas/open/198_phase_d_phase_e_lifecycle_naming_cleanup.md

## Purpose

Remove lifecycle ambiguity created by closed ideas 182-189 having
`phase_e_*` filenames even though their accepted work was Phase D follow-up
selected-consumer migration and one cross-target interface reuse point.

## Goal

Produce a durable naming note that future agents can cite when distinguishing
Phase D follow-up evidence from the future true Phase E `PreparedBirModule`
retirement analysis in draft 155.

## Core Rule

Treat ideas 182-189 as Phase D follow-up slices despite their filenames. Do
not rename closed idea files, open draft 155, or claim `PreparedBirModule`
retirement progress from label cleanup.

## Read First

- `ideas/open/198_phase_d_phase_e_lifecycle_naming_cleanup.md`
- `docs/bir_prealloc_fusion/phase_d_followup_pre_phase_e_readiness.md`
- `docs/bir_prealloc_fusion/residual_route_view_consumer_migration_map.md`
- `docs/bir_prealloc_fusion/cross_target_route_view_reuse_map.md`
- `docs/bir_prealloc_fusion/prepared_function_lookups_ownership_readiness_map.md`
- `docs/bir_prealloc_fusion/prepared_diagnostics_oracle_replacement_plan.md`
- `docs/bir_prealloc_fusion/return_chain_import_and_naming.md`
- `ideas/closed/182_phase_e_route4_publication_view_consumer_migration.md`
- `ideas/closed/183_phase_e_route5_edge_join_source_view_consumer_migration.md`
- `ideas/closed/184_phase_e_route1_producer_constant_view_consumer_migration.md`
- `ideas/closed/185_phase_e_route2_select_chain_view_consumer_migration.md`
- `ideas/closed/186_phase_e_route3_memory_semantic_view_consumer_migration.md`
- `ideas/closed/187_phase_e_route6_call_use_source_view_consumer_migration.md`
- `ideas/closed/188_phase_e_route7_comparison_view_consumer_migration.md`
- `ideas/closed/189_phase_e_cross_target_route_view_interface_reuse.md`
- `ideas/draft/155_phase_e_prepared_bir_module_retirement_plan.md`, if present.

## Scope

- Durable docs note for lifecycle naming and future citation language.
- Classification of ideas 182-189 as Phase D follow-up evidence with
  Phase E-like filenames.
- Citation rules for future references to ideas 182-189.
- Draft 155 prerequisite list based on the readiness, ownership, diagnostics,
  cross-target, residual-consumer, and return-chain docs now available.
- Explicit rule that historical closed idea filenames remain unchanged.

## Non-Goals

- Do not rename closed idea files for cosmetic cleanup.
- Do not edit closed idea contents solely to match the new label.
- Do not open, rewrite, or execute draft 155.
- Do not change implementation, tests, expectations, unsupported markers, or
  prepared API contracts.
- Do not claim selected-consumer migrations satisfy `PreparedBirModule`
  retirement acceptance criteria.

## Working Model

Ideas 182-188 each prove one selected AArch64 route-view consumer boundary.
Idea 189 proves one x86 Route 6 interface reuse boundary. These are useful
future Phase E inputs, but they are not the Phase E retirement plan. Draft 155
remains draft-only until future lifecycle work deliberately activates or
rewrites it using the prerequisite maps.

## Execution Rules

- Treat this as docs/lifecycle-only unless the supervisor explicitly delegates
  a different packet.
- Preserve historical filenames and closed idea history.
- Write citation language that uses both labels when necessary: "Phase D
  follow-up with a Phase E-like filename."
- If a reference could imply Phase E retirement completion, clarify it in the
  new note rather than editing closed history.
- If searches uncover implementation cleanup or draft 155 rewrite needs, record
  them in `todo.md` as follow-up routing notes instead of expanding this plan.

## Steps

### Step 1: Inventory Phase E-like Closed Ideas and Existing References

Goal: collect the exact closed ideas and current docs references that create
or resolve the naming ambiguity.

Primary targets:
- `ideas/closed/182_phase_e_route4_publication_view_consumer_migration.md`
- `ideas/closed/183_phase_e_route5_edge_join_source_view_consumer_migration.md`
- `ideas/closed/184_phase_e_route1_producer_constant_view_consumer_migration.md`
- `ideas/closed/185_phase_e_route2_select_chain_view_consumer_migration.md`
- `ideas/closed/186_phase_e_route3_memory_semantic_view_consumer_migration.md`
- `ideas/closed/187_phase_e_route6_call_use_source_view_consumer_migration.md`
- `ideas/closed/188_phase_e_route7_comparison_view_consumer_migration.md`
- `ideas/closed/189_phase_e_cross_target_route_view_interface_reuse.md`
- Current readiness docs under `docs/bir_prealloc_fusion/`.

Actions:
- Inspect the titles, goals, acceptance criteria, and closure notes for ideas
  182-189.
- Search current docs and open ideas for references to ideas 182-189,
  `phase_e_*`, draft 155, and `PreparedBirModule` retirement.
- Identify any reference language that could make future agents infer true
  Phase E completion from the filenames.

Completion check:
- `todo.md` records the inventory, current references, and any citation hazards
  found.

### Step 2: Classify Lifecycle Meaning and Draft 155 Prerequisites

Goal: separate historical filename labels from actual lifecycle meaning.

Actions:
- Classify each of ideas 182-189 as Phase D follow-up evidence, naming the
  selected consumer or interface boundary it proved.
- Identify the prerequisite maps now available for future draft 155 work:
  readiness audit, residual consumer migration map, cross-target reuse map,
  `PreparedFunctionLookups` ownership map, diagnostics/oracle replacement
  plan, and return-chain import note.
- State what these prerequisites do and do not authorize for draft 155.

Completion check:
- `todo.md` contains a classification summary and prerequisite list that does
  not treat selected-consumer evidence as retirement completion.

### Step 3: Define Stable Citation Language

Goal: give future plans exact wording for the historical naming hazard.

Actions:
- Define accepted language for ideas 182-189, such as "Phase D follow-up
  selected-consumer migration with a Phase E-like filename."
- Define negative citation rules: do not call ideas 182-189 true Phase E
  retirement, `PreparedBirModule` contraction, or draft 155 completion.
- Define when future docs should cite the new naming note instead of repeating
  the full historical explanation.

Completion check:
- `todo.md` records accepted citation language and reject-language examples.

### Step 4: Write the Durable Naming Note

Goal: create the docs artifact that resolves the lifecycle ambiguity.

Primary target:
- `docs/bir_prealloc_fusion/phase_d_phase_e_lifecycle_naming_cleanup.md`

Actions:
- Write a concise note classifying ideas 182-189 as Phase D follow-up slices
  despite `phase_e_*` filenames.
- Include a table for idea number, route or boundary, actual accepted scope,
  and how future plans should cite it.
- State that draft 155 remains the future true Phase E retirement analysis.
- Name the prerequisite docs that should exist before draft 155 activation or
  rewrite.
- State explicitly that no closed files were renamed.

Completion check:
- The new docs artifact satisfies the source idea acceptance criteria.

### Step 5: Final Consistency Check

Goal: verify the cleanup reduces lifecycle ambiguity without claiming
implementation or retirement progress.

Actions:
- Check the artifact against the source idea acceptance criteria and reviewer
  reject signals.
- Confirm the diff contains only lifecycle/docs changes.
- Run formatting or whitespace checks appropriate for changed Markdown files.

Completion check:
- `todo.md` records final proof and states whether the source idea is ready for
  plan-owner close.
