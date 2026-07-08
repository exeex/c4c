# Branch Stack-Load Freshness Contract

Status: Closed
Type: Architecture contract and narrow consumer migration
Parent: `ideas/closed/588_shared_prealloc_move_operand_source_freshness_inventory.md`
Depends-On: `ideas/open/589_direct_edge_publication_move_freshness_ownership.md`
Related:
- `ideas/closed/585_target_abi_contract_and_value_consumption_research.md`
- `ideas/closed/587_prepared_value_freshness_authority_mvp.md`
- `ideas/closed/588_shared_prealloc_move_operand_source_freshness_inventory.md`
- `ideas/open/589_direct_edge_publication_move_freshness_ownership.md`
Owning Layer: shared-prealloc branch stack-load authorities, branch-point
freshness contract, and source-consumer migration

## Goal

Define the freshness contract for branch stack-load authorities and migrate one
bounded representative branch stack-load route to the shared freshness query.

This idea should run after idea 589. Idea 589 settles direct
edge-publication move freshness ownership; this idea then addresses branch
condition/lhs/rhs stack-loaded sources at the branch point. It should not try
to solve typed or aggregate stack-source producer facts; those should be
reconsidered after this closure inventory.

## Why This Exists

Idea 588 migrated dependency operand `LoadFromStackSlot` to explicit freshness
authority and left `plan_prepared_branch_stack_load_authority` unwired. The
remaining question is whether a stack-loaded branch source is fresh at the
branch terminator point, not merely whether its stack home and load payload are
structurally complete.

For branch consumers, the relevant use point is sharper than ordinary operand
planning:

- branch condition, lhs, or rhs must be available before the branch
  terminator;
- a stack home can be complete but stale relative to a later producer or
  publication;
- branch lowering must not accept a source only because a stack slot exists;
- missing, ambiguous, stale, or wrong-use freshness should fail closed before
  target emission tries to infer intent.

## In Scope

- Audit branch stack-load consumers and helper APIs around
  `plan_prepared_branch_stack_load_authority`.
- Define the branch-point freshness ownership rule:
  - which use kind is queried for branch stack loads;
  - whether the use represents branch condition, branch lhs/rhs, or a shared
    branch stack-load source;
  - which source kinds are accepted;
  - what program point or ordering fact proves freshness before the branch
    terminator;
  - which structural stack-home facts remain necessary but insufficient.
- Add or reuse the narrowest freshness use/source vocabulary needed for the
  branch stack-load route. If existing 587/589 vocabulary is insufficient, add
  a branch-specific use kind instead of overloading dependency or
  edge-publication terminology.
- Migrate one representative branch stack-load authority route to require
  selected freshness authority before accepting the source.
- Preserve fail-closed behavior for missing, ambiguous, stale, wrong-value,
  wrong-use, future-producer, or stack-home-only authority.
- Add focused tests and/or prepared dump assertions proving:
  - accepted branch stack loads carry explicit freshness authority;
  - missing or invalid freshness keeps the route fail-closed;
  - a structurally complete stack home alone is not enough;
  - diagnostics/printer output make the selected or missing freshness visible.

## Out Of Scope

- Typed or aggregate stack-source freshness producer/publication facts. Decide
  whether to open that follow-up only after this idea closes.
- Full migration of all branch, select, edge-publication, RV64, AArch64, or x86
  consumers.
- Redesigning control-flow lowering, branch instruction selection, or
  terminator fragment emission.
- Treating a stack object, prepared home, or clobber-safety fact as freshness
  by itself.
- Changing target ABI classification, `TargetProfile`, or physical register
  identity policy.
- Expectation rewrites, unsupported-marker edits, allowlist edits, or runtime
  output changes as proof of progress.

## Acceptance Criteria

- The implementation identifies the audited branch stack-load consumer set.
- The implementation states a concrete branch-point freshness ownership rule.
- At least one branch stack-load route consults the shared freshness authority
  before accepting a source.
- The migrated route rejects freshness-less, stale, ambiguous, wrong-value,
  wrong-use, or future-producer authority with precise fail-closed status or
  diagnostics.
- Focused tests or prepared dumps prove the accepted route is authorized by
  explicit source freshness, not merely by a complete stack home or local
  branch payload.
- Existing 587 and 588 freshness tests continue to pass, and any idea 589
  freshness tests continue to pass if 589 has already closed.
- The closure note states whether typed/aggregate stack-source producer facts
  should become the next idea or remain deferred.

## Closure Note Requirements

Do not close this idea with a generic "future work remains" note. The closure
note must answer:

1. Which branch stack-load consumers were audited?
2. What is the branch-point freshness ownership rule?
3. Which freshness use kinds and source kinds were used or added?
4. Which representative branch stack-load consumer was migrated?
5. Which missing, ambiguous, stale, wrong-value, wrong-use, future-producer, or
   stack-home-only cases now fail closed, and through what diagnostics or
   statuses?
6. Which branch stack-load consumers remain unwired, and are they already
   protected, blocked on missing producer/publication facts, blocked on
   contract design, or deferred for scope?
7. Did the work expose a new architecture gap not already covered by ideas
   585, 587, 588, or 589?
8. Did the work expose any target-specific RV64, AArch64, or x86 tail that is
   ready for a follow-up idea?
9. Should typed or aggregate stack-source freshness producer facts become the
   next idea, or should another branch/edge/shared-prealloc migration happen
   first?
10. What concrete follow-up ideas should be opened next, if any?

## Recommended First Cut

Start from the branch stack-load path that already has a prepared stack object,
dependency or value identity, clobber-safety checks, and a fail-closed
`missing_stack_freshness` style status. Prefer one representative route over a
broad sweep of all branch forms.

If idea 589 adds a new edge-publication freshness use kind or establishes a
reusable source-ownership pattern, reuse that pattern only where it fits the
branch terminator use point. If the branch point needs a distinct use kind,
name it explicitly.

Do not fold typed or aggregate stack-source producer facts into this idea just
because they appear nearby. Record them in the closure inventory and decide
after 590 whether they are ready for a narrow implementation idea or need a
separate design discussion.

## Reviewer Reject Signals

- Reject a slice that accepts a branch stack source because the stack home is
  complete while freshness is absent.
- Reject overloading dependency, publication, or move freshness terminology if
  branch-point freshness needs its own use kind.
- Reject target-local branch lowering tweaks presented as shared-prealloc
  freshness migration.
- Reject a route that only changes diagnostics or expectations without moving
  a real branch stack-load consumer to freshness authority.
- Reject closure notes that do not answer whether typed/aggregate stack-source
  producer facts should be next or deferred.

## Closure Note

Closed after the active runbook completed Steps 1-5 and backend regression
proof passed for the shared-prealloc/backend freshness scope:

```text
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1
```

The close gate compared canonical backend logs with the regression guard in
non-decreasing mode because the supervisor had already rolled the green
executor proof forward: `test_before.log` and `test_after.log` both report
346 passed, 0 failed.

1. Audited consumers: `PreparedBranchStackLoadRole::Condition`, `Lhs`, and
   `Rhs` around `plan_prepared_branch_stack_load_authority`,
   `collect_prepared_branch_stack_load_authorities`, and the adjacent frame
   slot source fact collector.
2. Branch-point freshness ownership rule: a prepared stack home, frame slot,
   stack object, branch payload, and clobber-safety fact are necessary context
   but never sufficient authority. A branch stack source is accepted only when
   the shared freshness query selects matching authority for the same prepared
   value/home at the exact branch terminator block/instruction point.
3. Freshness vocabulary: the audited roles map to
   `PreparedValueFreshnessUseKind::BranchStackLoadSource`, using
   `PreparedValueFreshnessSourceKind::BranchStackSlot`,
   `PreparedValueFreshnessProofKind::BranchTerminatorOrdering`, and
   `PreparedValueFreshnessSourceRank::BranchStackSlot`.
4. Migrated representative consumer: the scalar `Condition` branch stack-load
   route now publishes/selects branch stack-slot freshness and can become
   `Available` only after selected freshness, clobber safety, and pointer
   status checks pass.
5. Fail-closed coverage: missing or no-candidate authority reports
   `MissingSourceFreshnessAuthority`; duplicate same-rank candidates report
   `AmbiguousSourceFreshnessAuthority`; destination-only or wrong-proof
   candidates report `InvalidSourceFreshnessAuthority`; selected authority
   with the wrong stack home reports `UnsupportedSourceFreshnessAuthority`;
   wrong value, wrong use, stale terminator point, and future terminator point
   are ignored and remain `MissingSourceFreshnessAuthority`; missing clobber
   safety reports `MissingStackClobberSafety`; stack-home-only or policy-none
   rows remain fail-closed at `MissingPolicy`.
6. Remaining consumers: pointer `Lhs` and `Rhs` stack-home rows are still
   collected for inventory visibility but deliberately stay
   `policy=none`/`MissingPolicy`. They are protected from accidental target
   emission because this runbook did not grant source-freshness candidates or
   load policy to those rows. Full branch, select, edge-publication, and target
   emission consumers are deferred by scope.
7. New architecture gap: typed and aggregate branch stack-source producer facts
   must define how they publish `BranchStackSlot` freshness at branch
   terminator points before more consumers can be widened.
8. Target-specific tail: no RV64, AArch64, or x86 emission files were changed,
   and no target-specific follow-up is ready until the typed/aggregate
   producer-publication contract exists.
9. Next direction: typed and aggregate stack-source freshness producer facts
   should be the next idea before pointer `Lhs`/`Rhs`, select, edge, or target
   branch-emission migration.
10. Concrete follow-up opened:
    `ideas/open/592_typed_aggregate_branch_stack_source_publication.md`.
