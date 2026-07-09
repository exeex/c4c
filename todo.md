Status: Active
Source Idea Path: ideas/open/618_runtime_mismatch_ownership_investigation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Runtime Symptom Evidence

# Current Packet

## Just Finished

Step 1 for `ideas/open/618_runtime_mismatch_ownership_investigation.md`
refreshed the current runtime symptom evidence from the accepted full RV64 gcc
torture backend scan.

Accepted current full-scan evidence:

- Supervisor refresh command:
  `BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh > /tmp/c4c_618_full_scan.log 2>&1`.
- `/tmp/c4c_618_full_scan.log` ends with
  `total=1467 passed=473 failed=994`.
- `build/agent_state/rv64_gcc_c_torture_backend_summary.tsv`
  has `1468` lines including the header, mtime
  `2026-07-09 01:31:56 +0000`.
- `build/agent_state/rv64_gcc_c_torture_backend_failed.txt`
  has `994` rows, mtime `2026-07-09 01:31:56 +0000`.
- Current per-case evidence lives under
  `build/rv64_gcc_c_torture_backend/<case-id>/case.log`.

Current runtime row set:

- Runtime mismatches: `212` logs with `[RV64_BACKEND_RUNTIME_MISMATCH]`.
- Run timeouts: `5` logs with `[RV64_C4C_RUN_TIMEOUT]`.
- Total runtime symptom rows for Step 2: `217`.

Abort/assertion rows (`110`):

- `109` rows have `clang_exit=0 c4c_exit=Subprocess aborted`: `src/20020206-2.c`,
  `src/20030408-1.c`, `src/pr38533.c`, `src/pr63641.c`,
  `src/pr79737-2.c`, `src/20000227-1.c`, `src/20000412-3.c`,
  `src/20000622-1.c`, `src/20000707-1.c`, `src/20000715-1.c`,
  `src/20000717-1.c`, `src/20000717-5.c`, `src/20010518-2.c`,
  `src/20010520-1.c`, `src/20011019-1.c`, `src/20020107-1.c`,
  `src/20020108-1.c`, `src/20020510-1.c`, `src/20020611-1.c`,
  `src/20020615-1.c`, `src/20021118-1.c`, `src/20021118-3.c`,
  `src/20021120-2.c`, `src/20030128-1.c`, `src/20030914-2.c`,
  `src/20040309-1.c`, `src/20040311-1.c`, `src/20040319-1.c`,
  `src/20050104-1.c`, `src/20050215-1.c`, `src/20071216-1.c`,
  `src/20081117-1.c`, `src/20100209-1.c`, `src/20100805-1.c`,
  `src/20180131-1.c`, `src/900409-1.c`, `src/920612-1.c`,
  `src/920731-1.c`, `src/920908-2.c`, `src/921112-1.c`,
  `src/921123-2.c`, `src/921204-1.c`, `src/930208-1.c`,
  `src/930713-1.c`, `src/931005-1.c`, `src/931012-1.c`,
  `src/931017-1.c`, `src/931110-2.c`, `src/941015-1.c`,
  `src/950704-1.c`, `src/950710-1.c`, `src/951003-1.c`,
  `src/961026-1.c`, `src/980505-1.c`, `src/980602-2.c`,
  `src/980604-1.c`, `src/981206-1.c`, `src/990222-1.c`,
  `src/990324-1.c`, `src/bitfld-4.c`, `src/divconst-3.c`,
  `src/enum-1.c`, `src/enum-3.c`, `src/ieee/980619-1.c`,
  `src/ieee/pr67218.c`, `src/nestfunc-4.c`, `src/pending-4.c`,
  `src/pr22429.c`, `src/pr23941.c`, `src/pr28651.c`,
  `src/pr33779-1.c`, `src/pr37102.c`, `src/pr37882.c`,
  `src/pr39233.c`, `src/pr40747.c`, `src/pr41463.c`,
  `src/pr42721.c`, `src/pr48809.c`, `src/pr49161.c`,
  `src/pr49281.c`, `src/pr55137.c`, `src/pr57829.c`,
  `src/pr59014.c`, `src/pr61306-2.c`, `src/pr61517.c`,
  `src/pr61682.c`, `src/pr63302.c`, `src/pr63659.c`,
  `src/pr64957.c`, `src/pr65418-1.c`, `src/pr65418-2.c`,
  `src/pr67226.c`, `src/pr68376-1.c`, `src/pr69097-2.c`,
  `src/pr70222-2.c`, `src/pr77767.c`, `src/pr78559.c`,
  `src/pr78720.c`, `src/pr80501.c`, `src/pr83298.c`,
  `src/pr84339.c`, `src/pr89195.c`, `src/pr89826.c`,
  `src/restrict-1.c`, `src/struct-ini-1.c`, `src/vprintf-1.c`,
  `src/vrp-5.c`, `src/wchar_t-1.c`, `src/widechar-2.c`.
- `src/990106-1.c` has `clang_exit=0 c4c_exit=127` with a dynamic-loader
  assertion in `c4c_out`; keep it with abort/assertion symptoms for Step 2.
- Representative logs: `build/rv64_gcc_c_torture_backend/src_pr38533.c/case.log`,
  `build/rv64_gcc_c_torture_backend/src_20020510-1.c/case.log`, and
  `build/rv64_gcc_c_torture_backend/src_990106-1.c/case.log`.

Segfault rows (`102`):

- `src/pr30778.c`, `src/pr35472.c`, `src/pr56982.c`, `src/strlen-7.c`,
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
- Representative logs: `build/rv64_gcc_c_torture_backend/src_20000412-2.c/case.log`,
  `build/rv64_gcc_c_torture_backend/src_20000706-5.c/case.log`, and
  `build/rv64_gcc_c_torture_backend/src_strct-pack-1.c/case.log`.

Wrong-output rows (`0`):

- No refreshed runtime-mismatch log had `clang_exit=0 c4c_exit=0`, and no
  wrong-output row was found by
  `rg -n "c4c_exit=0|wrong|actual|expected|diff" build/rv64_gcc_c_torture_backend/*/case.log`
  beyond unrelated diagnostic text.

Run-timeout rows (`5`):

- `src/20000224-1.c`, `src/20000731-2.c`, `src/loop-2b.c`,
  `src/pr24716.c`, `src/pr85582-1.c`.
- Each representative log reports `[RV64_C4C_RUN_TIMEOUT]` and says the row
  exceeded the 20s harness limit; see
  `build/rv64_gcc_c_torture_backend/src_20000224-1.c/case.log` and
  `build/rv64_gcc_c_torture_backend/src_pr85582-1.c/case.log`.

Compile-time and non-runtime owners are separate from the runtime row set:

- `738` rows: `[RV64_C4C_OBJ_COMPILE_FAIL]`.
- `11` rows: `[RV64_C4C_OBJ_COMPILE_TIMEOUT]`.
- `28` rows: `[RV64_C4C_LINK_FAIL]`.
- These `777` non-runtime failures should stay outside the Step 2 runtime
  symptom table except as prerequisite/non-runtime context.

Representative commands/log paths for Step 2:

- Full scan command:
  `BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh > /tmp/c4c_618_full_scan.log 2>&1`.
- Runtime mismatch inventory:
  `rg -n "\\[RV64_BACKEND_RUNTIME_MISMATCH\\]|clang_exit=0 c4c_exit=" build/rv64_gcc_c_torture_backend/*/case.log`.
- Run-timeout inventory:
  `rg -n "\\[RV64_C4C_RUN_TIMEOUT\\]" build/rv64_gcc_c_torture_backend/*/case.log`.
- Non-runtime owner inventory:
  `rg -n "\\[RV64_C4C_OBJ_COMPILE_FAIL\\]|\\[RV64_C4C_OBJ_COMPILE_TIMEOUT\\]|\\[RV64_C4C_LINK_FAIL\\]" build/rv64_gcc_c_torture_backend/*/case.log`.
- Summary artifacts:
  `build/agent_state/rv64_gcc_c_torture_backend_summary.tsv` and
  `build/agent_state/rv64_gcc_c_torture_backend_failed.txt`.

## Suggested Next

Proceed to Step 2 and write
`docs/runtime_mismatch_ownership/01_runtime_symptom_map.md` from the accepted
full-scan evidence above.

## Watchouts

`src/990106-1.c` is not a normal `Subprocess aborted` row; it exits `127`
after a dynamic-loader assertion. Keep it explicit as an abort/assertion
subcase rather than hiding it in the ordinary abort rows.

The runtime documentation should avoid mixing the `217` runtime symptom rows
with the `777` compile/link/compile-timeout failures. Do not change
implementation files, tests, expected outputs, unsupported markers, allowlists,
timeout policy, runtime comparison behavior, or unrelated lifecycle files.

## Proof

Documentation/research evidence refresh only. No build or backend proof was
run by this executor. The accepted full-scan log is
`/tmp/c4c_618_full_scan.log`; `test_after.log` was not created or modified.
