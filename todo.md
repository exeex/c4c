Status: Active
Source Idea Path: ideas/open/594_rv64_branch_stack_source_consumption_followup_from_593.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Migrate The Rhs Consumer

# Current Packet

## Just Finished

Completed `plan.md` Step 2 implementation for the selected RV64 pointer `Rhs`
consumer route in `src/backend/mir/riscv/codegen/object_emission.cpp`.

The RV64 fused pointer branch stack-load freshness query is now role-parametric
for `PreparedBranchStackLoadRole::Lhs` and `PreparedBranchStackLoadRole::Rhs`.
The new `Rhs` wrapper requires the same prepared value id/name as `rhs_home`,
the exact branch block index, exact terminator instruction index,
`BranchStackLoadSource`, `BranchStackSlot`, `BranchTerminatorOrdering`,
`BranchStackSlot` rank, the same home pointer, and the same branch instruction
point before reporting selected freshness available.

`fragment_for_prepared_fused_pointer_branch()` now runs the `Rhs` selected
shared freshness gate immediately after the existing `Lhs` gate and before
planning or emitting the RV64 fused pointer branch. A stack-homed pointer
`Rhs` can therefore no longer reach `append_rv64_move_value_to_register()`
from stack-source evidence unless selected shared freshness matches the exact
branch use.

The publication allowance is symmetric for `Rhs` stack-slot operands after the
freshness gate passes. Layout, stack-home, register, and publication status are
still only support facts: missing selected shared freshness returns before the
layout/publication allowance, and unsupported layout or operand-home failures
remain separate.

The unsupported terminator diagnostic path now checks `Rhs` after `Lhs` and
reports `unsupported_branch_stack_load_source_freshness` / `role=rhs` for
missing, invalid, ambiguous, or unsupported selected source freshness while
preserving the distinct `unsupported_branch_stack_load_authority` path for
layout/authority failures such as missing frame slots.

## Suggested Next

Execute `plan.md` Step 3 by adding focused RV64 object-emission proof for the
accepted stack-homed pointer `Rhs` route and rejected missing/ambiguous/stale/
wrong-value/wrong-use/future-point/stack-home-only authority cases, mirroring
the existing Lhs-focused object-emission coverage without expectation or
unsupported-marker rewrites.

## Watchouts

- Do not add an RV64 fallback if selected shared producer authority is missing.
- Do not infer freshness from stack homes, frame slots, aggregate lanes,
  clobber facts, register facts, operand shape, or testcase shape.
- Keep pointer `Rhs` consumer migration separate from 591 Prepared MIR view
  research and 595 umbrella triage.
- Keep layout, stack home, and clobber checks as support facts only. Adjacent
  fail-closed cases to prove in Step 3: missing selected freshness,
  ambiguous selected freshness, invalid/stale/wrong-value/wrong-use/future-point
  freshness, stack-home-only freshness, missing frame slot/layout, stack object
  mismatch, home/value mismatch, missing clobber safety, unsupported operand
  shape, and unknown pointer status.
- Step 2 added the consumer gate and diagnostics, but did not add or rewrite
  tests. Step 3 should pin accepted/rejected `Rhs` behavior directly.
- Existing focused RV64 test surface for the migrated Lhs route is
  `tests/backend/mir/backend_riscv_object_emission_test.cpp` lines 739-774 and
  12631-13020; Step 3 should add the analogous `Rhs` object/proof coverage
  without weakening expectations.

## Proof

Ran the supervisor-delegated proof command:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`.

Result: passed. `test_after.log` is the canonical proof log for this packet.
