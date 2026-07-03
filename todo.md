Status: Active
Source Idea Path: ideas/open/549_rv64_runtime_and_no_diagnostic_triage.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reconstruct Representative No-Diagnostic Families

# Current Packet

## Just Finished

Step 1 evidence reconstruction ran for representative RV64 gcc_torture
no-diagnostic families.

Artifact provenance:

- Literal 503/57/7 count statement found in
  `ideas/open/549_rv64_runtime_and_no_diagnostic_triage.md`; no standalone
  generated bucket-count file with those exact three counts was found under
  `build/agent_state` outside archived agent logs.
- Freshest current scan/log pointer:
  `build/agent_state/rv64_gcc_torture_backend_current_log_path.txt` ->
  `build/agent_state/rv64_gcc_torture_backend_current_20260703T015523Z.log`.
- Freshest no-diagnostic family map found:
  `build/agent_state/546_step3_instruction_fragment_classification.tsv`
  (2026-07-03 02:07 UTC), 265 classified rows. Family counts from that map:
  `join_phi_branch_publication=56`,
  `local_memory_or_stack_value=36`,
  `global_memory_addressing_small=33`,
  `integer_div_rem=30`,
  `f128_or_long_double_primary=28`,
  `select_join_or_value_publication=18`,
  `scalar_fp_cast_or_op=18`,
  `scalar_integer_binary=15`,
  `integer_arithmetic_shift_right=12`,
  `pointer_integer_cast=12`,
  `call_adjacent_or_helper=5`,
  `evidence_gap=2`.
- Related raw scan state:
  `build/agent_state/rv64_gcc_c_torture_backend_summary.full.tsv`
  (1467 rows, 1152 failures in
  `build/agent_state/rv64_gcc_c_torture_backend_failed.full.txt`) and current
  smoke/current summary `build/agent_state/rv64_gcc_c_torture_backend_summary.tsv`
  (1 row).

Representative rerun evidence was written under
`build/agent_state/549_step1_no_diagnostic_families/`:

| Family | Representative | Exit mode | Log |
| --- | --- | --- | --- |
| `join_phi_branch_publication` | `src/20030408-1.c` | compile fail, rc=1, `unsupported_instruction_fragment` | `build/agent_state/549_step1_no_diagnostic_families/src_20030408-1.c/case.log` |
| `local_memory_or_stack_value` | `src/20000412-2.c` | compile fail, rc=1, `unsupported_instruction_fragment` | `build/agent_state/549_step1_no_diagnostic_families/src_20000412-2.c/case.log` |
| `global_memory_addressing_small` | `src/20071211-1.c` | compile fail, rc=1, `unsupported_instruction_fragment` | `build/agent_state/549_step1_no_diagnostic_families/src_20071211-1.c/case.log` |
| `select_join_or_value_publication` | `src/pr51933.c` | compile fail, rc=1, `unsupported_instruction_fragment` | `build/agent_state/549_step1_no_diagnostic_families/src_pr51933.c/case.log` |
| `call_adjacent_or_helper` | `src/pr56982.c` | compile fail, rc=1, `unsupported_instruction_fragment` | `build/agent_state/549_step1_no_diagnostic_families/src_pr56982.c/case.log` |
| `integer_div_rem` | `src/20001026-1.c` | compile fail, rc=1, `prepared_consumer_category=ambiguous_non_parallel_multi_source_stack_destination` | `build/agent_state/549_step1_no_diagnostic_families/src_20001026-1.c/case.log` |
| `f128_or_long_double_primary` | `src/20000910-1.c` | compile fail, rc=1, `unsupported_instruction_fragment`; quarantine lane | `build/agent_state/549_step1_no_diagnostic_families/src_20000910-1.c/case.log` |
| `scalar_fp_cast_or_op` | `src/20000605-1.c` | compile fail, rc=1, `unsupported_instruction_fragment` | `build/agent_state/549_step1_no_diagnostic_families/src_20000605-1.c/case.log` |
| `integer_arithmetic_shift_right` | `src/pr78438.c` | compile fail, rc=1, `unsupported_instruction_fragment` | `build/agent_state/549_step1_no_diagnostic_families/src_pr78438.c/case.log` |
| `pointer_integer_cast` | `src/20000622-1.c` | compile fail, rc=1, `unsupported_instruction_fragment` | `build/agent_state/549_step1_no_diagnostic_families/src_20000622-1.c/case.log` |
| `scalar_integer_binary` | `src/20000819-1.c` | compile fail, rc=1, `unsupported_instruction_fragment` | `build/agent_state/549_step1_no_diagnostic_families/src_20000819-1.c/case.log` |
| `evidence_gap` | `src/20030307-1.c` | compile fail, rc=1, `unsupported_instruction_fragment` | `build/agent_state/549_step1_no_diagnostic_families/src_20030307-1.c/case.log` |

Command and detail manifests:

- `build/agent_state/549_step1_no_diagnostic_families/commands.sh.txt`
- `build/agent_state/549_step1_no_diagnostic_families/reproduction_summary.tsv`
- `build/agent_state/549_step1_no_diagnostic_families/reproduction_details.tsv`

## Suggested Next

Plan owner or supervisor should decide whether the missing standalone 503/57/7
bucket-count artifact is acceptable as a provenance gap. If acceptable, execute
Step 2 against the recorded representatives, starting with focused prepared/BIR
dumps for the generic `unsupported_instruction_fragment` rows and keeping
`src/20000910-1.c` in the F128 quarantine lane.

## Watchouts

- Do not implement fixes during the triage packet.
- Do not weaken runtime comparison, expected outputs, unsupported markers, or pass/fail accounting.
- Route primary-F128 cases to the F128 quarantine lane instead of ordinary-C repair.
- Treat named-case-only repairs as route drift, not progress.
- The exact generated bucket map containing the literal 503 compile-fail,
  57 other-fail, and 7 segfault counts remains missing/ambiguous; the freshest
  concrete no-diagnostic family map found has 265 rows and should not be
  overclaimed as the complete 567-row map.
- `src/20001026-1.c` is classified as `integer_div_rem` in the July 3 family
  map but now reproduces at the prepared move-bundle classifier boundary; do
  not claim this row as a clean integer-div/rem lowering representative without
  Step 2 owner classification.

## Proof

Executor proof:

- `cmake --build build --target c4cll`
- Per-case CMake runner invocations recorded in
  `build/agent_state/549_step1_no_diagnostic_families/commands.sh.txt`
- `git diff --check -- todo.md && scripts/plan_review_state.py show`
