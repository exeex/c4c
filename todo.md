Status: Active
Source Idea Path: ideas/open/547_bir_local_memory_call_metadata_boundary_review.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Reconstruct Call-Metadata Evidence

# Current Packet

## Just Finished

Step 2 reconstructed the available call-metadata evidence from existing RV64
gcc torture backend artifacts without creating new logs.

Evidence sources:
- `build/agent_state/rv64_gcc_c_torture_backend_summary.tsv` currently lists
  only `src/20001026-1.c` as failing, and that case reports
  `ambiguous_non_parallel_multi_source_stack_destination`, not
  a call argument, call return, variadic, helper-call, aggregate-call, or
  call-site metadata diagnostic.
- `build/agent_state/rv64_gcc_c_torture_backend_failed.txt` likewise contains
  only `src/20001026-1.c`, so the latest summary-file row set has no current
  call-metadata row.
- Existing per-case logs under `build/rv64_gcc_c_torture_backend/*/case.log`
  still contain 26 explicit semantic call-family rows from the retained RV64
  work-root sweep. These rows are current on disk, but they are not indexed by
  the latest summary/failed files because those files appear to have been
  overwritten by the later single-case run.
- `build/agent_state/rv64_gcc_c_torture_backend_summary.full.tsv` and
  `build/agent_state/rv64_gcc_c_torture_backend_failed.full.txt` are older
  broad-sweep summary artifacts. They agree that retained work-root case logs
  exist, but the row-level evidence below comes from the retained case logs,
  not from the latest summary-file row.

Verified retained work-root row group:
- `semantic_lir_to_bir_direct_call_family`: 26 rows. Diagnostic:
  `backend object route requires semantic lir_to_bir lowering before the
  prepared object handoff`; the latest function failure says the function
  `failed in semantic call family 'direct-call semantic family'`.
  Suspected producer boundary: semantic LIR-to-BIR direct-call lowering before
  prepared BIR handoff. First bad fact shape is not yet implementation-ready:
  the retained case logs name the direct-call semantic family, but they do not
  identify a missing call argument, call return, variadic, helper-call,
  aggregate-call, or call-site metadata fact.

Rows in that group:
- `src/20000419-1.c`
- `src/20000706-5.c`
- `src/20000707-1.c`
- `src/20000717-1.c`
- `src/20000717-5.c`
- `src/20011113-1.c`
- `src/20021118-1.c`
- `src/20030613-1.c`
- `src/20030914-1.c`
- `src/20040703-1.c`
- `src/20081117-1.c`
- `src/20180131-1.c`
- `src/20190820-1.c`
- `src/931004-1.c`
- `src/931004-11.c`
- `src/931004-13.c`
- `src/931004-3.c`
- `src/931004-5.c`
- `src/931004-7.c`
- `src/931004-9.c`
- `src/complex-1.c`
- `src/pr20621-1.c`
- `src/pr51323.c`
- `src/pr52129.c`
- `src/pr67226.c`
- `src/struct-ret-1.c`

Representative case logs:
- `build/rv64_gcc_c_torture_backend/src_20000419-1.c/case.log`
- `build/rv64_gcc_c_torture_backend/src_20000717-1.c/case.log`
- `build/rv64_gcc_c_torture_backend/src_20040703-1.c/case.log`
- `build/rv64_gcc_c_torture_backend/src_pr67226.c/case.log`

No-count suspicions kept out of implementation scope:
- 300 retained case logs mention `semantic call families` only inside the
  generic admitted-capability-bucket text. They are not call-metadata rows
  unless the `latest function failure` line names a call family.
- `build/rv64_gcc_c_torture_backend/src_stdarg-4.c/case.log` contains four
  `callee body contains va_arg` inline warnings, but its latest function
  failure is `load local-memory semantic family`, so it is not counted as a
  call-metadata row.
- `build/agent_state/558_step5_call_metadata_reconcile.allowlist` names
  `src/20000412-2.c` and `src/20050121-1.c`, but their retained case logs both
  report `unsupported_instruction_fragment`, not a call-metadata diagnostic.
- `build/agent_state/546_step3_call_adjacent_or_helper_pr56982_c_bir.txt` and
  `build/agent_state/546_step3_call_adjacent_or_helper_pr56982_c_prepared.txt`
  show call metadata being printed for `src/pr56982.c` (`call_arg_source`,
  `callsite`, `arg`, `result`, and `preserve` rows), but those dumps are
  supporting producer-surface evidence, not failing row evidence.

Step 2 conclusion:
- No current summary-file call-metadata row exists.
- The retained work-root case logs provide a 26-row direct-call semantic-family
  evidence set, but not an implementation-ready call-metadata fact gap. Step 3
  should inspect producer surfaces for this group only if the supervisor wants
  to prove whether the missing boundary is BIR semantic call lowering,
  prepared call metadata, or stale evidence.

## Suggested Next

Execute Step 3 by inspecting producer surfaces for the retained
`unsupported_local_memory_access` group and, if in scope, the 26 retained
`semantic_lir_to_bir_direct_call_family` rows to prove whether each group is
BIR-owned, prepared-owned, RV64-owned, or stale evidence.

## Watchouts

- Do not implement RV64 lowering in this review runbook.
- Do not treat call-metadata suspicions as implementation-ready before row-level
  evidence names a missing fact.
- Do not weaken tests, unsupported markers, expected output, or pass/fail
  accounting.
- Keep latest summary-file evidence separate from retained work-root case logs
  until a fresh full RV64 torture scan replaces the mixed summary state.
- Keep `review/557_step13_vector_local_memory_review.md` untouched unless the
  supervisor explicitly brings it into scope.

## Proof

Step 2 evidence-recording validation:

```sh
git diff --check -- todo.md && scripts/plan_review_state.py show
```

Result: passed. `scripts/plan_review_state.py show` reported
`current_step_id` = `2` and `current_step_title` =
`Reconstruct Call-Metadata Evidence`.
