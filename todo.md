Status: Active
Source Idea Path: ideas/open/636_prepared_branch_stack_source_freshness_publication.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Add Focused Producer/Publication Coverage

# Current Packet

## Just Finished

Step 2/3 focused producer publication coverage and repair completed. Added
collector coverage for a legal fused pointer branch stack-slot source whose BIR
block identity is carried through the structured label id rather than the raw
label string, then repaired
`collect_prepared_branch_stack_load_authorities(...)` to bind branch blocks
through prepared/BIR label agreement instead of comparing label ids across name
tables.

Fresh prepared dumps under
`build/agent_state/636_step2_branch_source_freshness/` show the target prepared
producer rows now publish selected `BranchStackLoadSource` / `BranchStackSlot`
freshness for the legal stack-slot operands:

- `src/930930-1.c`: `function=f`, `block=block_1`, `role=lhs`,
  `value=%t1`, `status=available`, `source_freshness_status=selected`,
  `source_freshness_candidates=1`.
- `src/990127-1.c`: `function=main`, `block=block_1`, `role=lhs`,
  `value=%t6`, `status=available`, `source_freshness_status=selected`,
  `source_freshness_candidates=1`.
- `src/20060910-1.c`: `function=check_header`, `block=block_2`,
  `role=rhs`, `value=%t9`, `status=available`,
  `source_freshness_status=selected`, `source_freshness_candidates=1`.

The supplemental row probe still exits nonzero, but the first blockers moved:
`930930-1.c` and `20060910-1.c` now stop at
`unsupported_terminator_fragment`, while `990127-1.c` stops at
`unsupported_branch_stack_load_authority` for `role=rhs`, `value=%lv.a`,
`authority_status=home_value_mismatch`. No RV64 target-local freshness
inference, expectation rewrite, unsupported marker, or allowlist change was
added.

## Suggested Next

Delegate Step 4/5 validation/disposition: confirm the RV64 consumer now only
sees prepared-published branch stack-source freshness for the moved rows, then
decide whether the remaining `unsupported_terminator_fragment` and
`home_value_mismatch` blockers belong to this idea's closeout or separate
follow-up ideas.

## Watchouts

- Do not infer freshness in RV64 consumers.
- Do not merge idea-635 clobber-safety work into this plan.
- Do not treat expectation, unsupported-marker, allowlist, timeout, runtime,
  accounting, helper rename, or diagnostic-only edits as capability progress.
- The repaired block lookup intentionally avoids raw `block.label_id ==
  prepared_label` comparisons because BIR and prepared labels live in distinct
  name tables; cross-table numeric id collisions were the source of the stale
  `unsupported_terminator value=<none>` producer rows.
- `990127-1.c` now has selected freshness for the original Step 1 `lhs %t6`
  row, but RV64 reaches a separate RHS `%lv.a` home/value mismatch in the same
  block.

## Proof

Required canonical proof passed and preserved `test_after.log`:

`cmake --build --preset default --target backend_prepare_stack_layout_test -j1 && ctest --test-dir build -j --output-on-failure -R '^backend_prepare_stack_layout$' > test_after.log 2>&1`

Supplemental row probe ran after code changes:

`cmake --build --preset default && ALLOWLIST=build/agent_state/636_step1_branch_source_freshness.allowlist BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/636_step2_branch_source_freshness.log 2>&1`

Result: command exited `1` with `total=3 passed=0 failed=3`; failures are now
the moved blockers recorded above, with per-case logs under
`build/rv64_gcc_c_torture_backend/`.
