# BIR Semantic Admission Rows

Status: current evidence reconstructed from the 2026-07-02 RV64 gcc_torture backend scan artifacts.

## Evidence Anchor

- Current scan pointer: `build/agent_state/rv64_gcc_torture_backend_current_log_path.txt` -> `build/agent_state/rv64_gcc_torture_backend_current_20260702T151551Z.log`
- Timestamped scan totals from pointer log: `1467` total, `349` passed, `1118` failed.
- Mutable summary caveat: `build/agent_state/rv64_gcc_c_torture_backend_summary.tsv` and `build/agent_state/rv64_gcc_c_torture_backend_failed.txt` are currently truncated to one failure row in this checkout, so this artifact reconstructs rows from the timestamped pointer log plus the current per-case `case.log` files.
- Per-case log root inspected: `build/rv64_gcc_c_torture_backend/<case-id>/case.log` (`1467` case logs present).

## Extraction Commands

```sh
awk -F '\t' '$1=="fail"{print $2 "\t" $3}' \
  build/agent_state/rv64_gcc_torture_backend_current_20260702T151551Z.log |
while IFS="$(printf '\t')" read -r case log; do
  if rg -q 'semantic lir_to_bir' "$log"; then
    printf '%s\t%s\n' "$case" "$log"
  fi
done | wc -l

awk -F '\t' '$1=="fail"{print $2 "\t" $3}' \
  build/agent_state/rv64_gcc_torture_backend_current_20260702T151551Z.log |
while IFS="$(printf '\t')" read -r case log; do
  if rg -q 'lir_to_bir lowering before the prepared object handoff: bootstrap' "$log"; then
    printf '%s\t%s\n' "$case" "$log"
  fi
done | wc -l

awk -F '\t' '$1=="fail"{print $2 "\t" $3}' \
  build/agent_state/rv64_gcc_torture_backend_current_20260702T151551Z.log |
while IFS="$(printf '\t')" read -r case log; do
  if rg -q 'semantic lir_to_bir' "$log"; then
    perl -0ne 'if (/function '\''([^'\'']+)'\'' failed in ((?:.|\n)*?)(?:\n\s*\n|\z)/s) { $fam=$2; $fam =~ s/\s+/ /g; $fam =~ s/\s+$//; print "$fam\n" } else { print "unknown/evidence-gap\n" }' "$log"
  fi
done | sort | uniq -c | sort -nr
```

## Result

- Exact `semantic lir_to_bir` admission candidate rows: `373`.
- Rows with enough visible evidence to identify a first BIR admission topic: `373`.
- Exact semantic rows without a visible topic line: `0`.
- Related BIR handoff/bootstrap admission-note rows, not counted as semantic producer rows: `44`.
- Total BIR handoff-related rows inspected for this packet: `417`.

## Topic Counts For Exact Semantic Rows

| First visible BIR admission topic | Rows | Evidence sufficiency |
| --- | ---: | --- |
| `load local-memory semantic family` | 79 | enough to identify first BIR producer/admission topic |
| `gep local-memory semantic family` | 62 | enough to identify first BIR producer/admission topic |
| `store local-memory semantic family` | 58 | enough to identify first BIR producer/admission topic |
| `semantic call family 'direct-call semantic family'` | 52 | enough to identify first BIR producer/admission topic |
| `scalar/local-memory semantic family` | 49 | enough to identify first BIR producer/admission topic |
| `runtime/intrinsic family 'memcpy runtime family'` | 19 | enough to identify first BIR producer/admission topic |
| `alloca local-memory semantic family` | 16 | enough to identify first BIR producer/admission topic |
| `runtime/intrinsic family 'memset runtime family'` | 15 | enough to identify first BIR producer/admission topic |
| `scalar-control-flow semantic family` | 10 | enough to identify first BIR producer/admission topic |
| `function-signature semantic family` | 9 | enough to identify first BIR producer/admission topic |
| `semantic call family 'call-return semantic family'` | 3 | enough to identify first BIR producer/admission topic |
| `scalar-binop semantic family` | 1 | enough to identify first BIR producer/admission topic |

## Related Bootstrap BIR Handoff Notes

| Related note | Rows | Evidence sufficiency |
| --- | ---: | --- |
| `lir_to_bir lowering before the prepared object handoff: bootstrap` / scalar integer-pointer globals only | 44 | related BIR admission note, but not an exact semantic producer row; first producer topic remains global/data-shape bootstrap support rather than semantic family admission |

## Representative Logs Inspected

| Case | Why inspected | Diagnostic evidence |
| --- | --- | --- |
| `src/pr82388.c` | semantic row representative | `function main failed in store local-memory semantic family` |
| `src/stdarg-4.c` | semantic row with preceding frontend warnings | `function f1 failed in scalar/local-memory semantic family` |
| `src/20000412-2.c` | semantic call-family row with wrapped diagnostic | `function main failed in semantic call family direct-call semantic family` |
| `src/complex-1.c` | semantic call-return row | `function main failed in semantic call family call-return semantic family` |
| `src/20000314-3.c` | scalar-control-flow row | `function attr_eq failed in scalar-control-flow semantic family` |
| `src/strlen-2.c` | related bootstrap BIR handoff row | `lir_to_bir only supports scalar integer/pointer globals...` |

## Current Candidate Row Table

The table below is reconstructed from the timestamped pointer scan log and current per-case logs. `topic_evidence=yes` means the log exposes a concrete first BIR admission topic after `function ... failed in ...`; bootstrap rows are retained as related admission notes but are not included in the exact semantic row count.

| Case | Candidate kind | Function | First visible topic | Topic evidence | Case log |
| --- | --- | --- | --- | --- | --- |
| `src/20000314-1.c` | exact semantic | `main` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20000314-1.c/case.log` |
| `src/20000314-3.c` | exact semantic | `attr_eq` | `scalar-control-flow semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20000314-3.c/case.log` |
| `src/20000412-2.c` | exact semantic | `main` | `semantic call family 'direct-call semantic family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20000412-2.c/case.log` |
| `src/20000419-1.c` | exact semantic | `main` | `semantic call family 'direct-call semantic family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20000419-1.c/case.log` |
| `src/20000519-1.c` | exact semantic | `foo` | `scalar/local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20000519-1.c/case.log` |
| `src/20000703-1.c` | exact semantic | `foo` | `runtime/intrinsic family 'memcpy runtime family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20000703-1.c/case.log` |
| `src/20000706-1.c` | exact semantic | `main` | `semantic call family 'direct-call semantic family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20000706-1.c/case.log` |
| `src/20000706-2.c` | exact semantic | `main` | `semantic call family 'direct-call semantic family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20000706-2.c/case.log` |
| `src/20000706-4.c` | exact semantic | `bar` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20000706-4.c/case.log` |
| `src/20000706-5.c` | exact semantic | `main` | `semantic call family 'direct-call semantic family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20000706-5.c/case.log` |
| `src/20000707-1.c` | exact semantic | `main` | `semantic call family 'direct-call semantic family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20000707-1.c/case.log` |
| `src/20000717-1.c` | exact semantic | `foo` | `semantic call family 'direct-call semantic family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20000717-1.c/case.log` |
| `src/20000717-4.c` | exact semantic | `x` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20000717-4.c/case.log` |
| `src/20000717-5.c` | exact semantic | `foo` | `semantic call family 'direct-call semantic family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20000717-5.c/case.log` |
| `src/20001024-1.c` | exact semantic | `foo` | `semantic call family 'direct-call semantic family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20001024-1.c/case.log` |
| `src/20001026-1.c` | exact semantic | `build_real_from_int_cst_1` | `store local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20001026-1.c/case.log` |
| `src/20001027-1.c` | exact semantic | `main` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20001027-1.c/case.log` |
| `src/20001101.c` | exact semantic | `main` | `semantic call family 'direct-call semantic family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20001101.c/case.log` |
| `src/20001124-1.c` | exact semantic | `do_isofs_readdir` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20001124-1.c/case.log` |
| `src/20001203-2.c` | exact semantic | `create_array_type` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20001203-2.c/case.log` |
| `src/20010116-1.c` | exact semantic | `main` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20010116-1.c/case.log` |
| `src/20010129-1.c` | exact semantic | `foo` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20010129-1.c/case.log` |
| `src/20010325-1.c` | related bootstrap | `(n/a)` | `bootstrap global/data shape admission note` | no; not exact semantic row | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20010325-1.c/case.log` |
| `src/20010409-1.c` | exact semantic | `test` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20010409-1.c/case.log` |
| `src/20010605-2.c` | exact semantic | `main` | `store local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20010605-2.c/case.log` |
| `src/20010910-1.c` | exact semantic | `epic_init_ring` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20010910-1.c/case.log` |
| `src/20010915-1.c` | exact semantic | `x` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20010915-1.c/case.log` |
| `src/20010925-1.c` | exact semantic | `foo` | `runtime/intrinsic family 'memcpy runtime family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20010925-1.c/case.log` |
| `src/20011008-3.c` | exact semantic | `__db_txnlist_lsnadd` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20011008-3.c/case.log` |
| `src/20011024-1.c` | exact semantic | `foo` | `runtime/intrinsic family 'memcpy runtime family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20011024-1.c/case.log` |
| `src/20011113-1.c` | exact semantic | `foo` | `semantic call family 'direct-call semantic family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20011113-1.c/case.log` |
| `src/20011121-1.c` | exact semantic | `main` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20011121-1.c/case.log` |
| `src/20020227-1.c` | exact semantic | `f1` | `scalar/local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20020227-1.c/case.log` |
| `src/20020321-1.c` | exact semantic | `main` | `semantic call family 'direct-call semantic family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20020321-1.c/case.log` |
| `src/20020402-2.c` | exact semantic | `setStatPointers` | `store local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20020402-2.c/case.log` |
| `src/20020404-1.c` | exact semantic | `main` | `semantic call family 'direct-call semantic family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20020404-1.c/case.log` |
| `src/20020411-1.c` | exact semantic | `foo` | `scalar/local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20020411-1.c/case.log` |
| `src/20020413-1.c` | exact semantic | `test` | `store local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20020413-1.c/case.log` |
| `src/20020503-1.c` | exact semantic | `main` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20020503-1.c/case.log` |
| `src/20021010-2.c` | related bootstrap | `(n/a)` | `bootstrap global/data shape admission note` | no; not exact semantic row | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20021010-2.c/case.log` |
| `src/20021011-1.c` | exact semantic | `main` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20021011-1.c/case.log` |
| `src/20021015-1.c` | exact semantic | `main` | `semantic call family 'direct-call semantic family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20021015-1.c/case.log` |
| `src/20021024-1.c` | exact semantic | `bar` | `store local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20021024-1.c/case.log` |
| `src/20021118-1.c` | exact semantic | `main` | `semantic call family 'direct-call semantic family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20021118-1.c/case.log` |
| `src/20021118-2.c` | exact semantic | `main` | `scalar/local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20021118-2.c/case.log` |
| `src/20021120-1.c` | related bootstrap | `(n/a)` | `bootstrap global/data shape admission note` | no; not exact semantic row | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20021120-1.c/case.log` |
| `src/20030307-1.c` | exact semantic | `main` | `semantic call family 'direct-call semantic family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20030307-1.c/case.log` |
| `src/20030408-1.c` | exact semantic | `test1` | `runtime/intrinsic family 'memcpy runtime family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20030408-1.c/case.log` |
| `src/20030613-1.c` | exact semantic | `c5p` | `semantic call family 'direct-call semantic family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20030613-1.c/case.log` |
| `src/20030715-1.c` | exact semantic | `main` | `semantic call family 'direct-call semantic family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20030715-1.c/case.log` |
| `src/20030717-1.c` | exact semantic | `bar` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20030717-1.c/case.log` |
| `src/20030828-1.c` | exact semantic | `bar` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20030828-1.c/case.log` |
| `src/20030913-1.c` | exact semantic | `test` | `store local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20030913-1.c/case.log` |
| `src/20030914-1.c` | exact semantic | `main` | `semantic call family 'direct-call semantic family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20030914-1.c/case.log` |
| `src/20030928-1.c` | exact semantic | `get_addrs` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20030928-1.c/case.log` |
| `src/20031201-1.c` | exact semantic | `f1` | `store local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20031201-1.c/case.log` |
| `src/20031214-1.c` | exact semantic | `main` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20031214-1.c/case.log` |
| `src/20031215-1.c` | exact semantic | `test1` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20031215-1.c/case.log` |
| `src/20040208-1.c` | exact semantic | `main` | `store local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20040208-1.c/case.log` |
| `src/20040302-1.c` | related bootstrap | `(n/a)` | `bootstrap global/data shape admission note` | no; not exact semantic row | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20040302-1.c/case.log` |
| `src/20040703-1.c` | exact semantic | `num_rshift` | `semantic call family 'direct-call semantic family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20040703-1.c/case.log` |
| `src/20040707-1.c` | exact semantic | `foo` | `store local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20040707-1.c/case.log` |
| `src/20040823-1.c` | exact semantic | `bla` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20040823-1.c/case.log` |
| `src/20041113-1.c` | exact semantic | `test` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20041113-1.c/case.log` |
| `src/20041124-1.c` | exact semantic | `main` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20041124-1.c/case.log` |
| `src/20041201-1.c` | exact semantic | `checkScc2` | `scalar/local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20041201-1.c/case.log` |
| `src/20041214-1.c` | related bootstrap | `(n/a)` | `bootstrap global/data shape admission note` | no; not exact semantic row | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20041214-1.c/case.log` |
| `src/20041218-1.c` | exact semantic | `baz` | `runtime/intrinsic family 'memset runtime family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20041218-1.c/case.log` |
| `src/20050121-1.c` | exact semantic | `bar_float` | `semantic call family 'call-return semantic family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20050121-1.c/case.log` |
| `src/20050218-1.c` | exact semantic | `main` | `semantic call family 'direct-call semantic family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20050218-1.c/case.log` |
| `src/20050316-3.c` | exact semantic | `test1` | `function-signature semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20050316-3.c/case.log` |
| `src/20050604-1.c` | exact semantic | `foo` | `alloca local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20050604-1.c/case.log` |
| `src/20050607-1.c` | exact semantic | `main` | `alloca local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20050607-1.c/case.log` |
| `src/20050826-1.c` | exact semantic | `foo` | `runtime/intrinsic family 'memset runtime family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20050826-1.c/case.log` |
| `src/20050826-2.c` | exact semantic | `inet_check_attr` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20050826-2.c/case.log` |
| `src/20051104-1.c` | exact semantic | `main` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20051104-1.c/case.log` |
| `src/20051113-1.c` | exact semantic | `Sum` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20051113-1.c/case.log` |
| `src/20051215-1.c` | exact semantic | `main` | `semantic call family 'direct-call semantic family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20051215-1.c/case.log` |
| `src/20060412-1.c` | exact semantic | `main` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20060412-1.c/case.log` |
| `src/20060420-1.c` | related bootstrap | `(n/a)` | `bootstrap global/data shape admission note` | no; not exact semantic row | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20060420-1.c/case.log` |
| `src/20060930-2.c` | exact semantic | `foo` | `runtime/intrinsic family 'memcpy runtime family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20060930-2.c/case.log` |
| `src/20070614-1.c` | exact semantic | `foo` | `scalar/local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20070614-1.c/case.log` |
| `src/20070824-1.c` | exact semantic | `main` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20070824-1.c/case.log` |
| `src/20071029-1.c` | exact semantic | `foo` | `function-signature semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20071029-1.c/case.log` |
| `src/20071030-1.c` | exact semantic | `main` | `runtime/intrinsic family 'memset runtime family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20071030-1.c/case.log` |
| `src/20071120-1.c` | exact semantic | `VEC_deferred_access_base_last` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20071120-1.c/case.log` |
| `src/20071202-1.c` | exact semantic | `foo` | `store local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20071202-1.c/case.log` |
| `src/20071210-1.c` | related bootstrap | `(n/a)` | `bootstrap global/data shape admission note` | no; not exact semantic row | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20071210-1.c/case.log` |
| `src/20071213-1.c` | exact semantic | `f1` | `scalar/local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20071213-1.c/case.log` |
| `src/20071219-1.c` | exact semantic | `test2` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20071219-1.c/case.log` |
| `src/20071220-1.c` | related bootstrap | `(n/a)` | `bootstrap global/data shape admission note` | no; not exact semantic row | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20071220-1.c/case.log` |
| `src/20071220-2.c` | related bootstrap | `(n/a)` | `bootstrap global/data shape admission note` | no; not exact semantic row | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20071220-2.c/case.log` |
| `src/20080117-1.c` | exact semantic | `gstate_path_memory` | `store local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20080117-1.c/case.log` |
| `src/20080424-1.c` | exact semantic | `main` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20080424-1.c/case.log` |
| `src/20080502-1.c` | exact semantic | `foo` | `scalar-control-flow semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20080502-1.c/case.log` |
| `src/20080604-1.c` | exact semantic | `baz` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20080604-1.c/case.log` |
| `src/20081103-1.c` | exact semantic | `main` | `runtime/intrinsic family 'memcpy runtime family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20081103-1.c/case.log` |
| `src/20081117-1.c` | exact semantic | `main` | `semantic call family 'direct-call semantic family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20081117-1.c/case.log` |
| `src/20081218-1.c` | exact semantic | `foo` | `runtime/intrinsic family 'memset runtime family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20081218-1.c/case.log` |
| `src/20090113-1.c` | exact semantic | `msum_i4` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20090113-1.c/case.log` |
| `src/20090623-1.c` | exact semantic | `foo` | `store local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20090623-1.c/case.log` |
| `src/20100430-1.c` | exact semantic | `foo` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20100430-1.c/case.log` |
| `src/20100708-1.c` | exact semantic | `f` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20100708-1.c/case.log` |
| `src/20101013-1.c` | exact semantic | `main` | `semantic call family 'direct-call semantic family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20101013-1.c/case.log` |
| `src/20111208-1.c` | exact semantic | `pack_unpack` | `alloca local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20111208-1.c/case.log` |
| `src/20111212-1.c` | exact semantic | `frob_entry` | `runtime/intrinsic family 'memcpy runtime family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20111212-1.c/case.log` |
| `src/20120808-1.c` | exact semantic | `main` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20120808-1.c/case.log` |
| `src/20120919-1.c` | exact semantic | `main` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20120919-1.c/case.log` |
| `src/20121108-1.c` | exact semantic | `string_to_ip` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20121108-1.c/case.log` |
| `src/20131127-1.c` | exact semantic | `fn2` | `store local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20131127-1.c/case.log` |
| `src/20141107-1.c` | exact semantic | `checkf` | `runtime/intrinsic family 'memcpy runtime family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20141107-1.c/case.log` |
| `src/20180131-1.c` | exact semantic | `main` | `semantic call family 'direct-call semantic family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20180131-1.c/case.log` |
| `src/20180921-1.c` | exact semantic | `aw` | `alloca local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20180921-1.c/case.log` |
| `src/20190820-1.c` | exact semantic | `pointer_string` | `semantic call family 'direct-call semantic family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_20190820-1.c/case.log` |
| `src/920501-4.c` | related bootstrap | `(n/a)` | `bootstrap global/data shape admission note` | no; not exact semantic row | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_920501-4.c/case.log` |
| `src/920501-5.c` | exact semantic | `x` | `store local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_920501-5.c/case.log` |
| `src/920501-8.c` | exact semantic | `va` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_920501-8.c/case.log` |
| `src/920625-1.c` | exact semantic | `main` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_920625-1.c/case.log` |
| `src/920726-1.c` | exact semantic | `first` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_920726-1.c/case.log` |
| `src/920810-1.c` | exact semantic | `f` | `runtime/intrinsic family 'memcpy runtime family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_920810-1.c/case.log` |
| `src/920922-1.c` | exact semantic | `main` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_920922-1.c/case.log` |
| `src/921013-1.c` | exact semantic | `main` | `scalar/local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_921013-1.c/case.log` |
| `src/921112-1.c` | exact semantic | `f` | `store local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_921112-1.c/case.log` |
| `src/930126-1.c` | exact semantic | `main` | `store local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_930126-1.c/case.log` |
| `src/930526-1.c` | exact semantic | `f` | `store local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_930526-1.c/case.log` |
| `src/930614-1.c` | exact semantic | `main` | `scalar-control-flow semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_930614-1.c/case.log` |
| `src/930614-2.c` | exact semantic | `main` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_930614-2.c/case.log` |
| `src/930719-1.c` | exact semantic | `f` | `store local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_930719-1.c/case.log` |
| `src/931004-1.c` | exact semantic | `main` | `semantic call family 'direct-call semantic family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_931004-1.c/case.log` |
| `src/931004-10.c` | exact semantic | `f` | `alloca local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_931004-10.c/case.log` |
| `src/931004-11.c` | exact semantic | `main` | `semantic call family 'direct-call semantic family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_931004-11.c/case.log` |
| `src/931004-12.c` | exact semantic | `f` | `alloca local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_931004-12.c/case.log` |
| `src/931004-13.c` | exact semantic | `main` | `semantic call family 'direct-call semantic family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_931004-13.c/case.log` |
| `src/931004-14.c` | exact semantic | `f` | `alloca local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_931004-14.c/case.log` |
| `src/931004-2.c` | exact semantic | `f` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_931004-2.c/case.log` |
| `src/931004-3.c` | exact semantic | `main` | `semantic call family 'direct-call semantic family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_931004-3.c/case.log` |
| `src/931004-4.c` | exact semantic | `f` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_931004-4.c/case.log` |
| `src/931004-5.c` | exact semantic | `main` | `semantic call family 'direct-call semantic family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_931004-5.c/case.log` |
| `src/931004-6.c` | exact semantic | `f` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_931004-6.c/case.log` |
| `src/931004-7.c` | exact semantic | `main` | `semantic call family 'direct-call semantic family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_931004-7.c/case.log` |
| `src/931004-8.c` | exact semantic | `f` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_931004-8.c/case.log` |
| `src/931004-9.c` | exact semantic | `main` | `semantic call family 'direct-call semantic family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_931004-9.c/case.log` |
| `src/931102-2.c` | exact semantic | `f` | `store local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_931102-2.c/case.log` |
| `src/940115-1.c` | exact semantic | `main` | `semantic call family 'direct-call semantic family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_940115-1.c/case.log` |
| `src/941021-1.c` | exact semantic | `main` | `semantic call family 'direct-call semantic family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_941021-1.c/case.log` |
| `src/950426-1.c` | exact semantic | `main` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_950426-1.c/case.log` |
| `src/960117-1.c` | exact semantic | `get_id` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_960117-1.c/case.log` |
| `src/960215-1.c` | related bootstrap | `(n/a)` | `bootstrap global/data shape admission note` | no; not exact semantic row | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_960215-1.c/case.log` |
| `src/960405-1.c` | related bootstrap | `(n/a)` | `bootstrap global/data shape admission note` | no; not exact semantic row | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_960405-1.c/case.log` |
| `src/960513-1.c` | exact semantic | `f` | `scalar-binop semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_960513-1.c/case.log` |
| `src/960521-1.c` | exact semantic | `main` | `store local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_960521-1.c/case.log` |
| `src/961223-1.c` | exact semantic | `main` | `scalar/local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_961223-1.c/case.log` |
| `src/980205.c` | exact semantic | `fdouble` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_980205.c/case.log` |
| `src/980506-3.c` | exact semantic | `build_lookup` | `runtime/intrinsic family 'memset runtime family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_980506-3.c/case.log` |
| `src/980604-1.c` | exact semantic | `main` | `scalar-control-flow semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_980604-1.c/case.log` |
| `src/980608-1.c` | exact semantic | `debug` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_980608-1.c/case.log` |
| `src/980716-1.c` | exact semantic | `stub` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_980716-1.c/case.log` |
| `src/981130-1.c` | exact semantic | `main` | `store local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_981130-1.c/case.log` |
| `src/990128-1.c` | exact semantic | `main` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_990128-1.c/case.log` |
| `src/990130-1.c` | exact semantic | `foo` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_990130-1.c/case.log` |
| `src/990208-1.c` | exact semantic | `doit` | `store local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_990208-1.c/case.log` |
| `src/990525-1.c` | exact semantic | `die` | `store local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_990525-1.c/case.log` |
| `src/990628-1.c` | exact semantic | `fetch` | `runtime/intrinsic family 'memset runtime family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_990628-1.c/case.log` |
| `src/991118-1.c` | exact semantic | `main` | `store local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_991118-1.c/case.log` |
| `src/991216-2.c` | exact semantic | `test` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_991216-2.c/case.log` |
| `src/991227-1.c` | exact semantic | `main` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_991227-1.c/case.log` |
| `src/991228-1.c` | exact semantic | `signbit` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_991228-1.c/case.log` |
| `src/alias-1.c` | exact semantic | `typepun` | `store local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_alias-1.c/case.log` |
| `src/alias-access-path-1.c` | exact semantic | `main` | `store local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_alias-access-path-1.c/case.log` |
| `src/bswap-2.c` | exact semantic | `main` | `alloca local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_bswap-2.c/case.log` |
| `src/builtin-bitops-1.c` | exact semantic | `main` | `scalar/local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_builtin-bitops-1.c/case.log` |
| `src/builtin-prefetch-4.c` | exact semantic | `assign_glob_idx` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_builtin-prefetch-4.c/case.log` |
| `src/builtin-prefetch-5.c` | exact semantic | `glob_idx` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_builtin-prefetch-5.c/case.log` |
| `src/builtin-types-compatible-p.c` | related bootstrap | `(n/a)` | `bootstrap global/data shape admission note` | no; not exact semantic row | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_builtin-types-compatible-p.c/case.log` |
| `src/cbrt.c` | exact semantic | `cbrtl` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_cbrt.c/case.log` |
| `src/comp-goto-1.c` | related bootstrap | `(n/a)` | `bootstrap global/data shape admission note` | no; not exact semantic row | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_comp-goto-1.c/case.log` |
| `src/complex-1.c` | exact semantic | `main` | `semantic call family 'call-return semantic family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_complex-1.c/case.log` |
| `src/complex-2.c` | exact semantic | `f` | `scalar/local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_complex-2.c/case.log` |
| `src/complex-5.c` | exact semantic | `p` | `scalar/local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_complex-5.c/case.log` |
| `src/complex-6.c` | exact semantic | `ctest_float` | `scalar/local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_complex-6.c/case.log` |
| `src/complex-7.c` | exact semantic | `check_float` | `scalar/local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_complex-7.c/case.log` |
| `src/compndlit-1.c` | exact semantic | `main` | `store local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_compndlit-1.c/case.log` |
| `src/enum-3.c` | exact semantic | `main` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_enum-3.c/case.log` |
| `src/ffs-1.c` | exact semantic | `main` | `scalar/local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_ffs-1.c/case.log` |
| `src/ffs-2.c` | exact semantic | `main` | `scalar/local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_ffs-2.c/case.log` |
| `src/fprintf-2.c` | exact semantic | `main` | `semantic call family 'direct-call semantic family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_fprintf-2.c/case.log` |
| `src/fprintf-chk-1.c` | exact semantic | `__fprintf_chk` | `scalar/local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_fprintf-chk-1.c/case.log` |
| `src/ieee/20010226-1.c` | related bootstrap | `(n/a)` | `bootstrap global/data shape admission note` | no; not exact semantic row | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_ieee_20010226-1.c/case.log` |
| `src/ieee/acc1.c` | exact semantic | `main` | `scalar/local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_ieee_acc1.c/case.log` |
| `src/ieee/acc2.c` | exact semantic | `main` | `scalar/local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_ieee_acc2.c/case.log` |
| `src/ieee/compare-fp-1.c` | exact semantic | `iuneq` | `scalar/local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_ieee_compare-fp-1.c/case.log` |
| `src/ieee/compare-fp-3.c` | exact semantic | `test5` | `scalar/local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_ieee_compare-fp-3.c/case.log` |
| `src/ieee/compare-fp-4.c` | exact semantic | `iuneq` | `scalar/local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_ieee_compare-fp-4.c/case.log` |
| `src/ieee/copysign2.c` | exact semantic | `testf` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_ieee_copysign2.c/case.log` |
| `src/ieee/fp-cmp-3.c` | related bootstrap | `(n/a)` | `bootstrap global/data shape admission note` | no; not exact semantic row | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_ieee_fp-cmp-3.c/case.log` |
| `src/ieee/fp-cmp-4.c` | exact semantic | `test_isunordered` | `scalar/local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_ieee_fp-cmp-4.c/case.log` |
| `src/ieee/fp-cmp-4f.c` | exact semantic | `test_isunordered` | `scalar/local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_ieee_fp-cmp-4f.c/case.log` |
| `src/ieee/fp-cmp-4l.c` | exact semantic | `test_isunordered` | `scalar/local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_ieee_fp-cmp-4l.c/case.log` |
| `src/ieee/fp-cmp-5.c` | exact semantic | `test_isunordered` | `scalar/local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_ieee_fp-cmp-5.c/case.log` |
| `src/ieee/fp-cmp-8.c` | exact semantic | `test_isunordered` | `scalar-control-flow semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_ieee_fp-cmp-8.c/case.log` |
| `src/ieee/fp-cmp-8f.c` | exact semantic | `test_isunordered` | `scalar-control-flow semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_ieee_fp-cmp-8f.c/case.log` |
| `src/ieee/fp-cmp-8l.c` | exact semantic | `test_isunordered` | `scalar-control-flow semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_ieee_fp-cmp-8l.c/case.log` |
| `src/ieee/inf-1.c` | exact semantic | `main` | `store local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_ieee_inf-1.c/case.log` |
| `src/ieee/inf-2.c` | exact semantic | `testl` | `scalar/local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_ieee_inf-2.c/case.log` |
| `src/ieee/inf-3.c` | exact semantic | `testl` | `scalar/local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_ieee_inf-3.c/case.log` |
| `src/ieee/minuszero.c` | exact semantic | `main` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_ieee_minuszero.c/case.log` |
| `src/ieee/pr30704.c` | exact semantic | `f1` | `runtime/intrinsic family 'memcpy runtime family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_ieee_pr30704.c/case.log` |
| `src/ieee/pr36332.c` | exact semantic | `foo` | `scalar/local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_ieee_pr36332.c/case.log` |
| `src/ieee/pr38016.c` | exact semantic | `test_isunordered` | `scalar-control-flow semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_ieee_pr38016.c/case.log` |
| `src/ieee/pr50310.c` | related bootstrap | `(n/a)` | `bootstrap global/data shape admission note` | no; not exact semantic row | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_ieee_pr50310.c/case.log` |
| `src/ieee/pr72824-2.c` | exact semantic | `foo` | `function-signature semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_ieee_pr72824-2.c/case.log` |
| `src/ipa-sra-1.c` | exact semantic | `main` | `semantic call family 'direct-call semantic family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_ipa-sra-1.c/case.log` |
| `src/longlong.c` | exact semantic | `alpha_ep_extbl_i_eq_0` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_longlong.c/case.log` |
| `src/loop-12.c` | exact semantic | `is_end_of_statement` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_loop-12.c/case.log` |
| `src/lto-tbaa-1.c` | exact semantic | `main` | `store local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_lto-tbaa-1.c/case.log` |
| `src/mayalias-2.c` | exact semantic | `f` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_mayalias-2.c/case.log` |
| `src/mayalias-3.c` | exact semantic | `f` | `store local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_mayalias-3.c/case.log` |
| `src/memchr-1.c` | related bootstrap | `(n/a)` | `bootstrap global/data shape admission note` | no; not exact semantic row | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_memchr-1.c/case.log` |
| `src/memcpy-1.c` | exact semantic | `copy` | `runtime/intrinsic family 'memcpy runtime family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_memcpy-1.c/case.log` |
| `src/memcpy-2.c` | exact semantic | `main` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_memcpy-2.c/case.log` |
| `src/memcpy-bi.c` | exact semantic | `main` | `runtime/intrinsic family 'memset runtime family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_memcpy-bi.c/case.log` |
| `src/memset-1.c` | exact semantic | `main` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_memset-1.c/case.log` |
| `src/memset-2.c` | exact semantic | `reset` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_memset-2.c/case.log` |
| `src/memset-3.c` | exact semantic | `reset` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_memset-3.c/case.log` |
| `src/memset-4.c` | exact semantic | `f` | `runtime/intrinsic family 'memset runtime family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_memset-4.c/case.log` |
| `src/multi-ix.c` | exact semantic | `s` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_multi-ix.c/case.log` |
| `src/postmod-1.c` | related bootstrap | `(n/a)` | `bootstrap global/data shape admission note` | no; not exact semantic row | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_postmod-1.c/case.log` |
| `src/pr15262-1.c` | exact semantic | `main` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr15262-1.c/case.log` |
| `src/pr15262-2.c` | exact semantic | `foo` | `store local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr15262-2.c/case.log` |
| `src/pr15262.c` | exact semantic | `foo` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr15262.c/case.log` |
| `src/pr15296.c` | exact semantic | `f` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr15296.c/case.log` |
| `src/pr19687.c` | exact semantic | `main` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr19687.c/case.log` |
| `src/pr20601-1.c` | exact semantic | `setup1` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr20601-1.c/case.log` |
| `src/pr20621-1.c` | exact semantic | `main` | `semantic call family 'direct-call semantic family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr20621-1.c/case.log` |
| `src/pr22098-1.c` | exact semantic | `main` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr22098-1.c/case.log` |
| `src/pr22098-2.c` | exact semantic | `main` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr22098-2.c/case.log` |
| `src/pr22098-3.c` | exact semantic | `main` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr22098-3.c/case.log` |
| `src/pr22141-1.c` | exact semantic | `c1` | `runtime/intrinsic family 'memset runtime family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr22141-1.c/case.log` |
| `src/pr22141-2.c` | exact semantic | `c1` | `runtime/intrinsic family 'memset runtime family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr22141-2.c/case.log` |
| `src/pr22630.c` | exact semantic | `main` | `semantic call family 'direct-call semantic family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr22630.c/case.log` |
| `src/pr23324.c` | related bootstrap | `(n/a)` | `bootstrap global/data shape admission note` | no; not exact semantic row | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr23324.c/case.log` |
| `src/pr24851.c` | exact semantic | `main` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr24851.c/case.log` |
| `src/pr27260.c` | exact semantic | `foo` | `runtime/intrinsic family 'memset runtime family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr27260.c/case.log` |
| `src/pr28778.c` | exact semantic | `main` | `semantic call family 'direct-call semantic family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr28778.c/case.log` |
| `src/pr28982a.c` | related bootstrap | `(n/a)` | `bootstrap global/data shape admission note` | no; not exact semantic row | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr28982a.c/case.log` |
| `src/pr28982b.c` | related bootstrap | `(n/a)` | `bootstrap global/data shape admission note` | no; not exact semantic row | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr28982b.c/case.log` |
| `src/pr30778.c` | exact semantic | `init_reg_last` | `runtime/intrinsic family 'memset runtime family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr30778.c/case.log` |
| `src/pr33870-1.c` | exact semantic | `merge_pagelist` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr33870-1.c/case.log` |
| `src/pr33870.c` | exact semantic | `sort_pagelist` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr33870.c/case.log` |
| `src/pr35456.c` | exact semantic | `not_fabs` | `scalar-control-flow semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr35456.c/case.log` |
| `src/pr35472.c` | exact semantic | `test` | `store local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr35472.c/case.log` |
| `src/pr36034-2.c` | related bootstrap | `(n/a)` | `bootstrap global/data shape admission note` | no; not exact semantic row | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr36034-2.c/case.log` |
| `src/pr36038.c` | exact semantic | `main` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr36038.c/case.log` |
| `src/pr36343.c` | exact semantic | `bar` | `store local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr36343.c/case.log` |
| `src/pr36765.c` | exact semantic | `foo` | `store local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr36765.c/case.log` |
| `src/pr38048-1.c` | exact semantic | `foo` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr38048-1.c/case.log` |
| `src/pr38048-2.c` | exact semantic | `inv_J` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr38048-2.c/case.log` |
| `src/pr38051.c` | exact semantic | `main` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr38051.c/case.log` |
| `src/pr38151.c` | exact semantic | `check2848va` | `alloca local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr38151.c/case.log` |
| `src/pr38236.c` | exact semantic | `main` | `semantic call family 'direct-call semantic family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr38236.c/case.log` |
| `src/pr38969.c` | exact semantic | `bar` | `semantic call family 'call-return semantic family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr38969.c/case.log` |
| `src/pr39120.c` | exact semantic | `bar` | `store local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr39120.c/case.log` |
| `src/pr39339.c` | exact semantic | `foo` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr39339.c/case.log` |
| `src/pr39501.c` | exact semantic | `float_min1` | `scalar-control-flow semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr39501.c/case.log` |
| `src/pr40022.c` | exact semantic | `main` | `semantic call family 'direct-call semantic family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr40022.c/case.log` |
| `src/pr40668.c` | exact semantic | `foo` | `runtime/intrinsic family 'memcpy runtime family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr40668.c/case.log` |
| `src/pr41239.c` | exact semantic | `test` | `semantic call family 'direct-call semantic family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr41239.c/case.log` |
| `src/pr41395-1.c` | exact semantic | `foo` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr41395-1.c/case.log` |
| `src/pr42248.c` | exact semantic | `check` | `scalar/local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr42248.c/case.log` |
| `src/pr42570.c` | related bootstrap | `(n/a)` | `bootstrap global/data shape admission note` | no; not exact semantic row | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr42570.c/case.log` |
| `src/pr42691.c` | exact semantic | `main` | `scalar/local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr42691.c/case.log` |
| `src/pr42833.c` | exact semantic | `helper_neon_rshl_s8` | `alloca local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr42833.c/case.log` |
| `src/pr43784.c` | exact semantic | `rp` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr43784.c/case.log` |
| `src/pr43987.c` | exact semantic | `add_input_file` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr43987.c/case.log` |
| `src/pr44164.c` | exact semantic | `foo` | `store local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr44164.c/case.log` |
| `src/pr44468.c` | exact semantic | `test1` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr44468.c/case.log` |
| `src/pr44555.c` | exact semantic | `main` | `semantic call family 'direct-call semantic family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr44555.c/case.log` |
| `src/pr44575.c` | exact semantic | `main` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr44575.c/case.log` |
| `src/pr44942.c` | exact semantic | `test1` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr44942.c/case.log` |
| `src/pr46309.c` | exact semantic | `bar` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr46309.c/case.log` |
| `src/pr47538.c` | exact semantic | `main` | `scalar/local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr47538.c/case.log` |
| `src/pr48571-1.c` | exact semantic | `bar` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr48571-1.c/case.log` |
| `src/pr49390.c` | exact semantic | `test` | `semantic call family 'direct-call semantic family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr49390.c/case.log` |
| `src/pr49419.c` | exact semantic | `foo` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr49419.c/case.log` |
| `src/pr51323.c` | exact semantic | `main` | `semantic call family 'direct-call semantic family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr51323.c/case.log` |
| `src/pr51877.c` | exact semantic | `bar` | `runtime/intrinsic family 'memset runtime family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr51877.c/case.log` |
| `src/pr52129.c` | exact semantic | `main` | `semantic call family 'direct-call semantic family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr52129.c/case.log` |
| `src/pr52760.c` | exact semantic | `foo` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr52760.c/case.log` |
| `src/pr52979-1.c` | exact semantic | `bar` | `store local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr52979-1.c/case.log` |
| `src/pr52979-2.c` | exact semantic | `bar` | `store local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr52979-2.c/case.log` |
| `src/pr53645-2.c` | related bootstrap | `(n/a)` | `bootstrap global/data shape admission note` | no; not exact semantic row | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr53645-2.c/case.log` |
| `src/pr53645.c` | related bootstrap | `(n/a)` | `bootstrap global/data shape admission note` | no; not exact semantic row | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr53645.c/case.log` |
| `src/pr53688.c` | exact semantic | `init` | `runtime/intrinsic family 'memcpy runtime family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr53688.c/case.log` |
| `src/pr56205.c` | exact semantic | `f1` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr56205.c/case.log` |
| `src/pr56837.c` | exact semantic | `foo` | `scalar/local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr56837.c/case.log` |
| `src/pr57321.c` | exact semantic | `foo` | `store local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr57321.c/case.log` |
| `src/pr57344-1.c` | exact semantic | `main` | `store local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr57344-1.c/case.log` |
| `src/pr57344-2.c` | exact semantic | `main` | `store local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr57344-2.c/case.log` |
| `src/pr57344-3.c` | exact semantic | `main` | `store local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr57344-3.c/case.log` |
| `src/pr57344-4.c` | exact semantic | `main` | `store local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr57344-4.c/case.log` |
| `src/pr58209.c` | exact semantic | `main` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr58209.c/case.log` |
| `src/pr58277-1.c` | exact semantic | `foo` | `store local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr58277-1.c/case.log` |
| `src/pr58277-2.c` | exact semantic | `fn2` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr58277-2.c/case.log` |
| `src/pr58365.c` | exact semantic | `main` | `store local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr58365.c/case.log` |
| `src/pr58419.c` | exact semantic | `bar` | `store local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr58419.c/case.log` |
| `src/pr59229.c` | exact semantic | `bar` | `runtime/intrinsic family 'memset runtime family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr59229.c/case.log` |
| `src/pr60072.c` | exact semantic | `main` | `store local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr60072.c/case.log` |
| `src/pr60960.c` | exact semantic | `f1` | `function-signature semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr60960.c/case.log` |
| `src/pr61725.c` | exact semantic | `main` | `scalar/local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr61725.c/case.log` |
| `src/pr63843.c` | exact semantic | `bar` | `runtime/intrinsic family 'memcpy runtime family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr63843.c/case.log` |
| `src/pr64006.c` | exact semantic | `test` | `scalar/local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr64006.c/case.log` |
| `src/pr64979.c` | exact semantic | `foo` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr64979.c/case.log` |
| `src/pr65369.c` | exact semantic | `bar` | `runtime/intrinsic family 'memcpy runtime family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr65369.c/case.log` |
| `src/pr65401.c` | exact semantic | `main` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr65401.c/case.log` |
| `src/pr65427.c` | related bootstrap | `(n/a)` | `bootstrap global/data shape admission note` | no; not exact semantic row | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr65427.c/case.log` |
| `src/pr65956.c` | exact semantic | `bar` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr65956.c/case.log` |
| `src/pr67226.c` | exact semantic | `t0` | `semantic call family 'direct-call semantic family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr67226.c/case.log` |
| `src/pr68381.c` | exact semantic | `foo` | `scalar/local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr68381.c/case.log` |
| `src/pr69320-2.c` | exact semantic | `fn1` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr69320-2.c/case.log` |
| `src/pr69691.c` | exact semantic | `bar` | `store local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr69691.c/case.log` |
| `src/pr70127.c` | exact semantic | `main` | `store local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr70127.c/case.log` |
| `src/pr70460.c` | related bootstrap | `(n/a)` | `bootstrap global/data shape admission note` | no; not exact semantic row | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr70460.c/case.log` |
| `src/pr70566.c` | exact semantic | `set_f2` | `semantic call family 'direct-call semantic family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr70566.c/case.log` |
| `src/pr70903.c` | exact semantic | `foo` | `function-signature semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr70903.c/case.log` |
| `src/pr71083.c` | exact semantic | `foo` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr71083.c/case.log` |
| `src/pr71550.c` | exact semantic | `main` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr71550.c/case.log` |
| `src/pr71554.c` | exact semantic | `foo` | `scalar/local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr71554.c/case.log` |
| `src/pr71626-1.c` | exact semantic | `foo` | `function-signature semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr71626-1.c/case.log` |
| `src/pr71626-2.c` | exact semantic | `foo` | `function-signature semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr71626-2.c/case.log` |
| `src/pr71700.c` | exact semantic | `main` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr71700.c/case.log` |
| `src/pr77718.c` | exact semantic | `main` | `runtime/intrinsic family 'memset runtime family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr77718.c/case.log` |
| `src/pr78170.c` | exact semantic | `fn1` | `store local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr78170.c/case.log` |
| `src/pr79043.c` | exact semantic | `main` | `store local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr79043.c/case.log` |
| `src/pr79286.c` | related bootstrap | `(n/a)` | `bootstrap global/data shape admission note` | no; not exact semantic row | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr79286.c/case.log` |
| `src/pr79354.c` | exact semantic | `foo` | `runtime/intrinsic family 'memcpy runtime family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr79354.c/case.log` |
| `src/pr79737-1.c` | exact semantic | `foo` | `store local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr79737-1.c/case.log` |
| `src/pr80153.c` | exact semantic | `main` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr80153.c/case.log` |
| `src/pr80421.c` | exact semantic | `bar` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr80421.c/case.log` |
| `src/pr82388.c` | exact semantic | `main` | `store local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr82388.c/case.log` |
| `src/pr82524.c` | exact semantic | `main` | `alloca local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr82524.c/case.log` |
| `src/pr84169.c` | exact semantic | `foo` | `scalar/local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr84169.c/case.log` |
| `src/pr84478.c` | exact semantic | `loadpoolstrings` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr84478.c/case.log` |
| `src/pr84748.c` | related bootstrap | `(n/a)` | `bootstrap global/data shape admission note` | no; not exact semantic row | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr84748.c/case.log` |
| `src/pr85095.c` | exact semantic | `f1` | `scalar/local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr85095.c/case.log` |
| `src/pr85169.c` | exact semantic | `foo` | `alloca local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr85169.c/case.log` |
| `src/pr85756.c` | exact semantic | `foo` | `store local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr85756.c/case.log` |
| `src/pr86231.c` | exact semantic | `main` | `semantic call family 'direct-call semantic family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr86231.c/case.log` |
| `src/pr86528.c` | exact semantic | `test` | `runtime/intrinsic family 'memcpy runtime family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr86528.c/case.log` |
| `src/pr86714.c` | related bootstrap | `(n/a)` | `bootstrap global/data shape admission note` | no; not exact semantic row | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr86714.c/case.log` |
| `src/pr86844.c` | exact semantic | `main` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr86844.c/case.log` |
| `src/pr88714.c` | exact semantic | `foo` | `store local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr88714.c/case.log` |
| `src/pr89434.c` | exact semantic | `foo` | `scalar/local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr89434.c/case.log` |
| `src/pr89634.c` | exact semantic | `bar` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr89634.c/case.log` |
| `src/pr90025.c` | exact semantic | `bar` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pr90025.c/case.log` |
| `src/printf-2.c` | exact semantic | `main` | `semantic call family 'direct-call semantic family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_printf-2.c/case.log` |
| `src/printf-chk-1.c` | exact semantic | `__printf_chk` | `scalar/local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_printf-chk-1.c/case.log` |
| `src/pta-field-1.c` | exact semantic | `bar` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pta-field-1.c/case.log` |
| `src/pta-field-2.c` | exact semantic | `bar` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_pta-field-2.c/case.log` |
| `src/regstack-1.c` | related bootstrap | `(n/a)` | `bootstrap global/data shape admission note` | no; not exact semantic row | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_regstack-1.c/case.log` |
| `src/restrict-1.c` | exact semantic | `bar` | `store local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_restrict-1.c/case.log` |
| `src/scal-to-vec1.c` | exact semantic | `main` | `alloca local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_scal-to-vec1.c/case.log` |
| `src/scal-to-vec2.c` | exact semantic | `main` | `alloca local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_scal-to-vec2.c/case.log` |
| `src/simd-1.c` | related bootstrap | `(n/a)` | `bootstrap global/data shape admission note` | no; not exact semantic row | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_simd-1.c/case.log` |
| `src/simd-2.c` | related bootstrap | `(n/a)` | `bootstrap global/data shape admission note` | no; not exact semantic row | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_simd-2.c/case.log` |
| `src/simd-5.c` | related bootstrap | `(n/a)` | `bootstrap global/data shape admission note` | no; not exact semantic row | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_simd-5.c/case.log` |
| `src/simd-6.c` | exact semantic | `foo` | `function-signature semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_simd-6.c/case.log` |
| `src/stdarg-1.c` | related bootstrap | `(n/a)` | `bootstrap global/data shape admission note` | no; not exact semantic row | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_stdarg-1.c/case.log` |
| `src/stdarg-2.c` | related bootstrap | `(n/a)` | `bootstrap global/data shape admission note` | no; not exact semantic row | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_stdarg-2.c/case.log` |
| `src/stdarg-3.c` | related bootstrap | `(n/a)` | `bootstrap global/data shape admission note` | no; not exact semantic row | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_stdarg-3.c/case.log` |
| `src/stdarg-4.c` | exact semantic | `f1` | `scalar/local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_stdarg-4.c/case.log` |
| `src/strcpy-1.c` | exact semantic | `main` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_strcpy-1.c/case.log` |
| `src/strct-pack-2.c` | exact semantic | `main` | `store local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_strct-pack-2.c/case.log` |
| `src/strct-stdarg-1.c` | exact semantic | `f` | `alloca local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_strct-stdarg-1.c/case.log` |
| `src/strct-varg-1.c` | exact semantic | `f` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_strct-varg-1.c/case.log` |
| `src/string-opt-17.c` | exact semantic | `test1` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_string-opt-17.c/case.log` |
| `src/string-opt-18.c` | exact semantic | `test1` | `runtime/intrinsic family 'memcpy runtime family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_string-opt-18.c/case.log` |
| `src/string-opt-5.c` | exact semantic | `main` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_string-opt-5.c/case.log` |
| `src/strlen-1.c` | exact semantic | `main` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_strlen-1.c/case.log` |
| `src/strlen-2.c` | related bootstrap | `(n/a)` | `bootstrap global/data shape admission note` | no; not exact semantic row | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_strlen-2.c/case.log` |
| `src/strlen-3.c` | related bootstrap | `(n/a)` | `bootstrap global/data shape admission note` | no; not exact semantic row | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_strlen-3.c/case.log` |
| `src/strlen-4.c` | related bootstrap | `(n/a)` | `bootstrap global/data shape admission note` | no; not exact semantic row | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_strlen-4.c/case.log` |
| `src/strlen-5.c` | exact semantic | `test_const_global_arrays` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_strlen-5.c/case.log` |
| `src/strlen-7.c` | exact semantic | `test_dynamic_type` | `runtime/intrinsic family 'memcpy runtime family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_strlen-7.c/case.log` |
| `src/struct-aliasing-1.c` | exact semantic | `foo` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_struct-aliasing-1.c/case.log` |
| `src/struct-cpy-1.c` | exact semantic | `ini` | `store local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_struct-cpy-1.c/case.log` |
| `src/struct-ret-1.c` | exact semantic | `main` | `semantic call family 'direct-call semantic family'` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_struct-ret-1.c/case.log` |
| `src/user-printf.c` | exact semantic | `user_print` | `scalar/local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_user-printf.c/case.log` |
| `src/va-arg-1.c` | exact semantic | `f` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_va-arg-1.c/case.log` |
| `src/va-arg-10.c` | exact semantic | `fap` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_va-arg-10.c/case.log` |
| `src/va-arg-11.c` | exact semantic | `foo` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_va-arg-11.c/case.log` |
| `src/va-arg-12.c` | exact semantic | `f` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_va-arg-12.c/case.log` |
| `src/va-arg-14.c` | related bootstrap | `(n/a)` | `bootstrap global/data shape admission note` | no; not exact semantic row | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_va-arg-14.c/case.log` |
| `src/va-arg-15.c` | exact semantic | `vafunction` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_va-arg-15.c/case.log` |
| `src/va-arg-16.c` | exact semantic | `vafunction` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_va-arg-16.c/case.log` |
| `src/va-arg-17.c` | exact semantic | `vafunction` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_va-arg-17.c/case.log` |
| `src/va-arg-18.c` | exact semantic | `f` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_va-arg-18.c/case.log` |
| `src/va-arg-19.c` | exact semantic | `vafunction` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_va-arg-19.c/case.log` |
| `src/va-arg-2.c` | exact semantic | `f0` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_va-arg-2.c/case.log` |
| `src/va-arg-20.c` | exact semantic | `bar` | `scalar/local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_va-arg-20.c/case.log` |
| `src/va-arg-22.c` | exact semantic | `foo` | `gep local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_va-arg-22.c/case.log` |
| `src/va-arg-23.c` | exact semantic | `foo` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_va-arg-23.c/case.log` |
| `src/va-arg-24.c` | exact semantic | `varargs0` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_va-arg-24.c/case.log` |
| `src/va-arg-26.c` | exact semantic | `f` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_va-arg-26.c/case.log` |
| `src/va-arg-4.c` | exact semantic | `f` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_va-arg-4.c/case.log` |
| `src/va-arg-5.c` | exact semantic | `va_double` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_va-arg-5.c/case.log` |
| `src/va-arg-6.c` | exact semantic | `f` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_va-arg-6.c/case.log` |
| `src/va-arg-7.c` | exact semantic | `debug` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_va-arg-7.c/case.log` |
| `src/va-arg-8.c` | exact semantic | `debug` | `load local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_va-arg-8.c/case.log` |
| `src/va-arg-9.c` | exact semantic | `f0` | `scalar/local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_va-arg-9.c/case.log` |
| `src/va-arg-trap-1.c` | related bootstrap | `(n/a)` | `bootstrap global/data shape admission note` | no; not exact semantic row | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_va-arg-trap-1.c/case.log` |
| `src/vfprintf-1.c` | exact semantic | `inner` | `scalar/local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_vfprintf-1.c/case.log` |
| `src/vfprintf-chk-1.c` | exact semantic | `inner` | `scalar/local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_vfprintf-chk-1.c/case.log` |
| `src/vprintf-1.c` | exact semantic | `inner` | `scalar/local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_vprintf-1.c/case.log` |
| `src/vprintf-chk-1.c` | exact semantic | `inner` | `scalar/local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_vprintf-chk-1.c/case.log` |
| `src/widechar-3.c` | related bootstrap | `(n/a)` | `bootstrap global/data shape admission note` | no; not exact semantic row | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_widechar-3.c/case.log` |
| `src/zero-struct-1.c` | exact semantic | `h` | `alloca local-memory semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_zero-struct-1.c/case.log` |
| `src/zero-struct-2.c` | exact semantic | `one_raw_spinlock` | `function-signature semantic family` | yes | `/workspaces/c4c/build/rv64_gcc_c_torture_backend/src_zero-struct-2.c/case.log` |
