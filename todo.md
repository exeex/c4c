Status: Active
Source Idea Path: ideas/open/700_prepared_mir_stack_view_contract.md
Source Plan Path: plan.md
Current Step ID: 2/3
Current Step Title: PreparedBranchStackLoadAuthority Record And MIR View

# Current Packet

## Just Finished

Step 1 selected `PreparedBranchStackLoadAuthority` as the first prepared
authority family for this runbook. This is the smallest legal first stack
family because it already has status-rich prepared producer rows and lookup
publication, while the remaining MIR work can be bounded to a named branch
stack-load prepared view instead of reusing raw route/debug evidence.

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

Delegate Step 2/3 as one narrow code packet for
`PreparedBranchStackLoadAuthority`: add or tighten a named
`PreparedMirBranchStackLoadAuthorityView` (or equivalently named prepared MIR
branch stack-load query) that reads only the prepared lookup records, then make
the RV64 branch stack-load source freshness helper consume that view instead
of raw lookup iteration.

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

## Proof

Todo-only selection packet; no build or test proof was required and no
`test_after.log` was produced.

Recommended executor proof command for the next code packet:

```sh
cmake --preset default -DC4C_ENABLE_PREPARED_FACT_TESTS=ON && cmake --build --preset default --target backend_prepare_stack_layout_test backend_riscv_prepared_edge_publication_test && ctest --test-dir build -R '^(backend_prepare_stack_layout|backend_riscv_prepared_edge_publication)$' --output-on-failure | tee test_after.log
```
