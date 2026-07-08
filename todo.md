Status: Active
Source Idea Path: ideas/open/608_prepared_global_data_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh residual diagnostics

# Current Packet

## Just Finished

- Plan owner rejected close for idea `608` because the exhausted runbook still
  left prepared global-memory fact residual evidence represented by
  `src/pr36034-1.c` and `src/pr91137.c`.
- Regenerated `plan.md` around the narrower residual while keeping the same
  source idea open.

## Suggested Next

- Execute Step 1 from `plan.md`: refresh current diagnostics for
  `src/pr36034-1.c` and `src/pr91137.c`, confirm first owner and missing fact,
  and choose the Step 2 implementation packet.

## Watchouts

- Do not close idea `608` until the residual prepared global-memory evidence
  has moved or been explicitly re-owned.
- Do not route `src/pr36034-1.c` or `src/pr91137.c` through selected
  object-data authority; the prior runbook ruled that out.
- Keep RV64 emission, relocation records, unsupported markers, allowlists,
  timeout/accounting files, and expectations out of this runbook.

## Proof

- Lifecycle-only reset; no code validation was run by the plan owner.
- Last executor proof before this reset was:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'
```

- Reported result: passed, 346 backend tests, 0 failures, in `test_before.log`.
