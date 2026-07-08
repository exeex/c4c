Status: Active
Source Idea Path: ideas/open/594_rv64_branch_stack_source_consumption_followup_from_593.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove Accepted And Rejected Rhs Cases

# Current Packet

## Just Finished

Completed `plan.md` Step 3 proof coverage for the selected RV64 pointer `Rhs`
branch stack-source consumer route in
`tests/backend/mir/backend_riscv_object_emission_test.cpp`.

Added a stack-homed pointer `%rhs` fused-branch fixture that keeps `%lhs` in a
GPR and publishes a real RV64 frame slot for `%rhs`. The accepted object test
now proves the prepared RV64 emitter builds only when selected shared
`BranchStackLoadSource` / `BranchStackSlot` freshness is present, loads `%rhs`
from its selected stack slot into the branch scratch register, and emits the
branch using that loaded RHS operand.

Added RHS-specific authority diagnostics mirroring the existing LHS coverage.
The rejected cases cover missing source freshness, ambiguous source freshness,
invalid source freshness, stale branch use, future branch use, wrong value
authority, wrong use authority, stack-home-only authority, and a distinct
missing-frame-slot/layout authority failure. The source-freshness cases require
`unsupported_branch_stack_load_source_freshness` with `role=rhs`; the layout
case remains `unsupported_branch_stack_load_authority`, so freshness failure
does not collapse into stack-home or layout evidence.

No production implementation edits were needed for Step 3, and no expectations,
unsupported markers, allowlists, or runtime-output contracts were weakened.

## Suggested Next

Execute `plan.md` Step 4 by validating the freshness queue coverage requested
by the supervisor across the 587 through 596 chain and deciding whether this
Step 3 object-emission proof is sufficient or should be paired with additional
focused freshness/producer subsets.

## Watchouts

- Step 3 only added object-emission tests; it did not broaden into producer,
  Prepared MIR view, or runtime expectation work.
- The RHS rejected cases mutate prepared lookup authority records after shared
  collection, so they prove the RV64 consumer requires selected shared
  freshness for the exact branch use rather than inferring from stack-home or
  operand shape.
- If Step 4 expands validation, keep proof commands supervisor-selected and
  preserve `test_after.log` as the canonical executor log.

## Proof

Ran the supervisor-delegated proof command:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`.

Result: passed. `test_after.log` is the canonical proof log for this packet.
