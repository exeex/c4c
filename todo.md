Status: Active
Source Idea Path: ideas/open/700_prepared_mir_stack_view_contract.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Record Residual Stack Revisit Eligibility

# Current Packet

## Just Finished

Step 4 is satisfied for the selected `PreparedBranchStackLoadAuthority`
family. No additional test change is needed for this proof step.

Existing accepted proof demonstrates the required positive surface above route
dumps: `backend_prepare_stack_layout` covers available condition, lhs, and rhs
branch stack-load authority rows plus prepared-printer exposure for
`branch_stack_load_authority`; the prepared MIR query/view exposes that same
authority through `query_prepared_mir_branch_stack_load_authority()` and
`PreparedMirFunctionView::branch_stack_load_authority()`; and the RV64 object
path consumes branch stack-load freshness through that named prepared MIR
authority boundary, including partial prepared snapshots that have authority
records without a full core view.

Existing accepted proof also demonstrates the negative fail-closed surface:
`check_branch_stack_load_authority_contract()` covers missing, ambiguous,
stale, wrong-value, wrong-use, stack-home-only, unsupported-home, same-slot
clobber, unpreserved call, mismatched preserved slot, and stale preserved
source rejection statuses. The query returns `available=true` only for
`PreparedBranchStackLoadAuthorityStatus::Available` records with selected
`BranchStackLoadSource` freshness backed by `BranchStackSlot` and
`BranchTerminatorOrdering`; missing, unsupported, ambiguous, invalid, stale, or
clobbered authority remains unavailable.

Route-numbered evidence is compatibility-only: Route 4, Route 5, Route 7,
`RouteIndexReferenceFacade`, route dumps, expectation rows, and allowlists may
explain old agreement or diagnostics, but they must not authorize MIR branch
stack-load operands. The selected family must be accepted only through
`PreparedBranchStackLoadAuthorityStatus::Available` rows with selected
`PreparedValueFreshnessUseKind::BranchStackLoadSource` freshness at the branch
terminator point.

Step 4 proof decision: satisfied by existing positive and negative proof. No
precise missing proof was found for the selected branch stack-load authority
family.

## Suggested Next

Delegate Step 5: record residual stack revisit eligibility. Compare the
prepared branch stack-load authority evidence against the prerequisite
threshold for ideas 647 and 655, then record whether those residual stack ideas
remain parked or can be revisited by a later lifecycle packet.

## Watchouts

- Do not use Route 4, Route 5, Route 7, route facade status, dump rows,
  expectations, or allowlists as MIR authority.
- Do not reactivate ideas 647 or 655 until positive prepared producer evidence
  exists above route dumps.
- Keep route facade contraction and dump vocabulary cleanup out of this packet.
- Keep the selected family exact: branch stack-load authority only. Do not fold
  aggregate stack-source, direct-edge publication source, generic value-home,
  or move-bundle authority into the next packet.
- Step 5 should not treat this branch stack-load authority slice as evidence
  for unrelated residual stack prerequisites such as destination value
  identity, destination home, storage kind, source value/home, move bundle or
  move resolution, aggregate stack source, or explicit destination authority.
- Branch stack-load authority is positive prepared evidence for its own family
  only: no `Available` MIR view result exists unless the prepared record is
  `Available` and its selected freshness is
  `BranchStackLoadSource`/`BranchStackSlot`/`BranchTerminatorOrdering`.
- Partial prepared lookup snapshots are accepted only through
  `query_prepared_mir_branch_stack_load_authority()`; do not reintroduce raw
  branch-authority lookup iteration or route-numbered/dump-based authority.

## Proof

No new proof was required for this todo-only Step 4 assessment.

Accepted existing proof:

- Focused proof passed into `test_after.log` before log roll-forward:

```sh
cmake --preset default -DC4C_ENABLE_PREPARED_FACT_TESTS=ON && cmake --build --preset default --target backend_prepare_stack_layout_test backend_riscv_prepared_edge_publication_test && ctest --test-dir build -R '^(backend_prepare_stack_layout|backend_riscv_prepared_edge_publication)$' --output-on-failure | tee test_after.log
```

- Default prepared-fact OFF object-emission regression bucket passed:

```sh
cmake --preset default -DC4C_ENABLE_PREPARED_FACT_TESTS=OFF && cmake --build --preset default && ctest --test-dir build -R '^backend_riscv_object_emission$' --output-on-failure
```

- Broader default backend bucket passed:

```sh
ctest --test-dir build -j --output-on-failure -R '^backend_'
```

Result: 304/304 passed.

- Hook baseline full-suite candidate was accepted at 3333/3333.

Conclusion: Step 4 proof is sufficient for this runbook step; no missing proof
is currently recorded.
