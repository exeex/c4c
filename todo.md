Status: Active
Source Idea Path: ideas/open/506_rv64_out_of_ssa_phi_join_immediate_materialization.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Validate And Prepare Closure Evidence

# Current Packet

## Just Finished

Completed plan Step 4 validation and closure-evidence preparation for idea 506.

Closure evidence:

- Step 2 commit `86e66eba5` materialized explicit generic out-of-SSA
  immediate-to-register phi joins.
- Step 3 commit `e04a4c687` added and kept source-authority coverage, including
  the repaired gate that requires the authoritative parallel-copy source to be
  an I32 immediate matching `source_immediate_i32`.
- Accepted backend proof was rolled into `test_before.log` after Step 3:
  `^backend_` passed 328/328, including `backend_riscv_object_emission`, with
  clean `git diff --check`.
- Supervisor broader validation ran
  `ctest --test-dir build -j2 --output-on-failure` and produced 3356/3357
  passed. The only failure was the pre-existing unrelated
  `string_authority_guard` baseline issue in
  `SameModuleFormalPointerProvenanceMap`; `backend_riscv_object_emission`
  passed in that broad run.
- No stale test processes remained after the broad validation run.
- The remaining unsupported families are outside idea 506 or remain
  fail-closed: stack destinations, memory destinations, pointer immediates,
  cycle/temp moves, non-`source_imm_i32` source shapes, and rows missing
  bundle-level edge coordinates or other coherent prepared authority.

Executor assessment: idea 506 appears ready for plan-owner closure review,
subject to supervisor acceptance of the recorded narrow proof and the known
unrelated broad-validation baseline failure.

## Suggested Next

Supervisor can hand this closure-evidence packet to the plan owner for idea 506
closure review.

## Watchouts

- The materialization is intentionally inside the out-of-SSA parallel-copy path,
  not the select-publication path.
- Missing bundle-level edge coordinates can be rejected by the shared prepared
  consumer classifier before RV64-local lowering runs; RV64 still has a
  fail-closed coordinate gate for any bundle that reaches target lowering.
- Stack destinations, memory destinations, pointer immediates, cycle/temp
  moves, and non-`source_imm_i32` source shapes remain unsupported.
- Do not drop the new source-shape test: it is a non-overfit authority check,
  not a testcase-name or source-file match.
- Do not accept or alter baseline candidates for the unrelated
  `string_authority_guard` failure as part of idea 506 closure.
- Evidence note: `build/agent_state/506_step4_generic_immediate_phi_join_validation/summary.md`.

## Proof

Delegated proof command:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed > test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Result: passed. The monotonic regression guard compared `test_before.log` to
itself and reported 328 passed, 0 failed, 328 total on both sides; `git diff
--check` passed. Output preserved in `test_after.log`.
