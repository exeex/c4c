Status: Active
Source Idea Path: ideas/open/306_rv64_c_testsuite_undefined_main_triage_and_followups.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Classify All Undefined-Main Cases

# Current Packet

## Just Finished

Step 3, Classify All Undefined-Main Cases, classified all 93 cases from
`build/rv64_c_testsuite_probe_v2/undefined_main_cases.txt` exactly once in
`build/rv64_c_testsuite_probe_v2/classification.tsv`, generated exact
per-bucket lists under
`build/rv64_c_testsuite_probe_v2/classification_work/buckets/`, captured
per-case prepared-BIR evidence under
`build/rv64_c_testsuite_probe_v2/classification_work/prepared_bir/`, and
summarized the bucket counts and representative evidence in
`build/rv64_c_testsuite_probe_v2/classification.md`.

## Suggested Next

Begin Step 4 repair-order planning. Start with the shared
`rv64_text_only_fail_closed` output-contract failure so prepared `main`
functions emit `.globl main`, a `main:` label, and a body before sequencing
follow-up repairs for string/external-call materialization, aggregate/global
storage, pointer globals, floating globals, scalar globals, and the remaining
aggregate/control-only shapes.

## Watchouts

- All 93 undefined-main cases have `prepared.func @main`; this is not a
  frontend or prepared-BIR loss of `main`.
- All 93 existing RV64 asm files are `.text` only; treat that fail-closed
  output contract as the primary root cause.
- `src/00094.c` remains the minimal unused-extern control: no prepared
  storage/addressing demand, but still `.text` only.
- Secondary buckets are follow-up repair hazards, not competing explanations
  for the undefined-main linker failures.
- Do not register the full 220-case sweep as required CTest coverage.
- Do not treat empty `.text` output as a valid successful unsupported path.
- Keep root-level scratch logs clean.

## Proof

Ran the delegated proof command:

```sh
{ cmake --build --preset default && test -f build/rv64_c_testsuite_probe_v2/undefined_main_cases.txt && test -f build/rv64_c_testsuite_probe_v2/classification.tsv && test "$(wc -l < build/rv64_c_testsuite_probe_v2/undefined_main_cases.txt)" = 93 && test "$(tail -n +2 build/rv64_c_testsuite_probe_v2/classification.tsv | wc -l)" = 93 && awk -F '\t' 'NR>1 {primary[$2]++; secondary[$3]++} END {print "primary buckets:"; for (k in primary) print k "=" primary[k]; print "secondary buckets:"; for (k in secondary) print k "=" secondary[k]}' build/rv64_c_testsuite_probe_v2/classification.tsv | sort; } > test_after.log 2>&1
```

Result: passed. The proof log is `test_after.log`.
