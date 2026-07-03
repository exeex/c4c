Status: Active
Source Idea Path: ideas/open/549_rv64_runtime_and_no_diagnostic_triage.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify First Owners

# Current Packet

## Just Finished

Step 2 classified first-owner boundaries for the Step 1 representative RV64
gcc_torture no-diagnostic families.

Evidence written under
`build/agent_state/549_step2_first_owner_classification/`:

- `stage_matrix.tsv`: per-representative rc matrix for `--dump-bir`,
  `--dump-prepared-bir`, `--dump-mir`, and the RV64 object runner.
- `classification.tsv`: first-owner classification with source family,
  object diagnostic, BIR operation summary, confidence, and log paths.
- `supporting_facts.tsv`: compact grep facts from object logs or dumps.
- `commands.sh.txt`: exact dump and object-runner commands.
- `classification.md`: readable owner summary and caveats.

Shared stage fact: all 12 representatives returned rc=0 for `--dump-bir`,
`--dump-prepared-bir`, and `--dump-mir`; all 12 returned rc=1 only at the
RV64 object runner. That separates these samples from parser/HIR/BIR producer,
runtime mismatch, timeout, and test-infrastructure failures.

| Family | Representative | First owner | Evidence |
| --- | --- | --- | --- |
| `join_phi_branch_publication` | `src/20030408-1.c` | RV64 object lowering, medium confidence | BIR/prepared/MIR rc=0, object rc=1 with `unsupported_instruction_fragment`; log `build/agent_state/549_step2_first_owner_classification/src_20030408-1.c/object-route.log` |
| `local_memory_or_stack_value` | `src/20000412-2.c` | RV64 object lowering, medium confidence | BIR/prepared/MIR rc=0, object rc=1 with `unsupported_instruction_fragment`; log `build/agent_state/549_step2_first_owner_classification/src_20000412-2.c/object-route.log` |
| `global_memory_addressing_small` | `src/20071211-1.c` | RV64 object lowering, medium confidence | BIR/prepared/MIR rc=0, object rc=1 with `unsupported_instruction_fragment`; source flagged inline asm, but no explicit unsupported-inline-asm diagnostic fired |
| `select_join_or_value_publication` | `src/pr51933.c` | RV64 object lowering, medium confidence | BIR/prepared/MIR rc=0, object rc=1 with `unsupported_instruction_fragment`; BIR summary has heavy `select`, `load_global`, and `store_global` presence |
| `call_adjacent_or_helper` | `src/pr56982.c` | RV64 object lowering, medium confidence | BIR/prepared/MIR rc=0, object rc=1 with `unsupported_instruction_fragment`; BIR summary has call-adjacent shape |
| `integer_div_rem` | `src/20001026-1.c` | Prepared contract/classifier, high confidence | object log names `prepared_consumer_category=ambiguous_non_parallel_multi_source_stack_destination` and prepared move-bundle classifier rejection |
| `f128_or_long_double_primary` | `src/20000910-1.c` | F128 quarantine, with caveat | Step 1 map selected this family; dumps contain F128 declarations/sections from `stdlib.h`, but the local test body is ordinary integer/pointer code, so do not use as direct F128-operation repair proof |
| `scalar_fp_cast_or_op` | `src/20000605-1.c` | RV64 object lowering, medium confidence | BIR/prepared/MIR rc=0, object rc=1 with `unsupported_instruction_fragment`; BIR summary includes `sitofp`/`fptosi` |
| `integer_arithmetic_shift_right` | `src/pr78438.c` | RV64 object lowering, medium confidence | BIR/prepared/MIR rc=0, object rc=1 with `unsupported_instruction_fragment`; source flagged inline asm, but no explicit unsupported-inline-asm diagnostic fired |
| `pointer_integer_cast` | `src/20000622-1.c` | RV64 object lowering, medium confidence | BIR/prepared/MIR rc=0, object rc=1 with `unsupported_instruction_fragment`; family map selected pointer/integer cast shape |
| `scalar_integer_binary` | `src/20000819-1.c` | RV64 object lowering, medium confidence | BIR/prepared/MIR rc=0, object rc=1 with `unsupported_instruction_fragment`; BIR summary includes scalar `add` |
| `evidence_gap` | `src/20030307-1.c` | Evidence gap, low confidence | BIR/prepared/MIR rc=0, object rc=1 with only generic `unsupported_instruction_fragment`; no specific first bad instruction or authority boundary in current evidence |

## Suggested Next

Execute Step 3: split durable follow-up ideas only for high-confidence owner
families. Suggested split candidates are a prepared move-bundle classifier idea
for `src/20001026-1.c`, a generic RV64 object-lowering instrumentation or
diagnostic idea for the nine `unsupported_instruction_fragment` rows, and a
separate quarantine/evidence note for `src/20000910-1.c`.

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
  additional evidence.
- `unsupported_instruction_fragment` proves the failure reaches RV64 object
  lowering after BIR/prepared/MIR dumps succeed, but it does not identify the
  exact unsupported instruction. Treat these rows as classification evidence,
  not implementation-ready opcode repairs.
- No representative produced a runtime mismatch, timeout, segmentation fault,
  or test-infrastructure failure in this packet.

## Proof

Executor proof:

- `cmake --build build --target c4cll`
- Focused `--dump-bir`, `--dump-prepared-bir`, `--dump-mir`, and RV64 object
  runner commands recorded in
  `build/agent_state/549_step2_first_owner_classification/commands.sh.txt`
- `git diff --check -- todo.md && scripts/plan_review_state.py show`
