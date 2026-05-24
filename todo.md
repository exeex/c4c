Status: Active
Source Idea Path: ideas/open/aarch64-codegen-01-calls-dispatch-bridge-helper-absorption.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Focused Behavior-Preservation Proof

# Current Packet

## Just Finished

Step 4 recorded focused behavior-preservation proof for the completed Step 2
AArch64 call bridge helper absorption.

Readback checks:

- The only implementation slice in this active plan is commit `1bf9f1c86`
  (`Move AArch64 call move sequencing helpers`), which moved the before-call
  move sequencing helper family across `calls_dispatch_bridge.*`,
  `calls.hpp`, and `calls_moves.cpp` plus the packet `todo.md` update.
- Step 3 was todo-only (`37fa0b122`) and found no remaining helper family
  same-shape enough for another bounded absorption packet under this plan.
- No test files or expectation files changed in the active plan diff, so there
  is no expectation downgrade, unsupported-test conversion, or testcase-shaped
  proof shortcut in this slice.

## Suggested Next

Supervisor should call plan-owner for lifecycle closure review. The runbook's
ordered steps are exhausted, the only code-changing packet has fresh backend
proof, and the remaining bridge helpers have documented ownership reasons.

## Watchouts

- Remaining helper caveat: `materialize_call_boundary_source_to_destination`
  is adjacent to before-call moves, but it materializes a missing frame-slot
  source value into a call ABI destination and records scalar-state
  publication, so absorbing it would require a wider proof surface.
- Other remaining bridge helpers own scalar argument materialization, public
  dispatch bridge entry, indirect callee lowering, call-result source
  recording, missing frame-slot argument materialization, or stack-preserved
  value publication. Treat any future absorption of those helpers as a separate
  plan or reviewed route change.
- Keep closure review focused on behavior preservation; there was no
  expectation churn or unsupported-path downgrade in this plan.

## Proof

Delegated proof command:

`bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"; } 2>&1 | tee test_after.log'`

Result: passed. Build reported `ninja: no work to do`; `ctest -R
"^backend_"` passed 162/162 tests with 0 failures. Proof log:
`test_after.log`.

Additional check: `git diff --check` passed.
