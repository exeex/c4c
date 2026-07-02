Status: Active
Source Idea Path: ideas/open/553_move_bundle_target_shape_evidence_gap_src_960209_1.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reproduce The Evidence Gap

# Current Packet

## Just Finished

Completed Step 1, "Reproduce The Evidence Gap", by preserving the fresh
one-row `src/960209-1.c` backend baseline in canonical execution state only.
The focused scan used
`build/agent_state/553_step1_evidence_gap.allowlist`, generated case log
`build/rv64_gcc_c_torture_backend/src_960209-1.c/case.log`, and currently
reports `[RV64_C4C_OBJ_COMPILE_FAIL]` with:

```text
prepared module shape: prepared_consumer_category=missing_move_bundle:
prepared copy traversal event is missing move-bundle authority
```

This does not match the stale classification/representatives expectation that
the row belongs under `unsupported_move_bundle_target_shape`.

## Suggested Next

Execute Step 2 by locating the diagnostic authority for
`missing_move_bundle` / "prepared copy traversal event is missing move-bundle
authority" and identifying which missing fact prevents this row from retaining
auditable move-bundle authority.

## Watchouts

- This plan is evidence-first. Do not route the row to RV64, prepared, BIR, or
  F128 from filename, source shape, raw BIR shape, or bucket membership alone.
- Do not change expectations, unsupported markers, allowlists, or runtime
  comparison behavior.
- Keep any diagnostic work focused on emitting auditable facts, not on making a
  narrow testcase pass.
- The current one-row diagnostic supersedes stale docs/tables that classify
  `src/960209-1.c` as `unsupported_move_bundle_target_shape`; the fresh row
  says `prepared_consumer_category=missing_move_bundle`.

## Proof

Ran the delegated proof command exactly:

```sh
{ echo '== cmake --build --preset default =='; cmake --build --preset default; echo '== ALLOWLIST=build/agent_state/553_step1_evidence_gap.allowlist scripts/check_progress_rv64_gcc_c_torture_backend.sh =='; ALLOWLIST=build/agent_state/553_step1_evidence_gap.allowlist scripts/check_progress_rv64_gcc_c_torture_backend.sh; } > test_after.log 2>&1
```

The build was up to date. The focused scan exited `1` because the single
allowlisted case fails as expected for this evidence-gap baseline:
`fail src/960209-1.c
/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_960209-1.c/case.log`.
Proof log preserved at `test_after.log`.
