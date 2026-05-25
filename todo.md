Status: Active
Source Idea Path: ideas/open/01_shared_call_plan_authority.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Behavior and Guard Against Overfit

# Current Packet

## Just Finished

Step 4 proof/guard work recorded supervisor-run broader backend validation for
the migrated callee-saved preservation boundary-effect family. The broader
backend run passed with 162/162 backend tests, preserving the migrated shared
prepared endpoint-fact consumption without downgrading expectations or adding
named-case shortcuts.

## Suggested Next

Proceed to Step 5 handoff documentation and closure decision for the shared
call-plan authority runbook.

## Watchouts

- Do not start AArch64 calls file consolidation before shared authority is
  proven.
- Do not hard-code AArch64-only facts into the shared planner.
- Do not weaken tests or add named-case matching as proof of progress.
- AArch64 still owns live-use gates for whether a prepared preservation effect
  should emit at block entry or before/after a call. This packet moved the
  preservation storage decision source, not those liveness decisions.
- `find_prior_preserved_value_for_value()` still checks previous preservation
  state to avoid redundant population; it is not the current-call preservation
  decision source.
- Shared planner reasons are not automatically printable AArch64 move-selector
  reasons. Keep the selected AArch64 reason on emitted preservation population
  records unless the selector contract is deliberately widened.

## Proof

Supervisor-run broader validation command:
`bash -lc '{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"; } > test_after.log 2>&1'`

Proof passed: build completed and all 162/162 backend tests passed.

Executor proof check:
`test -f test_after.log && tail -n 40 test_after.log >/dev/null`

Proof log: `test_after.log`.
