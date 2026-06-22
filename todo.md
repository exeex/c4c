Status: Active
Source Idea Path: ideas/open/306_rv64_c_testsuite_undefined_main_triage_and_followups.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Normalize the Probe Result

# Current Packet

## Just Finished

Step 1, Normalize the Probe Result, recorded the existing
`build/rv64_c_testsuite_probe_v2/summary.tsv` result in
`build/rv64_c_testsuite_probe_v2/normalized_summary.md` and wrote the exact
93-case undefined-main bucket to
`build/rv64_c_testsuite_probe_v2/undefined_main_cases.txt`.

## Suggested Next

Begin the next packet by triaging the normalized undefined-main list into
semantic follow-up buckets without regenerating the broad probe.

## Watchouts

- Keep this triage focused on classification and follow-up idea creation.
- Do not register the full 220-case sweep as required CTest coverage.
- Do not treat empty `.text` output as a valid successful unsupported path.
- Keep root-level scratch logs clean.
- `CLANG_FAIL=95` contains 93 undefined-main linker failures plus two separate
  undefined temporary-symbol failures (`src/00034.c`, `src/00127.c`).

## Proof

Ran the delegated proof command:

```sh
{ cmake --build --preset default && test -f build/rv64_c_testsuite_probe_v2/summary.tsv && awk -F '\t' 'NR>1 {status[$2]++; total++} END {print "total=" total; for (k in status) print k "=" status[k]}' build/rv64_c_testsuite_probe_v2/summary.tsv | sort; } > test_after.log 2>&1
```

Result: passed. The proof log is `test_after.log` and records `total=220`,
`PASS=62`, `QEMU_NONZERO=60`, `CLANG_FAIL=95`, and `EMIT_FAIL=3`.
