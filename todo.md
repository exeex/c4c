Status: Active
Source Idea Path: ideas/open/420_rv64_gcc_torture_post_contract_umbrella.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Classify Failure Buckets By First Owning Layer

# Current Packet

## Just Finished

Step 4 refreshed
`docs/rv64_gcc_torture_post_contract/failure_bucket_map.md` against the stable
2026-07-02 reset-main/post-cleanup RV64 gcc_torture backend-object evidence:
`1467` total, `349` pass, `1118` fail, with logs
`build/agent_state/rv64_gcc_torture_backend_current_20260702T032151Z.log` and
`build/agent_state/rv64_gcc_torture_backend_current_20260702T151551Z.log`.

The map now treats stale older summaries as historical only, records the
source-idea fact that `unsupported_move_bundle_target_shape` has 183 current
rows and is the first expected-value ordinary-C follow-up candidate, keeps F128
quarantined and lowest priority, and explicitly marks current row-level
ownership gaps instead of inventing exact first-owner counts for unverified
rows.

## Suggested Next

Execute Step 5 by refreshing
`docs/rv64_gcc_torture_post_contract/followup_idea_plan.md` and generating or
updating the follow-up idea set from the stable 2026-07-02 evidence. The next
packet should prioritize a move-bundle bucket idea that consumes the 183
current `unsupported_move_bundle_target_shape` rows and splits coherent RV64
materialization work from prepared/BIR authority gaps.

## Watchouts

- `followup_idea_plan.md` is still listed as stale in the Step 4 handoff and
  may still cite older bucket ordering or stale current counts.
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
docs/rv64_gcc_torture_post_contract/failure_bucket_map.md todo.md`; proof
output was captured in `test_after.log`.
