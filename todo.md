Status: Active
Source Idea Path: ideas/open/553_move_bundle_target_shape_evidence_gap_src_960209_1.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Or Regenerate Auditable Evidence

# Current Packet

## Just Finished

Completed Step 3, "Add Or Regenerate Auditable Evidence", by extending the
general prepared object traversal diagnostic for
`PreparedObjectMoveBundleConsumerStatus::MissingMoveBundle`. The diagnostic now
records event kind, event block/instruction indexes, prepared block label,
parallel-copy predecessor/successor/execution labels, move/step/cycle counts,
lookup availability, resolved execution block index, and aggregate
move-bundle candidate counts for phase, authority, execution block,
predecessor label, successor label, and exact parallel-copy match.

The fresh `src/960209-1.c` row still fails as expected with
`prepared_consumer_category=missing_move_bundle`, but the failure is now
auditable: `event_kind=pre_terminator_copies`, `event_block_index=15`,
`prepared_block_label=20`, `parallel_copy_predecessor=20`,
`parallel_copy_successor=19`, `parallel_copy_execution_block=20`,
`lookup_execution_block_index=15`, `candidate_move_bundle_count=21`,
`candidate_phase_block_entry_count=3`,
`candidate_authority_out_of_ssa_parallel_copy_count=3`,
`candidate_execution_block_count=0`, `candidate_predecessor_label_count=0`,
`candidate_successor_label_count=1`, and
`candidate_exact_parallel_copy_match_count=0`.

Value/home/type/F128 facts are explicitly reported as unavailable at this
missing-authority point; no classification ownership was invented and no
lowering semantics or test expectations were changed.

## Suggested Next

Use the new evidence to route the next packet to the producer that should emit
or propagate the missing out-of-SSA parallel-copy move bundle for execution
block label 20/block index 15, predecessor 20, successor 19. The next packet
should inspect why existing move bundles contain block-entry/out-of-SSA facts
and one successor-label match, but no bundle matches the execution block or
predecessor label.

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
- `clang-format` and versioned `clang-format-*` binaries were not available on
  PATH in this container; `git diff --check` passed.
- The exact blocker is now narrower than a generic missing authority:
  `find_parallel_copy_move_bundle(...)` sees value locations and resolves the
  execution block label, but finds zero exact matches because no candidate has
  the required execution block and no candidate has the required predecessor
  label.

## Proof

Ran the delegated Step 3 proof command exactly:

```sh
{ echo '== cmake --build --preset default =='; cmake --build --preset default; echo '== ALLOWLIST=build/agent_state/553_step1_evidence_gap.allowlist scripts/check_progress_rv64_gcc_c_torture_backend.sh =='; ALLOWLIST=build/agent_state/553_step1_evidence_gap.allowlist scripts/check_progress_rv64_gcc_c_torture_backend.sh; } > test_after.log 2>&1
```

The build completed successfully. The focused allowlist scan exited `1`
because `src/960209-1.c` still fails as expected, now with the expanded
missing move-bundle evidence. Proof log preserved at `test_after.log`; case log
is `build/rv64_gcc_c_torture_backend/src_960209-1.c/case.log`.

Supervisor acceptance also ran:

```sh
ctest --test-dir build -j --output-on-failure -R '^(backend_prepared_object_consumer_contract|backend_riscv_object_emission)$'
```

Both targeted backend tests passed after updating the prepared object consumer
contract test to require the diagnostic prefix and structured evidence fields.
