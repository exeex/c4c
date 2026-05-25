Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select One Preservation Reconstruction Authority Leak

# Current Packet

## Just Finished

Step 5 closure review rejected source-idea closure after the local-frame
publication checkpoint. The selected local-frame retained `CallInst::arg_abi`
and `CallInst::arg_types` helper chain is complete, but preservation
reconstruction and printing/helper-boundary consolidation remain within the
source idea's acceptance criteria.

## Suggested Next

Execute Step 1 by selecting one preservation reconstruction authority leak from
the regenerated runbook and recording the selected target plus proof command
here.

## Watchouts

- Do not work on `ideas/open/03_dispatch_responsibility_reduction.md`.
- Do not weaken tests or mark nearby preservation/publication cases
  unsupported to claim progress.
- If a needed prepared preservation fact is missing, stop and record that
  blocker instead of rebuilding the decision locally.

## Proof

Closure review evidence:

- Retained `CallInst::arg_abi` and `CallInst::arg_types` publication reads were
  not found in the surviving `calls*.cpp`/`calls*.hpp` scan.
- `calls_preservation.cpp` still exposes preservation reconstruction helpers
  and BIR/call-list scans that need authority review before the source idea can
  close.
- `calls_printing.cpp` still owns call and call-boundary printing/effect
  spelling listed by the source idea as possible remaining consolidation work.
- Existing Step 4 proof was recorded before this review as:
  `(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`.
- Current workspace check found `test_before.log` but no `test_after.log`;
  this does not block the lifecycle decision because closure was rejected before
  the close regression gate.

Close decision: close rejected; no close-time regression guard was generated
because source-idea completion is false.
