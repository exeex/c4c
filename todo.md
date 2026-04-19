# Execution State

Status: Active
Source Idea Path: ideas/open/62_prealloc_cfg_generalization_and_authoritative_control_flow.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Validate The CFG Ownership Route
Plan Review Counter: 1 / 10
# Current Packet

## Just Finished

Completed the `plan.md` Step 4 validation checkpoint after the Step 3.3.3.3
audit closed the residual joined-branch helper proof surface. The broad
`^backend_` check stayed stable at 68 passed / 4 failed / 72 total with no
new failing tests, but the strict close-time regression guard still rejected
closure because the pass count did not increase.

## Suggested Next

Treat idea 62 as technically exhausted on the current runbook, but keep the
plan active because close is rejected. Do not reopen Step 3.3 without naming a
real uncovered consumer family first; the remaining blocker is the close gate,
not an identified CFG-contract gap.

## Watchouts

- Do not reintroduce raw string-keyed control-flow contracts now that idea 64
  closed.
- Keep phi-completion work in idea 63 unless it is strictly required to make
  CFG ownership truthful.
- Reject testcase-shaped branch or join matcher growth.
- Do not treat the flat 68/72 `^backend_` result as a regression in the code
  path itself; the blocker is the strict monotonic close gate.
- Keep `PreparedBranchCondition` and `PreparedControlFlowBlock` targets
  contract-consistent; mismatches should still fail the canonical
  prepared-module handoff instead of silently preferring whichever record
  happens to look usable locally.
- Do not invent a new Step 3 packet unless a specific remaining consumer or
  helper surface is identified first.
- The trailing-join arithmetic, `xor`, `and`, `or`, `mul`, `shl`, `lshr`, and
  `ashr` families already have direct and `EdgeStoreSlot` joined-route proof.
- The generic compare-join render-contract helper case and the explicit
  trailing `xor`, `and`, `or`, `mul`, `shl`, `lshr`, and `ashr` helper lanes
  are all present in
  `tests/backend/backend_x86_handoff_boundary_joined_branch_test.cpp`.
- Close remains rejected unless the chosen close scope can satisfy the
  regression guard with a passing result.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_' | tee test_after.log`, then checked
`test_before.log` vs `test_after.log` with
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py
--before test_before.log --after test_after.log`. Raw counts were unchanged at
68 passed / 4 failed / 72 total with no new failing tests, but the checker
returned nonzero because passed count did not strictly increase. Close
decision: rejected.
