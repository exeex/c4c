Status: Active
Source Idea Path: ideas/open/590_branch_stack_load_freshness_contract.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Branch-Point Freshness Ownership

# Current Packet

## Just Finished

Completed Step 2, "Define Branch-Point Freshness Ownership", from `plan.md`.

Encoded the ownership rule for branch stack-load source freshness:

- Narrow use kind: `PreparedValueFreshnessUseKind::BranchStackLoadSource`.
- Accepted source kind: `PreparedValueFreshnessSourceKind::BranchStackSlot`
  only.
- Required proof/rank tuple:
  `PreparedValueFreshnessProofKind::BranchTerminatorOrdering` plus
  `PreparedValueFreshnessSourceRank::BranchStackSlot`.
- Ordering point: the freshness authority must carry the exact branch
  terminator `block_index` and `instruction_index` requested by the query.
- API surface: `prepared_branch_stack_load_freshness_use_kind` maps
  `Condition`, `Lhs`, and `Rhs` to the branch-specific freshness use, and
  `PreparedBranchStackLoadAuthorityInputs::stack_slot_fresh_at_branch`
  documents that stack-home structure alone is not freshness authority.

The shared freshness lookup now rejects branch stack-slot authority for
non-branch uses, rejects destination-only/direct-home authority for branch
stack loads, rejects wrong proof/rank/source tuples, and ignores stale or
wrong-use candidates. Collector acceptance is intentionally unchanged:
collector rows still call `plan_prepared_branch_stack_load_authority` with
`policy=none` and remain inventory-only until Step 3 wiring.

## Suggested Next

Execute Step 3 from `plan.md`: wire the selected scalar `Condition`
branch-stack-load collector row to query `BranchStackLoadSource` freshness at
the branch terminator point and require selected authority before setting
`LoadFromStackSlot`/`stack_slot_fresh_at_branch` for that route.

## Watchouts

- Step 3 should wire only the selected scalar `Condition` collector route; do
  not migrate pointer `Lhs`/`Rhs`, frame-slot source facts, target emission, or
  typed/aggregate producer facts in the same packet.
- Structural facts remain necessary but insufficient: stack home, frame slot,
  stack object, branch payload, and clobber safety must not imply freshness
  without a selected `BranchStackLoadSource` authority.
- Keep expectation files, unsupported markers, allowlists, and runtime-output
  contracts unchanged.
- `clang-format` was not available in this environment; edits were manually
  checked and compiled.

## Proof

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`
passed on rerun after a serial `cmake --build --preset default -j1` completed
objects that the first exact build attempt lost to `cc1plus` OOM kills. Proof
log: `test_after.log`.
