Status: Active
Source Idea Path: ideas/open/593_rv64_branch_stack_source_freshness_consumption.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add Focused RV64 Proof

# Current Packet

## Just Finished

Completed Step 4 for `plan.md`: added focused RV64 object-emission proof for
the migrated fused pointer `Lhs` stack-load path. The accepted fixture now
builds only after the selected shared `BranchStackLoadSource` /
`BranchStackSlot` authority is available for the exact branch use, then emits
an RV64 load from the selected stack slot into the `Lhs` branch scratch
register before branching.

The negative proof exercises the Step 3 visible status path with mutated RV64
lookup authority rows for missing, ambiguous, invalid, stale, wrong-value,
wrong-use, future-point, and stack-home-only freshness. Those cases fail closed
through `unsupported_branch_stack_load_source_freshness` with
`authority_status`, `source_freshness_status`, and
`source_freshness_candidates`. A missing-layout case remains distinct as
`unsupported_branch_stack_load_authority`, preserving the source-freshness vs.
layout/status boundary.

## Suggested Next

Execute Step 5 closure inventory for 593: inventory the migrated RV64 fused
pointer `Lhs` consumer, list remaining RV64 branch stack-source shapes and
whether they need a 594 handoff, and answer the source-idea completion
questions at the correct lifecycle layer.

## Watchouts

- Do not start 594 before 593 closes with a concrete closure-note handoff.
- Do not use RV64 target-local stack-home, frame-slot, aggregate-lane, clobber,
  register, or operand-shape evidence as freshness.
- If a producer fact promised by 592 is missing, record that as a blocker for
  the 592 family instead of manufacturing fallback freshness in RV64.
- Producer-side blocker: pointer `Rhs` is not available for the first RV64
  migration because the current producer/collector keeps it inventory-only
  (`policy=none`, `status=missing_policy`). Do not add an RV64 fallback for
  `Rhs`.
- Step 4 added a narrow RV64-local bypass for the older GPR-only fused pointer
  publication rejection only when stack-slot `Lhs` selected freshness has
  already passed and `Rhs` remains register-compatible or null. Do not broaden
  that to pointer `Rhs`, scalar condition register branches, string assembly
  emission, aggregate-adjacent consumers, AArch64, or x86 during closure.

## Proof

Ran the supervisor-selected proof command:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`.

Result: passed, `100% tests passed, 0 tests failed out of 346`. Canonical proof
log: `test_after.log`.
