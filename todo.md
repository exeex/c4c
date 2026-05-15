Status: Active
Source Idea Path: ideas/open/238_aarch64_atomic_machine_nodes.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Allocate Atomic Loop Printer Facts In Selection

# Current Packet

## Just Finished

Step 6 lifecycle review rejected closure for now and repaired the active
runbook. Step 5 proved selected atomic records can print structured atomic
machine nodes, but the source idea still requires the prepared-to-selected route
to generate the loop scratch/status/loaded-value register facts needed by
printable RMW and compare-exchange programs.

## Suggested Next

Execute Step 6 `Allocate Atomic Loop Printer Facts In Selection`.

Required proof command:
`set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } 2>&1 | tee test_after.log`

## Watchouts

- Do not infer atomic semantics from volatile flags, rendered text, fixed
  scratch-register snippets, or named testcase shortcuts.
- Step 5 prints from selected atomic records, but the prepared-to-selected route
  still does not allocate loop scratch/status/loaded-value registers for all
  end-to-end atomic programs; Step 6 owns that gap.
- Preserve ordinary volatile memory behavior separately from atomic behavior;
  atomic selection must continue to require carrier provenance.
- RMW and compare-exchange printer records now require explicit scratch/status
  register facts; do not replace that with fixed scratch-register assumptions.
- Do not fold intrinsic, inline-assembly, binary128, scalar FP, or i128 behavior
  into this route.

## Proof

Latest implementation proof command:
`set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } 2>&1 | tee test_after.log`

Result: passed, `139/139` backend tests green for Step 5.

Proof log: `test_after.log`.

Lifecycle review:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result: passed, `139/139` before and after.
