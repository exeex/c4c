Status: Active
Source Idea Path: ideas/open/86_full_x86_backend_contract_first_replan.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove Live Interface Conformance
Plan Review Counter: 0 / 6
# Current Packet

## Just Finished

Completed plan Step 3 by running the delegated build plus `^backend_` proof for
the x86 contract-first packet. The live backend/x86 public seams still
conform after the Step 2 ownership and markdown cleanup: the build stayed
clean and the enabled backend subset passed without introducing hidden
workaround paths.

## Suggested Next

Run Step 4 to classify any residual findings from this contract-first route,
separate behavior recovery from remaining contract/layout scope, and decide
whether idea 86 is ready for closure or needs a follow-on idea.

## Watchouts

- Keep this runbook focused on contract/layout ownership, not behavior recovery.
- Do not treat the passing Step 3 proof as license to absorb adjacent behavior
  work back into idea 86.
- The `backend_` subset still reports 12 disabled MIR-focused tests; they did
  not block Step 3 conformance, but they are not evidence for new behavior
  recovery scope inside this runbook.
- If Step 4 finds real leftovers, split behavior-only work into a separate
  idea instead of stretching the contract/layout closure criteria.

## Proof

Ran the delegated proof command:
`bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_" | tee test_after.log'`.
Result: success. Build completed with no work to rebuild, and ctest reported
`100% tests passed, 0 tests failed out of 97` for the enabled `backend_`
subset, with 12 tests listed as disabled/not run. Proof log:
`/workspaces/c4c/test_after.log`.
