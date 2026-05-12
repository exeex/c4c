Status: Active
Source Idea Path: ideas/open/173_aggregate_layout_identity_structured_boundary.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Validate And Summarize The Boundary

# Current Packet

## Just Finished

Completed `plan.md` Step 5 by validating and summarizing the selected local
aggregate GEP boundary.

Boundary accepted:
- The selected consumer is local aggregate GEP lookup/lowering.
- Metadata-rich generated local aggregate GEP inputs now use structured layout
  identity and fail closed when the structured context is stale, mismatched,
  opaque, or otherwise unusable for that consumer.
- Equal rendered aggregate spelling is no longer sufficient to accept the
  selected metadata-rich local GEP boundary.
- Legacy/no-metadata compatibility remains explicit: hand-authored inputs that
  lack structured metadata may still use the existing `type_decls`/rendered
  layout fallback, but that path does not hide structured mismatch or keyed
  structured-entry collision cases at this selected consumer.
- Focused behavior coverage remains in `backend_lir_to_bir_notes_test` for raw
  byte-slice layout, scalar leaf byte-slice layout, tail memcpy projection,
  structured-vs-legacy mismatch rejection, and opaque structured-entry legacy
  fallback rejection.
- `backend_prepare_structured_context_test` covers the structured context
  mismatch bit that the local GEP consumer observes.

No validation gap was found, so no tests or implementation files were edited.

## Suggested Next

Suggested next packet is lifecycle closure review by `c4c-plan-owner`.

## Watchouts

- Remaining work for globals, byval copies, call ABI, stack/global layout
  routes, or broad backend aggregate migration is outside this selected local
  aggregate GEP boundary.
- The residual compatibility fallback is intentional only for legacy or
  hand-authored no-metadata inputs. Future metadata-rich consumers should not
  copy that fallback without the same fail-closed structured mismatch guard.
- This runbook appears complete for the selected boundary; plan-owner should
  decide whether the source idea is complete, should close, or should continue
  through a separate follow-up initiative.

## Proof

No proof command was rerun for Step 5 because this packet made no code or test
edits and found no unrecorded validation gap.

Fresh focused proof already completed in `test_after.log`:

```sh
{ cmake --build build --target backend_prepare_structured_context_test backend_lir_to_bir_notes_test && ctest --test-dir build -R 'backend_prepare_structured_context|backend_lir_to_bir_notes' --output-on-failure; } > test_after.log 2>&1
```

Result: passed; `2/2` focused tests passed.

Regression guard:
- `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
- Result: passed with non-decreasing passed count allowed; focused CTest count
  stayed `2/2` because the new coverage is inside existing backend test
  binaries.

Additional backend validation:
- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
- Result: passed; `109` backend tests run, `0` failed, with the existing
  disabled MIR trace tests not run.

Accepted full-suite baseline candidate:
- `log/baseline_9aed9b5d0fbbb348ee25819ee2a715c0a5aa5793.log`
- Commit: `9aed9b5d0fbbb348ee25819ee2a715c0a5aa5793` (`Fail closed on local
  GEP layout mismatches`)
- Result: passed; `3137` tests run, `0` failed, with the existing disabled MIR
  trace tests not run.
