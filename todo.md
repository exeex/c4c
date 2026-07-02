Status: Active
Source Idea Path: ideas/open/420_rv64_gcc_torture_post_contract_umbrella.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Review And Close Readiness

# Current Packet

## Just Finished

Step 6 reviewed close readiness for the RV64 gcc_torture post-contract
umbrella and repaired the remaining docs-only evidence mismatch in
`docs/rv64_gcc_torture_post_contract/current_scan_summary.md`.

The handoff docs now agree that the current reset-main/post-cleanup RV64
gcc_torture backend-object evidence is the stable 2026-07-02 scan pair:

- `1467` total cases
- `349` pass
- `1118` fail
- `build/agent_state/rv64_gcc_torture_backend_current_20260702T032151Z.log`
- `build/agent_state/rv64_gcc_torture_backend_current_20260702T151551Z.log`

Stale 2026-06-30 and 2026-07-01 totals are documented as historical context
only. RV64 gcc_torture remains external evidence, not default CTest coverage.
No implementation files, test expectations, unsupported markers, allowlists, or
lifecycle source intent files were edited.

Close-readiness status: ready for supervisor/plan-owner lifecycle review. I
found no remaining docs-only evidence blocker inside the owned files.

## Suggested Next

Supervisor should route the completed umbrella to plan-owner for lifecycle
close, deactivate, or replacement decision.

## Watchouts

- Some auxiliary documents under `docs/rv64_gcc_torture_post_contract/` still
  intentionally describe historical packets and may mention older totals such
  as `314/1153` or `404/1063`; do not treat those as current anchors unless a
  later packet refreshes them.
- `regression_delta.md` and `try_gcc_torture_postmortem.md` were not present in
  this checkout during Step 5, despite earlier handoff notifications.
- The current bucket map has verified current diagnostic counts, but it does
  not have a refreshed row-level first-owner table for the 567 current failures
  that lack explicit `unsupported_*` ownership evidence.
- Do not implement RV64 fixes in this umbrella.
- Keep RV64 gcc_torture as external evidence, not a default CTest gate.
- Keep primary-F128 rows screened into the F128 quarantine lane unless fresh
  evidence proves broad non-F128 impact.
- Do not weaken unsupported markers, allowlists, expected output, runtime
  comparison, pass/fail accounting, or default CTest contracts.
- Keep `ideas/open/426_f128_quarantine_and_external_softfloat_policy.md` as
  the existing low-priority F128 policy lane.
- Leave `review/global_address_helper_cleanup_review.md` untouched.
- Treat `test_baseline.new.log` as a rejected full-suite candidate, not an
  accepted baseline.

## Proof

Ran `git diff --check --
docs/rv64_gcc_torture_post_contract/current_scan_summary.md
docs/rv64_gcc_torture_post_contract/README.md todo.md`; proof output was
captured in `test_after.log`.
