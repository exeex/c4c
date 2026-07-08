# Shared Prealloc Move/Operand Source Freshness Inventory Runbook

Status: Active
Source Idea: ideas/open/588_shared_prealloc_move_operand_source_freshness_inventory.md

## Purpose

Migrate the next shared-prealloc move or operand source consumers to the
freshness authority model introduced by idea 587, then leave a concrete
remaining-consumer inventory for closure.

Goal: audit the shared-prealloc consumers that still rely on local source
ordering, implicit prepared homes, or local fail-closed stack freshness checks;
wire a bounded representative subset through the shared freshness query; and
prove the migrated route rejects missing, ambiguous, stale, or wrong-use
freshness authority.

## Core Rule

Do not make progress by weakening expectations, changing unsupported markers,
adding named-testcase shortcuts, or accepting a source because a destination
bundle is legal. Each code-changing step must either publish/query shared
freshness authority from existing semantic prepared facts or preserve a precise
fail-closed result when that authority is absent or invalid.

## Read First

- ideas/open/588_shared_prealloc_move_operand_source_freshness_inventory.md
- ideas/closed/587_prepared_value_freshness_authority_mvp.md
- src/backend/prealloc/value_locations.hpp
- src/backend/prealloc/prepared_lookups.cpp
- src/backend/prealloc/publication_plans.cpp
- src/backend/prealloc/prepared_object_traversal.cpp
- tests/backend/bir/backend_prepared_lookup_helper_test.cpp
- tests/backend/bir/backend_prepare_stack_layout_test.cpp
- tests/backend/bir/backend_prepared_printer_test.cpp
- tests/backend/bir/backend_prepared_object_consumer_contract_test.cpp

## Current Targets

- Shared-prealloc consumers with existing source-freshness vocabulary,
  especially statuses such as `missing_stack_freshness`.
- Candidate consumer families:
  - dependency operand authorities
  - branch stack-load authorities
  - edge-publication move consumers
  - typed stack-source publications
  - select-carrier or select-alias operand authority when adjacent and small
- Existing freshness authority model and query surface from idea 587:
  `PreparedValueFreshnessAuthority`,
  `PreparedValueFreshnessUseKind`,
  `PreparedValueFreshnessSourceKind`,
  `PreparedValueFreshnessProofKind`,
  `PreparedValueFreshnessSourceRank`,
  `PreparedValueFreshnessQuery`, and
  `find_prepared_value_freshness_authority`.

## Non-Goals

- Do not migrate every AArch64, RV64, and x86 backend consumer.
- Do not replace every prepared value-home lookup in object emission.
- Do not change target triple parsing, `TargetProfile`, ABI semantic
  classification, or uniform target register identity policy.
- Do not redesign the freshness authority data model in this runbook. If the
  chosen consumer proves the model is insufficient, record the gap and split a
  separate idea.
- Do not broadly rewrite move-bundle scheduling, parallel-copy legality, call
  lowering, or target-local backend paths.
- Do not claim target backend progress unless the change is routed through
  shared prepared/prealloc authority or is explicitly documented as deferred.

## Working Model

The freshness authority from idea 587 is the source of truth for selected
prepared/prealloc value uses. This runbook should extend its consumer coverage
on the shared-prealloc side without inventing new target-local ownership.

For each candidate consumer, execution should answer:

- What value/use is being consumed?
- Which existing semantic prepared fact can publish source freshness?
- Which source kinds and use kinds are valid for that consumer?
- Which missing, ambiguous, invalid, stale, or wrong-use cases must fail closed?
- Which diagnostic, status, or prepared dump lets later agents distinguish
  unsupported shape from missing source freshness and local target
  incompleteness?

Prefer one or two representative migrations over a broad sweep. The closure
inventory is part of the deliverable, not cleanup.

## Execution Rules

- Keep each executor packet narrow enough for build plus focused prepared or
  backend tests.
- Begin with inventory before migration so the chosen subset is defensible.
- Publish freshness authority only from existing semantic prepared facts.
- Preserve existing fail-closed statuses unless a migrated consumer replaces
  them with an equally precise freshness-query result.
- Add tests or prepared dump assertions for both accepted explicit freshness
  and rejected missing/ambiguous/stale/wrong-use freshness.
- Track closure-inventory notes in `todo.md` as consumers are audited, wired,
  deferred, or split.
- Escalate to supervisor/reviewer if a code slice changes only expectations,
  unsupported markers, allowlists, diagnostic strings, or local ordering.
- Use build proof plus the focused test subset selected by the supervisor for
  each implementation packet; broader regression proof belongs to supervisor
  acceptance or close.

## Step 1: Audit Shared-Prealloc Freshness Consumers

Goal: identify the concrete shared-prealloc consumer set and choose the
smallest useful migration subset.

Primary targets:

- src/backend/prealloc/publication_plans.cpp
- src/backend/prealloc/prepared_object_traversal.cpp
- src/backend/prealloc/prepared_lookups.cpp
- src/backend/prealloc/value_locations.hpp
- tests/backend/bir/backend_prepare_stack_layout_test.cpp
- tests/backend/bir/backend_prepared_printer_test.cpp
- tests/backend/bir/backend_prepared_object_consumer_contract_test.cpp

Actions:

- Inventory every reachable shared-prealloc consumer that already reports
  source-freshness-related statuses or diagnostics, including dependency
  operands and branch stack-load authorities.
- Identify edge-publication move, typed stack-source, select-carrier, and
  select-alias routes that are close enough to inspect without broad rewrites.
- For each candidate, record whether it already has a producer/publication fact
  that can publish freshness authority from existing semantic data.
- Choose one bounded migration subset, preferring a consumer with existing
  `missing_stack_freshness` or equivalent fail-closed vocabulary and test
  anchors.
- Write the audit result and chosen subset into `todo.md` for closure
  inventory.

Completion check:

- `todo.md` names audited consumers, the selected migration subset, and the
  reason each unselected family is deferred, already protected, blocked on
  missing producer facts, or blocked on contract design.
- No implementation behavior has changed except incidental compile-safe
  discovery edits, if any.
- The next executor packet can start migration without redoing the audit.

## Step 2: Publish Freshness For The Selected Shared Consumer

Goal: expose source freshness facts for the selected shared-prealloc consumer
from existing prepared semantics.

Primary targets:

- the selected producer/publication or stack-source planning helper from Step 1
- src/backend/prealloc/value_locations.hpp only if a narrow use/source-kind
  vocabulary addition is unavoidable
- prepared printer or dump output only if needed to make authority observable

Actions:

- Add or reuse a freshness use kind and source kind that matches the selected
  consumer without broadening the model beyond the chosen route.
- Publish authority candidates from existing semantic prepared facts only.
- Record source reference, proof kind, and rank precisely enough for the shared
  query to reject invalid or ambiguous candidates.
- Preserve old fail-closed behavior until the migrated consumer checks the
  shared authority.
- Add focused lookup or dump coverage proving the authority is visible.

Completion check:

- The selected consumer's candidate freshness authority is visible in code and
  focused tests or prepared dumps.
- Missing or incomplete source facts do not create authority.
- Existing 587 freshness lookup and publisher tests continue to pass.

## Step 3: Wire The Selected Consumer To The Freshness Query

Goal: make the selected shared-prealloc consumer consult the shared freshness
query before accepting a source.

Primary targets:

- the selected shared-prealloc consumer from Step 1
- src/backend/prealloc/prepared_lookups.cpp if query behavior needs a narrow
  extension
- focused tests for the selected consumer family

Actions:

- Replace local implicit source acceptance with a query for the concrete
  value/use selected in Step 1.
- Accept only selected authority with the expected use kind, source kind, proof
  kind, and source reference.
- Fail closed for no-candidate, invalid-candidate, ambiguous-candidate,
  stale-source, wrong-use, and destination-only authority.
- Preserve existing target-independent validation for payload completeness,
  storage class, stack slot, register bank, and publication position.
- Keep diagnostics specific enough to distinguish missing freshness from
  unsupported shape and local target incompleteness.

Completion check:

- At least one shared-prealloc move or operand source consumer not wired by
  idea 587 now consults the shared freshness authority before accepting a
  source.
- Focused tests prove accepted explicit authority and rejected missing,
  ambiguous, stale, or wrong-use authority.
- The diff is semantic, not testcase-shaped matching or expectation weakening.

## Step 4: Strengthen Diagnostics And Debug Visibility

Goal: make the migrated path observable enough for later backend debugging and
closure inventory.

Primary targets:

- prepared printer, verifier, or diagnostic surfaces touched by the selected
  consumer
- focused prepared dump tests for the selected consumer route

Actions:

- Ensure selected freshness authority is printable or otherwise assertable in
  focused tests.
- Ensure fail-closed statuses distinguish unsupported shape, missing source
  freshness, ambiguous source freshness, stale/wrong-use freshness, and local
  target incompleteness where the selected route exposes those cases.
- Avoid broad diagnostic churn outside the selected consumer family.
- Add `todo.md` inventory notes for any exposed architecture gap.

Completion check:

- Focused dumps or diagnostics identify the migrated source authority and the
  relevant failure category.
- No unrelated diagnostic strings were changed as proof of progress.
- Closure inventory notes include newly exposed gaps, if any.

## Step 5: Recheck 587 Regression Anchors And Adjacent Shared Consumers

Goal: prove the new shared-prealloc migration did not break idea 587's existing
authority consumers and update the remaining-consumer map.

Primary targets:

- tests/backend/bir/backend_prepared_lookup_helper_test.cpp
- tests/backend/bir/backend_prepared_printer_test.cpp
- tests/backend/bir/backend_prepared_object_consumer_contract_test.cpp
- tests covering the selected shared-prealloc consumer

Actions:

- Run the focused test subset for the selected consumer and existing 587
  freshness routes.
- Recheck nearby shared-prealloc consumers from Step 1 for whether they remain
  protected, deferred, blocked on missing producer/publication facts, or blocked
  on a contract design issue.
- Record exact wired and unwired consumer families in `todo.md`.
- Open or request a separate source idea only if execution discovers a distinct
  initiative that should not remain in this runbook.

Completion check:

- Existing 587 call-argument, move-bundle source, and producer-publication
  tests continue to pass.
- The selected shared-prealloc tests pass.
- `todo.md` contains enough inventory detail to draft the required closure
  note without a fresh code audit.

## Step 6: Acceptance Inventory And Close-Readiness Review

Goal: decide whether the source idea is complete or whether the runbook should
be replaced, split, or followed by another idea.

Primary targets:

- todo.md closure-inventory notes
- ideas/open/588_shared_prealloc_move_operand_source_freshness_inventory.md
- changed implementation and test files from prior steps

Actions:

- Check every acceptance criterion from the source idea against the actual
  implementation and proof logs.
- Ensure the inventory explicitly answers all closure-note requirements from
  the source idea.
- Name concrete follow-up ideas if remaining consumer families are ready for
  separate work.
- Hand back to the supervisor for broader validation and any lifecycle close
  decision.

Completion check:

- The runbook can be either closed by the plan owner after regression guard or
  replaced/split with a concrete reason.
- Closure notes identify audited, wired, unwired, newly exposed, and follow-up
  consumer families.
- There is no accepted slice whose proof is only a narrow expectation rewrite,
  unsupported-marker edit, or target-local shortcut.
