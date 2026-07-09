Status: Active
Source Idea Path: ideas/open/636_prepared_branch_stack_source_freshness_publication.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Validate RV64 Consumption Boundary

# Current Packet

## Just Finished

Step 4/5 validation/disposition completed for the RV64 consumption boundary.
The delegated row probe rebuilt the default preset and reran the three
representative rows with
`ALLOWLIST=build/agent_state/636_step1_branch_source_freshness.allowlist`.
The probe still exits `1` (`total=3 passed=0 failed=3`), but none of the
first failures is the original
`missing_source_freshness_authority` / no-candidate freshness blocker.

Fresh prepared dumps under
`build/agent_state/636_step4_rv64_consumption_boundary/` show the moved target
rows still publish prepared `BranchStackLoadSource` / `BranchStackSlot`
freshness before RV64 consumption:

- `src/930930-1.c`: `function=f`, `block=block_1`, `role=lhs`,
  `value=%t1`, `status=available`, `source_freshness_status=selected`,
  `source_freshness_candidates=1`.
- `src/990127-1.c`: `function=main`, `block=block_1`, `role=lhs`,
  `value=%t6`, `status=available`, `source_freshness_status=selected`,
  `source_freshness_candidates=1`.
- `src/20060910-1.c`: `function=check_header`, `block=block_2`,
  `role=rhs`, `value=%t9`, `status=available`,
  `source_freshness_status=selected`, `source_freshness_candidates=1`.

Current row classifications:

- `src/930930-1.c`: first owner is terminator lowering. The case stops at
  `unsupported_terminator_fragment: BIR terminator requires unsupported RV64
  object lowering` after the prepared `block_1`/`lhs` `%t1` branch stack-load
  authority is selected.
- `src/990127-1.c`: the original `block_1`/`lhs` `%t6` row is selected, then
  RV64 reaches a separate same-block `role=rhs`, `value=%lv.a` row with
  `authority_status=home_value_mismatch`,
  `source_freshness_status=missing_value`, and
  `source_freshness_candidates=0`. First owner is home/value identity
  reconciliation, not another prepared freshness publication packet.
- `src/20060910-1.c`: first owner is terminator lowering. The case stops at
  `unsupported_terminator_fragment: BIR terminator requires unsupported RV64
  object lowering` after the prepared `check_header` `block_2`/`rhs` `%t9`
  branch stack-load authority is selected.

Recommendation: idea 636 is close-ready from the executor perspective. The
remaining representative failures have moved to terminator lowering and
home/value mismatch owners; no additional freshness-publication packet is
justified by the current first-failure evidence. Residual work should be split
or routed to those owner tracks rather than continued in this source idea.

## Suggested Next

Supervisor should hand idea 636 to the plan owner for close/disposition, with
follow-up routing for the residual terminator-lowering rows and the
`990127-1.c` RHS `%lv.a` home/value mismatch.

## Watchouts

- Do not infer freshness in RV64 consumers.
- Do not merge idea-635 clobber-safety work into this plan.
- Do not treat expectation, unsupported-marker, allowlist, timeout, runtime,
  accounting, helper rename, or diagnostic-only edits as capability progress.
- The repaired block lookup intentionally avoids raw `block.label_id ==
  prepared_label` comparisons because BIR and prepared labels live in distinct
  name tables; cross-table numeric id collisions were the source of the stale
  `unsupported_terminator value=<none>` producer rows.
- `990127-1.c` still contains other `missing_policy` / no-candidate prepared
  rows, but the current RV64 first failure is the RHS `%lv.a`
  `home_value_mismatch`; do not recast that as idea-636 source freshness work.

## Proof

Required delegated proof command ran and preserved `test_after.log`:

`cmake --build --preset default && ALLOWLIST=build/agent_state/636_step1_branch_source_freshness.allowlist BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh > test_after.log 2>&1`

Result: build succeeded (`ninja: no work to do`); row probe exited `1` with
`total=3 passed=0 failed=3` because all three representative rows still fail
under later owners. The nonzero result is sufficient for classification but
not a backend pass.

Focused evidence artifacts:

- `test_after.log`
- `build/agent_state/636_step4_rv64_consumption_boundary/summary.md`
- `build/agent_state/636_step4_rv64_consumption_boundary/*.prepared.txt`
- Per-case first-failure logs under `build/rv64_gcc_c_torture_backend/`.
