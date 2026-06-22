# RV64 c-testsuite Probe v2 Normalized Summary

Date: 2026-06-22 16:34:44 UTC

Command context:

```sh
cmake --build --preset default
test -f build/rv64_c_testsuite_probe_v2/summary.tsv
awk -F '\t' 'NR>1 {status[$2]++; total++} END {print "total=" total; for (k in status) print k "=" status[k]}' build/rv64_c_testsuite_probe_v2/summary.tsv | sort
```

Input artifact: `build/rv64_c_testsuite_probe_v2/summary.tsv`

## Status Counts

| Status | Count |
| --- | ---: |
| total | 220 |
| PASS | 62 |
| QEMU_NONZERO | 60 |
| CLANG_FAIL | 95 |
| EMIT_FAIL | 3 |

## Failure Buckets

| Bucket | Status | Count | Notes |
| --- | --- | ---: | --- |
| undefined-main linker failure | CLANG_FAIL | 93 | Detected by linker diagnostics from `Scrt1.o: in function '_start'`; the undefined-main count is still 93. |
| undefined temporary symbol | CLANG_FAIL | 2 | `src/00034.c` and `src/00127.c` report undefined `.Lmain_block_*` symbols, not undefined main. |
| semantic lir_to_bir required | EMIT_FAIL | 3 | Backend route rejects cases before prepared-module handoff. |
| qemu illegal instruction | QEMU_NONZERO | 40 | Runtime return code 132. |
| qemu segmentation fault | QEMU_NONZERO | 20 | Runtime return code 139. |

The exact undefined-main case list is recorded in `undefined_main_cases.txt`.
