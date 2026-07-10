Status: Active
Source Idea Path: ideas/open/675_post_wave_residual_baseline_failures.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Reconcile The Post-676/677 Residual Set

# Current Packet

## Just Finished

Lifecycle transition: closed 677 after the object-route materialization repair
and reactivated 675 for post-follow-up residual reconciliation.

## Suggested Next

Execute Step 1 from `plan.md`: reconcile the post-676/677 residual set by
stable test name, starting from the remaining backend row
`backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication`.

## Watchouts

- Ideas 676 and 677 are closed; do not reopen them without fresh contradictory
  evidence.
- `test_baseline.new.log` is still not accepted until all remaining
  candidate-only/common residual policy is settled.
- Compare rows by stable test name, not numeric row id.

## Proof

Close-gate backend baseline before lifecycle edits was captured with
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
in `test_before.log`; it reflects the known remaining backend failure.
