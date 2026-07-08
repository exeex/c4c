# Pointer Base Plus Offset Selected Authority Runbook

Status: Active
Source Idea: ideas/open/599_pointer_base_plus_offset_selected_authority.md
Activated After:
- ideas/closed/597_pointer_address_semantic_model_research.md
- ideas/closed/598_select_carrier_alias_freshness_contract.md

## Purpose

Define selected authority for pointer base plus offset facts before any
consumer treats a computed pointer home as semantically current.

## Goal

Make one representative pointer-base-plus-offset route require explicit
selected authority that ties base pointer freshness, result pointer identity,
byte delta, use kind, and program point together.

## Core Rule

Do not accept a computed pointer use because the home shape, byte delta,
stack/register placement, range proof, target offset encodability, or final
target operand shape exists. A migrated consumer must fail closed unless the
selected pointer-arithmetic authority for the exact use is satisfied.

## Read First

- ideas/open/599_pointer_base_plus_offset_selected_authority.md
- docs/pointer_address_semantic_model_research/01_pointer_address_family_inventory.md
- docs/pointer_address_semantic_model_research/02_semantic_authority_and_fact_classes.md
- docs/pointer_address_semantic_model_research/03_fail_closed_rules.md
- docs/pointer_address_semantic_model_research/05_followup_recommendations.md
- ideas/closed/587_prepared_value_freshness_authority_mvp.md
- ideas/closed/597_pointer_address_semantic_model_research.md

## Current Targets

- `src/backend/prealloc/value_locations.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/prealloc/prepared_contract_verifier.*`
- `src/backend/prealloc/decoded_home_storage.*`
- `src/backend/prealloc/prepared_printer/*`
- `src/backend/mir/riscv/codegen/prepared_edge_publication_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_local_memory_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_scalar_emit.cpp`
- `src/backend/mir/aarch64/codegen/operands.cpp`
- `tests/backend/bir/backend_prealloc_decoded_home_storage_test.cpp`
- `tests/backend/bir/backend_prealloc_prepared_contract_verifier_test.cpp`
- `tests/backend/bir/backend_prepare_stack_layout_test.cpp`
- `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`

## Non-Goals

- Do not implement pointer-value indirect memory-use freshness.
- Do not implement semantic GEP target consumption.
- Do not solve relocation or materialization semantics.
- Do not migrate broad RV64, AArch64, x86, or string assembly consumers.
- Do not redesign target memory operand formation, regalloc, or BIR lowering.
- Do not change expectations, unsupported markers, allowlists, runtime
  behavior, harness behavior, or final assembly text as proof.

## Working Model

- `PreparedValueHomeKind::PointerBasePlusOffset` and
  `PreparedPointerBasePlusOffsetFact` are support/target-consume evidence
  until selected pointer-arithmetic authority exists.
- The selected authority must name the base pointer freshness, result pointer
  identity, byte delta, use kind, proof, reference/program point, and rank.
- Missing, ambiguous, stale, wrong-base, wrong-result, wrong-delta, wrong-use,
  range-only, target-shape-only, and diagnostic-only evidence must fail
  closed before semantic acceptance.
- Targets may later consume a selected result, but target offset encodability
  is not authority.

## Execution Rules

- Start with audit and contract definition before adding acceptance behavior.
- Migrate at most one representative consumer in this runbook.
- Prefer shared prepared/prealloc authority helpers and freshness lookup
  vocabulary over target-local shape checks.
- Keep pointer-value memory-use freshness in idea 600, not this plan.
- If the audit proves the first consumer depends on MIR-view design or broad
  target migration, record that in `todo.md` and stop for lifecycle review.
- For code-changing steps, run the build proof selected by the supervisor and
  the narrow tests touched by the packet.

## Step 1. Audit Pointer-Base-Plus-Offset Consumers

Goal: identify the live producers, support facts, and consumers around
pointer-base-plus-offset homes.

Actions:

- Inspect `PreparedValueHomeKind::PointerBasePlusOffset`,
  `PreparedPointerBasePlusOffsetFact`, and
  `as_pointer_base_plus_offset_fact(...)`.
- Inspect verifier, decoded-home storage, publication hook, prepared printer,
  and prepared lookup surfaces that classify or expose this home shape.
- Inspect RV64, AArch64, and x86 consumers only enough to classify them as
  representative candidates, target-consume-only paths, rejections, or
  deferred target migrations.
- Choose one representative shared prepared/prealloc consumer route, or record
  why implementation should stop at contract/audit state.

Completion Check:

- `todo.md` names the audited consumer set, the proposed representative route,
  and which target paths remain out of scope.

## Step 2. Define Pointer-Arithmetic Authority

Goal: make the selected authority dimensions explicit before changing any
consumer acceptance.

Actions:

- Decide the freshness use kind, source kind, proof kind, and rank for
  pointer-base-plus-offset selected authority.
- State how base pointer freshness, result pointer identity, byte delta, use
  kind, and program point are represented in the query/reference.
- Define which support facts remain insufficient by themselves: home shape,
  byte delta, stack/register placement, range proof, target offset
  encodability, target operand shape, and diagnostics.
- Define fail-closed statuses or diagnostics for missing, ambiguous, stale,
  wrong-base, wrong-result, wrong-delta, wrong-use, range-only, and
  target-shape-only authority.

Completion Check:

- The ownership rule is recorded in `todo.md` and, if implementation proceeds,
  encoded in names/helpers that cannot be confused with branch,
  edge-publication, move-bundle, select-carrier, or pointer-value memory-use
  freshness.

## Step 3. Migrate One Representative Consumer

Goal: require selected pointer-arithmetic authority before one representative
consumer accepts the computed pointer as semantically current.

Actions:

- Add or reuse the minimal freshness vocabulary required by Step 2.
- Publish, collect, or query the selected authority from existing semantic
  prepared facts only.
- Require the selected authority before accepting the representative
  pointer-base-plus-offset source or result.
- Preserve rejection for missing, ambiguous, stale, wrong-base, wrong-result,
  wrong-delta, wrong-use, range-only, target-shape-only, and structurally
  complete but freshness-less evidence.
- Keep target-wide migration and pointer-value memory freshness out of this
  step.

Completion Check:

- The migrated consumer accepts only when selected authority matches the exact
  base/result/delta/use/proof/reference/rank/program point required by the
  contract.

## Step 4. Prove Fail-Closed Behavior

Goal: prove the migrated route uses explicit selected authority and not
support or target-shape facts.

Actions:

- Add focused tests or prepared dump assertions for accepted explicit
  pointer-base-plus-offset authority.
- Cover missing/no-candidate, ambiguous, stale, wrong-base, wrong-result,
  wrong-delta, wrong-use, range-only, target-shape-only, and
  freshness-less support facts where practical.
- Keep diagnostics precise enough to distinguish missing pointer-arithmetic
  authority from unrelated unsupported route failures.
- Re-run existing freshness authority tests or the closest repo-native subset
  that covers 587 and the touched prepared/prealloc bucket.

Completion Check:

- Fresh proof shows `PreparedValueHomeKind::PointerBasePlusOffset`,
  `PreparedPointerBasePlusOffsetFact`, stack/register homes, range facts, and
  target-encodable offsets remain insufficient without selected authority.

## Step 5. Closure Inventory And Follow-Up Decision

Goal: prepare source-idea closure without broadening this route.

Actions:

- Record audited consumers and the selected pointer-arithmetic ownership rule.
- Identify which representative consumer was migrated, if any.
- Identify which stale, wrong-value, wrong-delta, wrong-use, range-only, and
  target-shape-only cases fail closed.
- Record adjacent families that remain separate: pointer-value memory-use
  freshness, semantic GEP target consumption, relocation/materialization, and
  broad target migration.
- Run the supervisor-selected broader validation or regression guard before
  closure.

Completion Check:

- `todo.md` contains the closure inventory needed to answer every closure-note
  requirement from the source idea.

## Acceptance Check

- The audited pointer-base-plus-offset consumer set is recorded.
- A selected pointer-arithmetic authority rule names base freshness, result
  identity, byte delta, use kind, proof, rank, and program point.
- At most one representative consumer is migrated to require selected
  authority before semantic acceptance.
- Missing, ambiguous, stale, wrong-base, wrong-result, wrong-delta, wrong-use,
  range-only, target-shape-only, and support-only evidence fail closed.
- Focused proof demonstrates support facts and target offset encodability do
  not substitute for selected authority.
