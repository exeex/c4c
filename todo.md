Status: Active
Source Idea Path: ideas/open/700_prepared_mir_stack_view_contract.md
Source Plan Path: plan.md
Current Step ID: 2/3
Current Step Title: PreparedBranchStackLoadAuthority Record And MIR View

# Current Packet

## Just Finished

Step 2/3 implemented the selected `PreparedBranchStackLoadAuthority` slice.
`PreparedMirBranchStackLoadAuthorityView` and the named
`query_prepared_mir_branch_stack_load_authority()` query now expose branch
stack-load authority through the prepared MIR boundary, and
`PreparedMirFunctionView::branch_stack_load_authority()` wraps that query for
normal MIR consumers.

RV64 fused pointer branch stack-load freshness now goes through the named
prepared MIR authority boundary on the real object-emission path:
`PreparedMirFunctionView::branch_stack_load_authority()` when the full MIR view
exists, and `query_prepared_mir_branch_stack_load_authority()` for partial
prepared snapshots that have branch stack-load authority without the full core
view. The query returns `available=true` only for
`PreparedBranchStackLoadAuthorityStatus::Available` records with selected
`BranchStackLoadSource` freshness backed by `BranchStackSlot` and
`BranchTerminatorOrdering`. Missing, unsupported, ambiguous, invalid, stale, or
clobbered authority remains fail-closed.

The focused stack-layout test now covers the prepared MIR query directly with
one available lhs branch stack-load authority and one same-slot clobber
rejection.

Owning files for the selected family:

- Producer record and collection surface:
  `src/backend/prealloc/publication_plans.hpp`,
  `src/backend/prealloc/publication_plans.cpp`, and
  `src/backend/prealloc/prepared_lookups.cpp`.
- Prepared lookup/view exposure:
  `src/backend/prealloc/prepared_lookups.hpp`,
  `src/backend/prealloc/prepared_lookups.cpp`,
  `src/backend/mir/prepared_view.hpp`, and
  `src/backend/mir/prepared_view.cpp`.
- Current MIR consumer seam to move behind the named prepared view:
  `src/backend/mir/riscv/codegen/object_emission.cpp` branch stack-load
  source freshness helpers.

Route-numbered evidence is compatibility-only: Route 4, Route 5, Route 7,
`RouteIndexReferenceFacade`, route dumps, expectation rows, and allowlists may
explain old agreement or diagnostics, but they must not authorize MIR branch
stack-load operands. The selected family must be accepted only through
`PreparedBranchStackLoadAuthorityStatus::Available` rows with selected
`PreparedValueFreshnessUseKind::BranchStackLoadSource` freshness at the branch
terminator point.

Named positive proof surface: `backend_prepare_stack_layout` already exercises
`check_branch_stack_load_authority_contract()` with available condition, lhs,
and rhs branch stack-load rows plus prepared-printer exposure for
`branch_stack_load_authority`.

Named negative fail-closed proof surface: the same
`check_branch_stack_load_authority_contract()` covers missing, ambiguous,
stale, wrong-value, wrong-use, stack-home-only, unsupported-home, same-slot
clobber, unpreserved call, mismatched preserved slot, and stale preserved
source rejection statuses; the MIR/object consumer side should pair this with
`backend_riscv_prepared_edge_publication` when the next code packet moves RV64
branch stack-load consumption behind the prepared view.

## Suggested Next

Delegate Step 4 for `PreparedBranchStackLoadAuthority`: decide whether the
current focused producer/MIR/RV64 proof is sufficient for the runbook proof
step, or add one narrow object/runtime-facing negative proof if supervisor
wants an executable RV64 rejection above the current contract-level coverage.

## Watchouts

- Do not use Route 4, Route 5, Route 7, route facade status, dump rows,
  expectations, or allowlists as MIR authority.
- Do not reactivate ideas 647 or 655 until positive prepared producer evidence
  exists above route dumps.
- Keep route facade contraction and dump vocabulary cleanup out of this packet.
- Keep the selected family exact: branch stack-load authority only. Do not fold
  aggregate stack-source, direct-edge publication source, generic value-home,
  or move-bundle authority into the next packet.
- The next packet should fail closed on missing, ambiguous, invalid,
  unsupported, stale, or route-only branch stack-load authority. In practice
  that means no `Available` MIR view result unless the prepared record is
  `Available` and its selected freshness is
  `BranchStackLoadSource`/`BranchStackSlot`/`BranchTerminatorOrdering`.
- Partial prepared lookup snapshots are accepted only through
  `query_prepared_mir_branch_stack_load_authority()`; do not reintroduce raw
  branch-authority lookup iteration or route-numbered/dump-based authority.

## Proof

Fresh focused proof passed and is recorded in `test_after.log`. The default
prepared-fact OFF object-emission regression bucket also passed.

Command run:

```sh
cmake --preset default -DC4C_ENABLE_PREPARED_FACT_TESTS=ON && cmake --build --preset default --target backend_prepare_stack_layout_test backend_riscv_prepared_edge_publication_test && ctest --test-dir build -R '^(backend_prepare_stack_layout|backend_riscv_prepared_edge_publication)$' --output-on-failure | tee test_after.log
```

Result: `backend_prepare_stack_layout` and
`backend_riscv_prepared_edge_publication` both passed.

Additional regression command run:

```sh
cmake --preset default -DC4C_ENABLE_PREPARED_FACT_TESTS=OFF && cmake --build --preset default && ctest --test-dir build -R '^backend_riscv_object_emission$' --output-on-failure
```

Result: `backend_riscv_object_emission` passed.
