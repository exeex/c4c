# Phase C2 Selected Adapter Cache Contraction Readiness Audit

Status: Active
Source Idea: ideas/open/202_phase_c2_selected_adapter_cache_contraction_readiness_audit.md

## Purpose

Determine whether the Phase B2 selected adapter and diagnostic follow-ups make
any prepared/prealloc lookup, cache, context, diagnostic, or API surface ready
for bounded contraction, or whether each surface must remain public as fallback,
oracle, target policy, or pass context.

## Goal

Produce `docs/bir_prealloc_fusion/phase_c2_selected_adapter_cache_contraction_readiness.md`
as the durable Phase C2 evidence record, with only one-surface-at-a-time
follow-up ideas opened when contraction is actually ready.

## Core Rule

Do not claim contraction readiness from adapter greenness alone. A surface is
contraction-ready only when the audit shows no remaining production,
printer/debug, target-wrapper, oracle, policy-sensitive, or pass-context public
consumer boundary.

## Read First

- `ideas/open/202_phase_c2_selected_adapter_cache_contraction_readiness_audit.md`
- `docs/bir_prealloc_fusion/phase_a2_residual_semantic_owner_audit.md`
- `docs/bir_prealloc_fusion/phase_b2_selected_route_extension_schema_readiness.md`
- `ideas/closed/201_phase_b2_selected_route_extension_schema_readiness_audit.md`
- `ideas/closed/202_route3_memory_source_identity_adapter.md`
- `ideas/closed/203_route4_publication_identity_adapter.md`
- `ideas/closed/204_route5_edge_join_source_adapter.md`
- `ideas/closed/205_route6_call_use_source_adapter.md`
- `ideas/closed/206_route7_comparison_provenance_diagnostic_oracle.md`
- `docs/bir_prealloc_fusion/prepared_function_lookups_ownership_readiness_map.md`
- `docs/bir_prealloc_fusion/prepared_diagnostics_oracle_replacement_plan.md`
- `docs/bir_prealloc_fusion/residual_route_view_consumer_migration_map.md`
- `test_baseline.log`

## Current Targets

- Route 3 memory/source helpers and `memory_accesses`
- Route 4 publication helpers and `edge_publications`
- Route 5 edge/join-source helpers and move/edge publication surfaces
- Route 6 call-use helpers and `call_plans`
- Route 7 comparison/provenance diagnostics and comparison helper surfaces
- Aggregate `PreparedFunctionLookups` coupling
- Prepared diagnostics/oracles and string-authority surfaces

## Non-Goals

- Do not delete, privatize, or contract prepared APIs during this analysis
  phase.
- Do not plan consumer migration ladders; leave that to D2 or later work.
- Do not contract target-policy surfaces.
- Do not treat adapter success as schema evidence, consumer exhaustion, or
  diagnostic replacement.
- Do not combine diagnostics, cache contraction, and consumer migration in one
  broad implementation idea.
- Do not open draft 155.

## Working Model

Classify each touched surface as exactly one of:

- ready for a bounded micro-contraction idea
- private/pass-context ready only after a named consumer migration
- retained public fallback/oracle
- retained target/prepared policy
- diagnostic/oracle replacement prerequisite
- blocked/unknown pending more evidence

For every Route 3 through Route 7 adapter or diagnostic closure, record the
selected reader or diagnostic row, prepared surface touched, public consumer
removal status, retained fallback/oracle, retained policy, proof scope, and
contraction readiness result.

## Execution Rules

- Keep this as an audit and documentation route unless the supervisor delegates
  a follow-up idea creation packet.
- Preserve the source idea; write execution notes and proof into `todo.md`.
- If a surface is not ready, say why in the C2 document instead of weakening
  the classification.
- If a ready surface is found, propose a narrow follow-up idea for that one
  cache/API surface only.
- Treat expectation rewrites, unsupported downgrades, helper renames, or
  classification-only changes as non-progress for contraction.
- Use `test_baseline.log` and accepted closure proof as evidence scope, not as
  standalone proof of contraction readiness.

## Ordered Steps

### Step 1: Build the Evidence Inventory

Goal: collect the exact adapter, diagnostic, map, and baseline evidence that C2
must compare.

Primary targets:

- Phase A2 and B2 docs
- closed ideas 201 through 206
- ownership, diagnostic, and residual consumer maps
- `test_baseline.log`

Concrete actions:

- Read every required artifact listed in `Read First`.
- Extract the Route 3 through Route 7 selected reader or diagnostic rows.
- Record the prepared surfaces touched and the proof scope claimed by each
  closure.
- Note whether the evidence is adapter correctness, oracle equivalence,
  consumer migration, or actual contraction evidence.

Completion check:

- `todo.md` names the evidence read and any missing or ambiguous artifact.
- No source idea or implementation file is changed.

### Step 2: Classify Route-Specific Surfaces

Goal: decide the contraction fate for each surface directly touched by Route 3
through Route 7.

Primary target:

- `docs/bir_prealloc_fusion/phase_c2_selected_adapter_cache_contraction_readiness.md`

Concrete actions:

- Create or update the C2 document with a surface-by-surface readiness table.
- For each route, record public consumer removal status, retained prepared
  fallback/oracle, retained target/prepared policy, proof scope, and readiness
  result.
- Separate adapter correctness from cache/API contraction readiness.

Completion check:

- The C2 document has explicit rows for Route 3, Route 4, Route 5, Route 6,
  and Route 7 surfaces.
- Each row has one readiness classification from the working model.

### Step 3: Classify Aggregate Coupling and Diagnostic Authority

Goal: decide whether the selected adapters change the retirement prospects for
`PreparedFunctionLookups`, `PreparedBirModule`, and prepared diagnostic/oracle
authority.

Primary target:

- `docs/bir_prealloc_fusion/phase_c2_selected_adapter_cache_contraction_readiness.md`

Concrete actions:

- Add aggregate findings for `PreparedFunctionLookups` coupling and
  `PreparedBirModule` retirement blockers.
- Add diagnostic/oracle replacement prerequisites, especially for Route 7
  comparison/provenance diagnostics and string-authority surfaces.
- Mark target-policy and prepared-policy surfaces as retained when policy or
  final emission authority still requires them.

Completion check:

- The C2 document explicitly states blockers to broad
  `PreparedFunctionLookups` and `PreparedBirModule` retirement.
- The diagnostic/oracle section distinguishes replacement prerequisites from
  contraction-ready surfaces.

### Step 4: Write Follow-Up Decisions and D2 Guidance

Goal: convert the audit findings into narrow next actions without expanding C2
into implementation work.

Primary targets:

- `docs/bir_prealloc_fusion/phase_c2_selected_adapter_cache_contraction_readiness.md`
- `ideas/open/` only if a ready one-surface micro-contraction follow-up is
  justified and delegated as part of this active plan

Concrete actions:

- List any accepted micro-contraction follow-up candidates one surface at a
  time.
- Explicitly list retained public fallback/oracle and retained target-policy
  surfaces.
- Recommend how D2 should be rewritten or opened based on retained-surface
  decisions.
- Do not open a broad D2 migration plan that ignores C2 retained-surface
  findings.

Completion check:

- The C2 document has final follow-up decisions and D2 guidance.
- Any new idea, if created, has concrete reviewer reject signals and stays
  limited to one cache/API surface.

### Step 5: Final Consistency and Proof Check

Goal: make the analysis closure reviewable and ready for supervisor acceptance.

Primary targets:

- `docs/bir_prealloc_fusion/phase_c2_selected_adapter_cache_contraction_readiness.md`
- `todo.md`

Concrete actions:

- Check that every required analysis item from the source idea appears in the
  C2 document.
- Check that no classification claims contraction from full-suite greenness or
  adapter success alone.
- Run the supervisor-delegated proof command if one is provided for the
  analysis/documentation slice.
- Record proof and remaining blockers in `todo.md`.

Completion check:

- The C2 document links back to the source idea and required artifacts.
- `todo.md` records the final proof command and result.
- The source idea can be judged for closure or a follow-up lifecycle decision
  by the plan owner.
