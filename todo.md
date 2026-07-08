Status: Active
Source Idea Path: ideas/open/590_branch_stack_load_freshness_contract.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Closure Inventory and Follow-Up Decision

# Current Packet

## Just Finished

Completed Step 5, "Closure Inventory and Follow-Up Decision", from `plan.md`.

Closure inventory:

- Audited consumers: `PreparedBranchStackLoadRole::Condition`, `Lhs`, and
  `Rhs` around `plan_prepared_branch_stack_load_authority`,
  `collect_prepared_branch_stack_load_authorities`, and the adjacent frame-slot
  source fact collector. All three roles now map to the branch-specific
  freshness use kind `PreparedValueFreshnessUseKind::BranchStackLoadSource`;
  the accepted freshness source vocabulary is
  `PreparedValueFreshnessSourceKind::BranchStackSlot` with
  `PreparedValueFreshnessProofKind::BranchTerminatorOrdering` and
  `PreparedValueFreshnessSourceRank::BranchStackSlot`.
- Branch-point freshness ownership rule: a complete prepared stack home,
  frame slot, stack object, branch payload, and clobber-safety fact are
  necessary context but never sufficient authority. A branch stack source is
  accepted only when the shared freshness query selects a matching
  `BranchStackLoadSource` authority for the same prepared value/home and the
  exact branch terminator block/instruction point.
- Migrated representative consumer: the scalar `Condition` branch stack-load
  route is wired to publish/select branch stack-slot freshness and can reach
  `Available` only after selected freshness, clobber safety, and pointer-status
  checks pass. Prepared dump coverage shows the accepted `Condition` row with
  selected source-freshness fields.
- Fail-closed coverage: missing/no-candidate authority maps to
  `MissingSourceFreshnessAuthority`; duplicate same-rank candidates map to
  `AmbiguousSourceFreshnessAuthority`; destination-only or wrong-proof
  candidates map to `InvalidSourceFreshnessAuthority`; selected authority with
  the wrong stack home maps to `UnsupportedSourceFreshnessAuthority`; wrong
  value, wrong use, stale terminator point, and future terminator point are
  ignored and therefore remain `MissingSourceFreshnessAuthority`; missing
  clobber safety maps to `MissingStackClobberSafety`; stack-home-only or
  policy-none rows remain fail-closed at `MissingPolicy`.
- Remaining unwired/protected/deferred consumers: pointer `Lhs` and `Rhs`
  stack-home rows are still collected for inventory visibility but deliberately
  stay `policy=none`/`MissingPolicy`; they are protected from accidental target
  emission by not receiving source-freshness candidates or a load policy in
  this runbook. Full branch/select/edge-publication consumers and target
  emission consumers are deferred by source-idea scope rather than blocked by
  this scalar condition migration.
- Typed and aggregate stack-source producer facts should become a follow-up
  idea, not be folded into this closure. The next idea should first define how
  typed/aggregate stack-source producers publish `BranchStackSlot` freshness at
  branch terminator points before broadening more consumers.
- Target-specific tail: no RV64, AArch64, or x86 emission files were changed in
  this runbook, and no target-specific tail is ready without the typed/aggregate
  producer-publication contract. After that contract exists, a narrow follow-up
  can migrate pointer `Lhs`/`Rhs` or target branch emission paths using the same
  branch-point freshness query.

## Suggested Next

Ask the plan owner to use this inventory for source-idea close/deactivate
review. Recommended follow-up ideas:

- Define typed/aggregate branch stack-source producer/publication facts that
  can publish `BranchStackSlot` freshness at the exact branch terminator point.
- After that producer contract exists, migrate pointer `Lhs`/`Rhs` branch
  stack-load consumers from inventory-only `policy=none` rows to selected
  branch freshness authority.
- Then consider RV64/AArch64/x86 branch-emission follow-up only if those
  target paths can consume the shared branch freshness authority without
  target-local inference.

## Watchouts

- This is close-ready for the current runbook but not source-idea closure by
  itself; lifecycle close/deactivate belongs to the plan owner.
- Do not describe pointer `Lhs`/`Rhs`, select, edge-publication, RV64, AArch64,
  or x86 consumers as migrated by this runbook. They are explicitly deferred.
- The next route should avoid target-local freshness inference; source
  producer/publication facts need to be shared-prealloc facts first.

## Proof

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`
passed. Proof log: `test_after.log` (`346` backend tests passed).
