Status: Active
Source Idea Path: ideas/open/420_rv64_gcc_torture_post_contract_umbrella.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Generate Ordered Follow-Up Ideas
你該做code review了

# Current Packet

## Just Finished

Step 2 repair refreshed the remaining docs-only evidence mismatch in
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

This repair is a prerequisite for Step 6 close-readiness review. I found no
remaining docs-only evidence blocker inside the owned files.

## Suggested Next

Execute Step 6 by reviewing close readiness for the umbrella. Confirm the
handoff docs and generated ideas satisfy the source idea, verify no
implementation patches or expectation downgrades entered the umbrella, and
record the close-gate expectation for supervisor/plan-owner review.

## Watchouts

- Some auxiliary documents under `docs/rv64_gcc_torture_post_contract/` still
  intentionally describe historical packets and may mention older totals such
  as `314/1153` or `404/1063`; do not treat those as current anchors unless a
  later packet refreshes them.
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
todo.md`; proof output was captured in `test_after.log`.
