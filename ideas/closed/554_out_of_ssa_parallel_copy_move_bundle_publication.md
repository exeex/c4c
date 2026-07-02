# Out-Of-SSA Parallel-Copy Move-Bundle Publication

Status: Closed
Type: Producer/fact-propagation repair
Parent: `ideas/closed/553_move_bundle_target_shape_evidence_gap_src_960209_1.md`
Owning Layer: Prepared value-location move-bundle publication
Closed: 2026-07-02

## Goal

Repair the producer or fact-propagation path that should publish
out-of-SSA parallel-copy move bundles for prepared traversal consumers.

## Why This Existed

The evidence-gap route for `src/960209-1.c` completed. The row no longer
lacked first-owner evidence: prepared traversal had a `pre_terminator_copies`
event for prepared block label 20 and the parallel-copy edge 20 -> 19, but
value locations did not expose a matching out-of-SSA parallel-copy move bundle
for execution block label 20 / block index 15 and predecessor label 20.

Fresh evidence at activation:

- `prepared_consumer_category=missing_move_bundle`
- `event_kind=pre_terminator_copies`
- `event_block_index=15`
- `prepared_block_label=20`
- `parallel_copy_predecessor=20`
- `parallel_copy_successor=19`
- `parallel_copy_execution_block=20`
- `lookup_execution_block_index=15`
- `candidate_move_bundle_count=21`
- `candidate_phase_block_entry_count=3`
- `candidate_authority_out_of_ssa_parallel_copy_count=3`
- `candidate_execution_block_count=0`
- `candidate_predecessor_label_count=0`
- `candidate_successor_label_count=1`
- `candidate_exact_parallel_copy_match_count=0`
- `value_home_type_f128_facts=unavailable_at_missing_move_bundle`

The useful owner was therefore not RV64 materialization, F128 quarantine, or
diagnostic printing. It was the publication path that decides which execution
block, predecessor, and successor coordinates are attached to out-of-SSA
parallel-copy move bundles.

## Completion Summary

Step 2 commit `b100e69ab` generalized out-of-SSA scalar integer immediate
publication and matching RV64 object materialization. The focused producer and
object tests passed, and the one-row `src/960209-1.c` RV64 gcc torture backend
scan advanced past the audited `prepared_consumer_category=missing_move_bundle`
blocker.

The current first blocker is now:

```text
[RV64_C4C_OBJ_COMPILE_FAIL]
unsupported_local_memory_access: RV64 object route requires prepared frame-slot or pointer-value base-plus-offset local memory addressing
```

This satisfies the route acceptance condition because the original missing
move-bundle publication failure is replaced by a different auditable first
blocker without weakening expectations, unsupported markers, allowlists, or
runtime comparison behavior.

## Validation

Close proof used the existing matching before/after composite logs:

```sh
{ echo '== cmake --build --preset default =='; cmake --build --preset default; echo "== ctest producer/object subset =="; ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_phi_materialize|backend_riscv_object_emission|backend_prepared_object_consumer_contract)$'; echo '== ALLOWLIST=build/agent_state/553_step1_evidence_gap.allowlist scripts/check_progress_rv64_gcc_c_torture_backend.sh =='; ALLOWLIST=build/agent_state/553_step1_evidence_gap.allowlist scripts/check_progress_rv64_gcc_c_torture_backend.sh; } > test_after.log 2>&1
```

The CTest subset remained green and the one-row scan demonstrated row
advancement. The regression guard passed with non-decreasing pass-count policy:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
```

Supervisor acceptance also ran:

```sh
ctest --test-dir build -j --output-on-failure -R '^backend_'
```

The backend bucket passed `345/345`.

## Follow-Up

The new local-memory first blocker is tracked by
`ideas/open/555_rv64_prepared_local_memory_addressing_src_960209_1.md`.
The broader bucket-level review remains
`ideas/open/547_bir_local_memory_call_metadata_boundary_review.md`; it is
related context, not the exact active implementation owner for this row.

## Reviewer Reject Signals

- Reject retroactive claims that this idea completed through diagnostic-only
  changes while the row still reports `prepared_consumer_category=missing_move_bundle`.
- Reject unsupported downgrades, expectation rewrites, allowlist filtering, or
  weaker runtime comparisons as completion evidence.
- Reject folding the new local-memory blocker back into this closed route.
