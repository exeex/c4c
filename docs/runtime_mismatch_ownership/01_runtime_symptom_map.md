# Runtime Symptom Map

Source idea: `ideas/open/618_runtime_mismatch_ownership_investigation.md`

This file records the Step 2 symptom inventory only. It groups the accepted
current RV64 gcc torture backend runtime rows by observed harness outcome and
identifies which families are stable enough for Step 3 owner mapping.

## Evidence Baseline

The accepted full scan was refreshed with:

```sh
BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh > /tmp/c4c_618_full_scan.log 2>&1
```

The scan log ends with `total=1467 passed=473 failed=994`. The current summary
artifacts are:

- `build/agent_state/rv64_gcc_c_torture_backend_summary.tsv`: `1468` lines
  including the header, mtime `2026-07-09 01:31:56 +0000`.
- `build/agent_state/rv64_gcc_c_torture_backend_failed.txt`: `994` rows, mtime
  `2026-07-09 01:31:56 +0000`.
- Per-case logs: `build/rv64_gcc_c_torture_backend/<case-id>/case.log`.

Evidence commands used for the symptom inventory:

```sh
rg -n "\[RV64_BACKEND_RUNTIME_MISMATCH\]|clang_exit=0 c4c_exit=" build/rv64_gcc_c_torture_backend/*/case.log
rg -n "\[RV64_C4C_RUN_TIMEOUT\]" build/rv64_gcc_c_torture_backend/*/case.log
rg -n "c4c_exit=0|wrong|actual|expected|diff" build/rv64_gcc_c_torture_backend/*/case.log
rg -n "\[RV64_C4C_OBJ_COMPILE_FAIL\]|\[RV64_C4C_OBJ_COMPILE_TIMEOUT\]|\[RV64_C4C_LINK_FAIL\]" build/rv64_gcc_c_torture_backend/*/case.log
```

Current runtime symptom total: `217` rows. That is `212`
`[RV64_BACKEND_RUNTIME_MISMATCH]` rows plus `5`
`[RV64_C4C_RUN_TIMEOUT]` rows.

## Abort Or Assertion

Count: `110` rows.

This group contains `109` rows where the harness reports
`clang_exit=0 c4c_exit=Subprocess aborted`, plus one dynamic-loader assertion
row where `src/990106-1.c` reports `clang_exit=0 c4c_exit=127`.

Representative evidence:

- `build/rv64_gcc_c_torture_backend/src_pr38533.c/case.log`:
  `[RV64_BACKEND_RUNTIME_MISMATCH]` with `clang_exit=0
  c4c_exit=Subprocess aborted`.
- `build/rv64_gcc_c_torture_backend/src_20020510-1.c/case.log`:
  `[RV64_BACKEND_RUNTIME_MISMATCH]` with `clang_exit=0
  c4c_exit=Subprocess aborted`.
- `build/rv64_gcc_c_torture_backend/src_990106-1.c/case.log`:
  `[RV64_BACKEND_RUNTIME_MISMATCH]` with `clang_exit=0 c4c_exit=127` and
  `ld.so` reporting `_dl_fixup` assertion failure for a relocation that is not
  `ELF_MACHINE_JMP_SLOT`.

Rows:

`src/20020206-2.c`, `src/20030408-1.c`, `src/pr38533.c`,
`src/pr63641.c`, `src/pr79737-2.c`, `src/20000227-1.c`,
`src/20000412-3.c`, `src/20000622-1.c`, `src/20000707-1.c`,
`src/20000715-1.c`, `src/20000717-1.c`, `src/20000717-5.c`,
`src/20010518-2.c`, `src/20010520-1.c`, `src/20011019-1.c`,
`src/20020107-1.c`, `src/20020108-1.c`, `src/20020510-1.c`,
`src/20020611-1.c`, `src/20020615-1.c`, `src/20021118-1.c`,
`src/20021118-3.c`, `src/20021120-2.c`, `src/20030128-1.c`,
`src/20030914-2.c`, `src/20040309-1.c`, `src/20040311-1.c`,
`src/20040319-1.c`, `src/20050104-1.c`, `src/20050215-1.c`,
`src/20071216-1.c`, `src/20081117-1.c`, `src/20100209-1.c`,
`src/20100805-1.c`, `src/20180131-1.c`, `src/900409-1.c`,
`src/920612-1.c`, `src/920731-1.c`, `src/920908-2.c`,
`src/921112-1.c`, `src/921123-2.c`, `src/921204-1.c`,
`src/930208-1.c`, `src/930713-1.c`, `src/931005-1.c`,
`src/931012-1.c`, `src/931017-1.c`, `src/931110-2.c`,
`src/941015-1.c`, `src/950704-1.c`, `src/950710-1.c`,
`src/951003-1.c`, `src/961026-1.c`, `src/980505-1.c`,
`src/980602-2.c`, `src/980604-1.c`, `src/981206-1.c`,
`src/990106-1.c`, `src/990222-1.c`, `src/990324-1.c`,
`src/bitfld-4.c`, `src/divconst-3.c`, `src/enum-1.c`,
`src/enum-3.c`, `src/ieee/980619-1.c`, `src/ieee/pr67218.c`,
`src/nestfunc-4.c`, `src/pending-4.c`, `src/pr22429.c`,
`src/pr23941.c`, `src/pr28651.c`, `src/pr33779-1.c`,
`src/pr37102.c`, `src/pr37882.c`, `src/pr39233.c`,
`src/pr40747.c`, `src/pr41463.c`, `src/pr42721.c`,
`src/pr48809.c`, `src/pr49161.c`, `src/pr49281.c`,
`src/pr55137.c`, `src/pr57829.c`, `src/pr59014.c`,
`src/pr61306-2.c`, `src/pr61517.c`, `src/pr61682.c`,
`src/pr63302.c`, `src/pr63659.c`, `src/pr64957.c`,
`src/pr65418-1.c`, `src/pr65418-2.c`, `src/pr67226.c`,
`src/pr68376-1.c`, `src/pr69097-2.c`, `src/pr70222-2.c`,
`src/pr77767.c`, `src/pr78559.c`, `src/pr78720.c`,
`src/pr80501.c`, `src/pr83298.c`, `src/pr84339.c`,
`src/pr89195.c`, `src/pr89826.c`, `src/restrict-1.c`,
`src/struct-ini-1.c`, `src/vprintf-1.c`, `src/vrp-5.c`,
`src/wchar_t-1.c`, `src/widechar-2.c`.

Stability for Step 3: stable enough for owner mapping as a symptom family, but
`src/990106-1.c` should remain an explicit dynamic-linker assertion subcase.

## Segfault

Count: `102` rows.

This group contains runtime mismatch rows where clang exits cleanly and the c4c
binary exits with `Segmentation fault`.

Representative evidence:

- `build/rv64_gcc_c_torture_backend/src_20000412-2.c/case.log`:
  `[RV64_BACKEND_RUNTIME_MISMATCH]` with `clang_exit=0
  c4c_exit=Segmentation fault`.
- `build/rv64_gcc_c_torture_backend/src_20000706-5.c/case.log`:
  `[RV64_BACKEND_RUNTIME_MISMATCH]` with `clang_exit=0
  c4c_exit=Segmentation fault`.
- `build/rv64_gcc_c_torture_backend/src_strct-pack-1.c/case.log`:
  `[RV64_BACKEND_RUNTIME_MISMATCH]` with `clang_exit=0
  c4c_exit=Segmentation fault`.

Rows:

`src/pr30778.c`, `src/pr35472.c`, `src/pr56982.c`, `src/strlen-7.c`,
`src/20000412-2.c`, `src/20000519-1.c`, `src/20000603-1.c`,
`src/20000605-2.c`, `src/20000703-1.c`, `src/20000706-1.c`,
`src/20000706-2.c`, `src/20000706-3.c`, `src/20000706-4.c`,
`src/20000706-5.c`, `src/20000717-3.c`, `src/20000910-1.c`,
`src/20001017-2.c`, `src/20001024-1.c`, `src/20010118-1.c`,
`src/20010224-1.c`, `src/20010403-1.c`, `src/20011126-2.c`,
`src/20011219-1.c`, `src/20020402-2.c`, `src/20021011-1.c`,
`src/20030218-1.c`, `src/20030606-1.c`, `src/20030828-2.c`,
`src/20031012-1.c`, `src/20031201-1.c`, `src/20040218-1.c`,
`src/20040823-1.c`, `src/20050107-1.c`, `src/20050613-1.c`,
`src/20070212-1.c`, `src/20070212-3.c`, `src/20071011-1.c`,
`src/20071018-1.c`, `src/20071202-1.c`, `src/20071213-1.c`,
`src/20080506-2.c`, `src/20080522-1.c`, `src/20090527-1.c`,
`src/20090623-1.c`, `src/20120105-1.c`, `src/20120427-1.c`,
`src/20120427-2.c`, `src/20140425-1.c`, `src/20170401-2.c`,
`src/20180226-1.c`, `src/920501-2.c`, `src/920506-1.c`,
`src/920520-1.c`, `src/921123-1.c`, `src/930518-1.c`,
`src/930603-3.c`, `src/930718-1.c`, `src/930725-1.c`,
`src/950322-1.c`, `src/950809-1.c`, `src/951204-1.c`,
`src/961213-1.c`, `src/980506-2.c`, `src/980617-1.c`,
`src/980701-1.c`, `src/981001-1.c`, `src/bf-pack-1.c`,
`src/divconst-1.c`, `src/ieee/mzero2.c`, `src/loop-12.c`,
`src/loop-13.c`, `src/mayalias-3.c`, `src/pr17078-1.c`,
`src/pr20466-1.c`, `src/pr20527-1.c`, `src/pr29006.c`,
`src/pr29156.c`, `src/pr31169.c`, `src/pr31448-2.c`,
`src/pr31448.c`, `src/pr33992.c`, `src/pr36339.c`,
`src/pr41317.c`, `src/pr43008.c`, `src/pr44202-1.c`,
`src/pr51323.c`, `src/pr57124.c`, `src/pr57130.c`,
`src/pr57321.c`, `src/pr61673.c`, `src/pr68143_1.c`,
`src/pr70566.c`, `src/pr78586.c`, `src/pr79327.c`,
`src/pr87623.c`, `src/pr88693.c`, `src/strcmp-1.c`,
`src/strct-pack-1.c`, `src/string-opt-17.c`, `src/strlen-1.c`,
`src/strncmp-1.c`, `src/va-arg-20.c`.

Stability for Step 3: stable enough for owner mapping as a symptom family.

## Wrong Output

Count: `0` rows.

No refreshed runtime-mismatch row had `clang_exit=0 c4c_exit=0`, and the
wrong-output search did not find a current row with output diff evidence. This
means Step 3 should not create a wrong-output owner family from the accepted
Step 1 baseline.

Stability for Step 3: stable empty family.

## Timeout

Count: `5` rows.

This group contains rows with `[RV64_C4C_RUN_TIMEOUT]`; each exceeded the
current 20 second harness limit.

Representative evidence:

- `build/rv64_gcc_c_torture_backend/src_20000224-1.c/case.log`:
  `[RV64_C4C_RUN_TIMEOUT]` and `src/20000224-1.c exceeded 20s`.
- `build/rv64_gcc_c_torture_backend/src_pr85582-1.c/case.log`:
  `[RV64_C4C_RUN_TIMEOUT]` and `src/pr85582-1.c exceeded 20s`.

Rows:

`src/20000224-1.c`, `src/20000731-2.c`, `src/loop-2b.c`,
`src/pr24716.c`, `src/pr85582-1.c`.

Stability for Step 3: stable enough for owner mapping as a timeout symptom
family, while preserving that this plan must not change timeout policy.

## Non-Runtime Context

The same accepted scan has `777` non-runtime failures. They are prerequisite or
separate compile/link failures, not runtime symptom rows:

- `738` rows: `[RV64_C4C_OBJ_COMPILE_FAIL]`.
- `11` rows: `[RV64_C4C_OBJ_COMPILE_TIMEOUT]`.
- `28` rows: `[RV64_C4C_LINK_FAIL]`.

These rows must stay out of the Step 3 runtime-owner mapping except where a
runtime row is later shown to require rerun after a specific compile-time
prerequisite.

## Step 3 Readiness

The abort/assertion, segfault, and timeout families are stable enough for Step
3 likely-first-owner mapping. Wrong output is stable as an empty family in the
current baseline. Step 3 should preserve the concrete row counts above and map
each runtime family to likely first owners without collapsing all runtime
symptoms into one generic runtime-support bucket.
