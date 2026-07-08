# Typed/Aggregate Branch Stack-Source Publication

Status: Closed
Type: Architecture contract and narrow producer migration
Parent: `ideas/closed/590_branch_stack_load_freshness_contract.md`
Related:
- `ideas/closed/587_prepared_value_freshness_authority_mvp.md`
- `ideas/closed/588_shared_prealloc_move_operand_source_freshness_inventory.md`
- `ideas/closed/589_direct_edge_publication_move_freshness_ownership.md`
- `ideas/closed/590_branch_stack_load_freshness_contract.md`
Owning Layer: shared-prealloc branch stack-source producer facts and
branch-terminator freshness publication

## Goal

Define how typed and aggregate branch stack-source producers publish
`PreparedValueFreshnessSourceKind::BranchStackSlot` freshness at the exact
branch terminator point, then migrate the narrowest blocked branch stack-load
consumer that depends on those facts.

## Why This Exists

Idea 590 established the branch-point freshness ownership rule and migrated the
scalar condition branch stack-load route to selected shared freshness authority.
It deliberately left pointer `Lhs` and `Rhs` rows inventory-only because they
need typed or aggregate stack-source producer/publication facts before they can
receive branch stack-slot freshness without target-local inference.

Without this producer contract, widening branch stack-load freshness to pointer
operands, aggregate-adjacent sources, or target branch emission would either
invent freshness at the consumer or rely on structural stack-home completion.

## In Scope

- Audit typed and aggregate stack-source producer paths that can feed branch
  condition, `Lhs`, or `Rhs` stack-load consumers.
- Define which producer facts are allowed to publish
  `PreparedValueFreshnessSourceKind::BranchStackSlot` for
  `PreparedValueFreshnessUseKind::BranchStackLoadSource`.
- Require publication at the exact branch terminator block/instruction point,
  using `PreparedValueFreshnessProofKind::BranchTerminatorOrdering` or a
  semantically equivalent branch-point proof if the current proof kind is
  insufficient.
- Migrate one blocked consumer, preferably pointer `Lhs` or `Rhs`, from
  inventory-only `policy=none` to selected branch freshness authority.
- Add focused proof for accepted typed or aggregate branch stack-source
  freshness and fail-closed cases for missing, ambiguous, stale, wrong-value,
  wrong-use, future-point, and stack-home-only authority.

## Out Of Scope

- Broad migration of every branch, select, edge-publication, RV64, AArch64, or
  x86 consumer.
- Target-local freshness inference in branch emission.
- Redesigning control-flow lowering, instruction selection, terminator
  emission, ABI classification, or physical register identity policy.
- Treating prepared homes, frame slots, aggregate lanes, or clobber-safety
  facts as freshness by themselves.
- Expectation rewrites, unsupported-marker edits, allowlist edits, or runtime
  output changes as proof of capability progress.

## Acceptance Criteria

- The implementation identifies the typed and aggregate producer paths that can
  publish branch stack-source freshness.
- The producer/publication contract states which facts may create
  `BranchStackSlot` freshness for the `BranchStackLoadSource` use.
- At least one previously blocked pointer or aggregate-adjacent branch
  stack-load consumer consults selected shared freshness authority before
  accepting a source.
- Missing, ambiguous, stale, wrong-value, wrong-use, future-point, and
  stack-home-only authority fail closed with visible status or diagnostics.
- Focused tests or prepared dumps prove the accepted route is authorized by
  explicit producer-published branch freshness.
- Existing 587, 588, 589, and 590 freshness tests continue to pass.

## Reviewer Reject Signals

- Reject a slice that lets pointer `Lhs`, pointer `Rhs`, or an aggregate
  branch stack source become available only because a stack home, frame slot,
  aggregate lane, or clobber-safety fact exists.
- Reject target-local branch emission changes that infer branch freshness
  instead of consuming shared-prealloc producer-published freshness.
- Reject helper renames, expectation rewrites, unsupported-marker edits, or
  classification-only changes claimed as producer/publication progress.
- Reject testcase-shaped matching for a named branch case instead of a semantic
  producer rule tied to the branch terminator point.
- Reject broad target or select/edge rewrites before the typed/aggregate
  branch stack-source publication contract is explicit and proven.
- Reject retaining the exact idea-590 blocked state for pointer `Lhs`/`Rhs`
  behind a new abstraction name.

## Closure Notes

Closed after the active runbook completed Step 5 closure inventory.

Completed:
- Audited branch stack-load producer paths through
  `collect_prepared_branch_stack_load_authorities`,
  `collect_branch_stack_load_authority_for_role`,
  `make_branch_stack_load_authority_record`, and
  `plan_prepared_branch_stack_load_authority`.
- Defined producer publication through
  `PreparedBranchStackSourceFreshnessPublicationInputs` and
  `publish_prepared_branch_stack_source_freshness_candidate`, requiring the
  matching source value, `BranchStackLoadSource` use, `BranchStackSlot`
  source, `BranchTerminatorOrdering` proof, `BranchStackSlot` rank, stack-slot
  home, and exact branch block plus terminator instruction point.
- Migrated pointer `PreparedBranchStackLoadRole::Lhs` from inventory-only
  `policy=none` to selected `LoadFromStackSlot` authority gated by explicit
  producer-published branch stack-slot freshness.
- Preserved fail-closed behavior for missing, ambiguous, stale, wrong-value,
  wrong-use, future-point, and stack-home-only authority.

Deliberately left for follow-up ideas:
- Pointer `Rhs`, aggregate-adjacent consumers, select consumers,
  edge-publication consumers, RV64/AArch64/x86 target consumption or emission,
  and target-local branch freshness inference remain out of scope.
- Existing follow-ups
  `ideas/open/593_rv64_branch_stack_source_freshness_consumption.md` and
  `ideas/open/594_rv64_branch_stack_source_consumption_followup_from_593.md`
  cover the RV64 consume-side queue after this producer contract.

Closure validation:
- Backend regression guard used canonical `test_before.log` and
  `test_after.log`; both logs report 346/346 passing backend tests, and
  non-decreasing regression comparison passed with no new failures.
- Supervisor-provided validation context recorded hook-managed full-suite
  baselines accepted at 3375/3375 after the code commits.
