# BIR Semantic Admission Classification

Status: Step 2 classification of the 2026-07-02 RV64 gcc_torture backend scan rows reconstructed in `bir_semantic_admission_rows.md`.

## Evidence Anchor

- Source row artifact: `docs/rv64_gcc_torture_post_contract/bir_semantic_admission_rows.md`
- Current scan pointer: `build/agent_state/rv64_gcc_torture_backend_current_log_path.txt` -> `build/agent_state/rv64_gcc_torture_backend_current_20260702T151551Z.log`
- Per-case log root: `build/rv64_gcc_c_torture_backend/<case-id>/case.log`
- Exact semantic row set classified here: `373` rows whose current per-case log contains `semantic lir_to_bir` and a visible `function ... failed in ...` first-topic line.
- Related bootstrap handoff rows kept separate: `44` rows with `lir_to_bir lowering before the prepared object handoff: bootstrap`.

## Classification Commands

Count first visible exact semantic topics from the Step 1 table:

```sh
awk -F'|' '/\| `src\// {
  kind=$3
  topic=$5
  gsub(/^ +| +$/,"",kind)
  gsub(/^ +| +$/,"",topic)
  if (kind=="exact semantic") count[topic]++
  else if (kind=="related bootstrap") boot[topic]++
}
END {
  print "exact topics"
  for (k in count) print count[k] "\t" k
  print "bootstrap"
  for (k in boot) print boot[k] "\t" k
}' docs/rv64_gcc_torture_post_contract/bir_semantic_admission_rows.md | sort -nr
```

Inspect representative logs for each admitted topic family:

```sh
for f in \
  src_pr82388.c \
  src_20000717-4.c \
  src_20011008-3.c \
  src_20000412-2.c \
  src_stdarg-4.c \
  src_20000703-1.c \
  src_20050604-1.c \
  src_20041218-1.c \
  src_20000314-3.c \
  src_20050316-3.c \
  src_complex-1.c \
  src_960513-1.c \
  src_strlen-2.c
do
  sed -n '1,40p' "build/rv64_gcc_c_torture_backend/$f/case.log"
done
```

Confirm current per-case logs publish the first-topic line shape used for classification:

```sh
rg -n \
  'function .* failed in|only supports scalar|semantic admission|prepared object handoff' \
  build/rv64_gcc_c_torture_backend -g case.log
```

## Owner Summary

| Classification lane | First owner | Rows | Included first visible topics | Routing decision |
| --- | --- | ---: | --- | --- |
| Local-memory facts | BIR semantic local-memory producer | 264 | `load local-memory`, `gep local-memory`, `store local-memory`, `scalar/local-memory`, `alloca local-memory` | BIR producer follow-up candidate |
| Call metadata | BIR semantic call producer | 55 | `direct-call`, `call-return` | BIR producer follow-up candidate |
| Runtime/intrinsic memory facts | BIR runtime/intrinsic producer | 34 | `memcpy runtime`, `memset runtime` | BIR producer follow-up candidate, separate from generic local-memory |
| Scalar, signature, and control facts | BIR scalar/signature/control producers | 20 | `scalar-control-flow`, `function-signature`, `scalar-binop` | BIR producer follow-up candidate, not a RV64/MIR route |
| Aggregate facts as a first owner | none proven by exact row evidence | 0 | none | Do not create an aggregate-first producer claim from these rows |
| Publication gaps as a first owner | none proven by exact row evidence | 0 | none | Do not route to prepared/RV64 publication without row evidence |
| Malformed or intentionally rejected semantic inputs | none proven by exact row evidence | 0 | none | No exact row currently shows this as first owner |
| Unknown/evidence gap | none | 0 | none | All `373` exact rows expose a first-topic line |
| Related bootstrap handoff lane | BIR bootstrap/global data-shape support | 44 | scalar integer/pointer globals, linear integer-array globals, aggregate-backed globals with byte-address semantics | Related non-semantic lane; not counted in the `373` exact semantic rows |

Exact semantic rows accounted for by first owner: `264 + 55 + 34 + 20 = 373`.

## Producer Topic Counts

| Producer topic | Rows | Classified owner |
| --- | ---: | --- |
| `load local-memory semantic family` | 79 | BIR semantic local-memory producer |
| `gep local-memory semantic family` | 62 | BIR semantic local-memory producer |
| `store local-memory semantic family` | 58 | BIR semantic local-memory producer |
| `semantic call family 'direct-call semantic family'` | 52 | BIR semantic call producer |
| `scalar/local-memory semantic family` | 49 | BIR semantic local-memory producer |
| `runtime/intrinsic family 'memcpy runtime family'` | 19 | BIR runtime/intrinsic producer |
| `alloca local-memory semantic family` | 16 | BIR semantic local-memory producer |
| `runtime/intrinsic family 'memset runtime family'` | 15 | BIR runtime/intrinsic producer |
| `scalar-control-flow semantic family` | 10 | BIR scalar/control-flow producer |
| `function-signature semantic family` | 9 | BIR function-signature producer |
| `semantic call family 'call-return semantic family'` | 3 | BIR semantic call producer |
| `scalar-binop semantic family` | 1 | BIR scalar producer |

## Representative Evidence

| Family | Representative log | Visible first-topic evidence |
| --- | --- | --- |
| Store local-memory | `build/rv64_gcc_c_torture_backend/src_pr82388.c/case.log` | `function 'main' failed in store local-memory semantic family` |
| GEP local-memory | `build/rv64_gcc_c_torture_backend/src_20000717-4.c/case.log` | `function 'x' failed in gep local-memory semantic family` |
| Load local-memory | `build/rv64_gcc_c_torture_backend/src_20011008-3.c/case.log` | `function '__db_txnlist_lsnadd' failed in load local-memory semantic family` |
| Scalar/local-memory | `build/rv64_gcc_c_torture_backend/src_stdarg-4.c/case.log` | `function 'f1' failed in scalar/local-memory semantic family` |
| Alloca local-memory | `build/rv64_gcc_c_torture_backend/src_20050604-1.c/case.log` | `function 'foo' failed in alloca local-memory semantic family` |
| Direct-call metadata | `build/rv64_gcc_c_torture_backend/src_20000412-2.c/case.log` | `function 'main' failed in semantic call family 'direct-call semantic family'` |
| Call-return metadata | `build/rv64_gcc_c_torture_backend/src_complex-1.c/case.log` | `function 'main' failed in semantic call family 'call-return semantic family'` |
| Memcpy intrinsic | `build/rv64_gcc_c_torture_backend/src_20000703-1.c/case.log` | `function 'foo' failed in runtime/intrinsic family 'memcpy runtime family'` |
| Memset intrinsic | `build/rv64_gcc_c_torture_backend/src_20041218-1.c/case.log` | `function 'baz' failed in runtime/intrinsic family 'memset runtime family'` |
| Scalar control-flow | `build/rv64_gcc_c_torture_backend/src_20000314-3.c/case.log` | `function 'attr_eq' failed in scalar-control-flow semantic family` |
| Function signature | `build/rv64_gcc_c_torture_backend/src_20050316-3.c/case.log` | `function 'test1' failed in function-signature semantic family` |
| Scalar binop | `build/rv64_gcc_c_torture_backend/src_960513-1.c/case.log` | `function 'f' failed in scalar-binop semantic family` |
| Bootstrap/global data-shape handoff | `build/rv64_gcc_c_torture_backend/src_strlen-2.c/case.log` | `lir_to_bir only supports scalar integer/pointer globals, linear integer-array globals, and aggregate-backed globals with honest byte-address semantics right now` |

## Non-BIR And Evidence-Gap Lanes

- Prepared contract gaps: `0` rows in the exact `373` semantic row set. The visible current diagnostics stop at `semantic lir_to_bir`, before prepared-object handoff can own the row.
- RV64/MIR object lowering: `0` rows in the exact `373` semantic row set. No exact semantic row is routed downstream without a BIR producer fact.
- Runtime mismatch: `0` rows in the exact `373` semantic row set. These rows fail compile/object admission before runtime comparison.
- Test infrastructure: `0` rows in the exact `373` semantic row set.
- F128 quarantine as first owner: `0` rows proven by exact row evidence. Some case names may be floating-point-oriented, but the current first-topic diagnostics are BIR semantic producer topics, not F128 quarantine diagnostics.
- Bootstrap/global data-shape handoff: `44` related rows, kept separate from exact semantic rows. These are BIR handoff-related but not `semantic lir_to_bir` producer rows.
- Evidence gaps: `0` exact semantic rows. Every exact row has a visible first-topic line in the current per-case log.

## Classification Notes

- The current diagnostics provide family-level first ownership, not a per-row missing-field payload. Follow-up work should refine each producer family with focused BIR tests before implementation.
- Aggregate facts are not first-owner rows in this packet. Aggregate behavior may be involved inside local-memory, call, runtime, or bootstrap cases, but the exact-row evidence does not justify routing any of the `373` semantic rows to an aggregate-first owner.
- Publication gaps are also not first-owner rows in this packet. Do not turn these rows into prepared/RV64 publication work unless a later log exposes publication facts as the first failing producer boundary.
- The `44` bootstrap rows are related to BIR admission and global/data shape support, but they are intentionally excluded from the `373` exact semantic producer count.
