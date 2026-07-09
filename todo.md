Status: Active
Source Idea Path: ideas/open/636_prepared_branch_stack_source_freshness_publication.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Refresh No-Candidate Branch Stack-Source Evidence

# Current Packet

## Just Finished

Step 1 evidence refresh completed for the idea-636 representative rows.
Fresh proof still places all three rows in the no-candidate branch stack-load
source freshness bucket:

- `src/930930-1.c`: `unsupported_branch_stack_load_source_freshness`;
  `function=f`, `block=block_1`, `block_index=3`,
  `terminator_instruction_index=2`, `role=lhs`, `value=%t1`;
  `authority_status=missing_source_freshness_authority`,
  `source_freshness_status=no_candidate`, `source_freshness_candidates=0`.
- `src/990127-1.c`: `unsupported_branch_stack_load_source_freshness`;
  `function=main`, `block=block_1`, `block_index=3`,
  `terminator_instruction_index=2`, `role=lhs`, `value=%t6`;
  `authority_status=missing_source_freshness_authority`,
  `source_freshness_status=no_candidate`, `source_freshness_candidates=0`.
- `src/20060910-1.c`: `unsupported_branch_stack_load_source_freshness`;
  `function=check_header`, `block=block_2`, `block_index=3`,
  `terminator_instruction_index=3`, `role=rhs`, `value=%t9`;
  `authority_status=missing_source_freshness_authority`,
  `source_freshness_status=no_candidate`, `source_freshness_candidates=0`.

No row reclassified out of the idea-636 bucket. Prepared dump evidence under
`build/agent_state/636_step1_branch_source_freshness/` shows the shared
prepared surface already has the branch condition, stack-slot home, object, and
branch block identity for the target value relation, but the collected
`branch_stack_load_authority` rows for the target fused pointer blocks currently
publish no matching source freshness candidate. The target producer surface is
`collect_prepared_branch_stack_load_authorities(...)` /
`make_branch_stack_load_authority_record(...)` feeding
`publish_prepared_branch_stack_source_freshness_candidate(...)`, not RV64
target-local freshness inference.

## Suggested Next

Delegate Step 2/3 as a focused prepared producer packet: add producer-side
coverage for one legal fused pointer branch stack-slot source and then repair
`collect_prepared_branch_stack_load_authorities(...)` so the target LHS/RHS
stack-slot value publishes exactly one `BranchStackLoadSource` /
`BranchStackSlot` freshness candidate at the branch terminator ordering point.

## Watchouts

- Do not infer freshness in RV64 consumers.
- Do not merge idea-635 clobber-safety work into this plan.
- Do not treat expectation, unsupported-marker, allowlist, timeout, runtime,
  accounting, helper rename, or diagnostic-only edits as capability progress.
- The prepared printer currently shows target fused pointer block authority rows
  as `unsupported_terminator` with `value=<none>` while RV64 reports zero
  matching candidates; the next packet should resolve the prepared collection
  source of that mismatch before any consumer-side work.

## Proof

Created `build/agent_state/636_step1_branch_source_freshness.allowlist` with
exactly the three delegated rows and ran:

`cmake --build --preset default && ALLOWLIST=build/agent_state/636_step1_branch_source_freshness.allowlist BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh > test_after.log 2>&1`

Result: build up to date; focused row probe exited `1` with `total=3 passed=0
failed=3`, preserving the expected current blockers in `test_after.log` and
per-case logs under `build/rv64_gcc_c_torture_backend/`. Additional focused
diagnostic artifacts are under
`build/agent_state/636_step1_branch_source_freshness/`.
