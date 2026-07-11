# Prepared MIR Source Dependency Freshness View Contract

Status: Open
Type: Implementation idea
After: `ideas/closed/684_bir_mir_interface_cleanup_from_prepared_mir_view_docs.md`
Parent: `ideas/closed/683_prepared_mir_view_contract_research.md`
Consumes:
- `docs/prepared_mir_view_contract_research/03_feature_views_and_optional_contracts.md`
- `docs/prepared_mir_view_contract_research/04_debug_proof_and_diagnostic_boundaries.md`
- `docs/prepared_mir_view_contract_research/07_open_questions_and_followup_implementation_ideas.md`
Related:
- `ideas/closed/589_direct_edge_publication_move_freshness_ownership.md`
- `ideas/closed/590_branch_stack_load_freshness_contract.md`
- `ideas/closed/684_bir_mir_interface_cleanup_from_prepared_mir_view_docs.md`
- `src/backend/mir/prepared_view.hpp`
- `src/backend/mir/x86/`
Owning Layer: Prepared MIR source/freshness view boundary

## Goal

Design and implement the first typed source/freshness prepared MIR view
contract that exposes selected lowering authority while keeping missing,
invalid, stale, or explanatory proof state as verifier or diagnostic facts.

## Why This Exists

The prepared MIR core view deliberately avoids making raw publication rows,
stack homes, move bundles, route names, or debug text into semantic lowering
authority. Closed ideas 589 and 590 established representative freshness
ownership rules for direct edge-publication moves and branch stack-load
sources. The MIR-facing view needs a stable way to expose those selected facts
without letting targets rediscover authority from raw prepared internals.

## In Scope

- Define the narrow source/freshness view shape for one selected authority
  family, preferably a family already governed by ideas 589 or 590.
- Expose selected source identity and freshness facts as typed lowering inputs
  only when the producer fact is present and valid.
- Expose missing, ambiguous, stale, wrong-use, wrong-value, or
  destination-only outcomes as verifier/admission facts, not lowering
  authority.
- Keep route names, proof paths, debug summaries, and checker explanation text
  observational.
- Migrate one low-risk x86 consumer path or add structural tests that prove the
  view fails closed before target lowering observes an incomplete source fact.

## Out Of Scope

- Reopening ideas 589 or 590.
- Replacing all publication/source dependency producers.
- Broad target migrations or deleting raw prepared fields.
- Treating stack homes, move bundles, destination legality, route names, or
  debug text as standalone source freshness authority.
- Changing expectations, unsupported markers, allowlists, runtime behavior,
  diagnostics, timeout policy, or baseline acceptance policy.

## Acceptance Criteria

- A typed source/freshness view contract exists for the selected authority
  family.
- Valid selected source/freshness facts are available to the chosen consumer or
  comparator/test surface through the view.
- Missing or invalid source authority is represented as fail-closed
  verifier/admission state and is not usable as lowering authority.
- Focused tests or backend proof cover both accepted and rejected authority
  paths for the selected family.
- Fresh build plus focused backend tests pass.

## Reviewer Reject Signals

- Reject lowering code that accepts source authority from stack-home shape,
  move-bundle completeness, destination legality, raw publication rows, route
  names, or rendered debug text.
- Reject moving diagnostic/proof certificates into semantic codegen authority.
- Reject a view that simply exposes all publication/source dependency internals
  without typed admission semantics.
- Reject testcase-shaped shortcuts, named-case-only handling, expectation
  rewrites, unsupported downgrades, allowlist filtering, or baseline-policy
  changes claimed as freshness-view progress.
- Reject broad producer rewrites or all-target migrations hidden inside the
  first source/freshness contract slice.
- Reject any route that reintroduces the failure modes closed by ideas 589 or
  590 behind a new view name.
