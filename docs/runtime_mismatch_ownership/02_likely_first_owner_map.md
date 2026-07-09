# Likely First Owner Map

Source idea: `ideas/open/618_runtime_mismatch_ownership_investigation.md`

This file records the Step 3 likely-first-owner map for the accepted July 9
RV64 gcc torture backend runtime evidence. It uses the symptom inventory in
`01_runtime_symptom_map.md` as the baseline:

- `217` runtime symptom rows.
- `110` abort or assertion rows.
- `102` segfault rows.
- `0` wrong-output rows.
- `5` timeout rows.

The source idea was opened from an older `75` row estimate. Step 1 accepted the
newer full-scan evidence, so this document classifies the current `217` row
runtime set instead of the stale estimate.

## Owner Rules

The allowed owner buckets are ABI, layout, local/global memory, call lowering,
true runtime support, and unresolved. A runtime exit symptom is not, by itself,
an owner. This map therefore separates:

- direct owner evidence, where the log names a concrete mechanism;
- heuristic owner evidence, where source shape suggests a likely lane but the
  current log only proves a runtime symptom;
- unresolved rows, where a rerun after prerequisite compile-time or codegen
  ideas is required before implementation ownership is credible.

The evidence baseline is:

- scan command:
  `BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh > /tmp/c4c_618_full_scan.log 2>&1`
- summary:
  `build/agent_state/rv64_gcc_c_torture_backend_summary.tsv`
- failed list:
  `build/agent_state/rv64_gcc_c_torture_backend_failed.txt`
- per-case logs:
  `build/rv64_gcc_c_torture_backend/<case-id>/case.log`

## Owner Rollup

| Runtime family | Rows | Likely first owner now | Confidence | Why |
| --- | ---: | --- | --- | --- |
| Dynamic-loader assertion | 1 | call lowering | high | The log names an ELF relocation kind mismatch during `_dl_fixup`. |
| Generic abort | 109 | unresolved, split by rerun lane | low | The logs only show `Subprocess aborted`; abort may be from deliberate testcase `abort()`, bad call/result ABI, memory corruption, layout, or runtime/library behavior. |
| Segfault | 102 | unresolved, split by rerun lane | low | The logs only show `Segmentation fault`; likely causes span pointer memory, stack/global layout, ABI, calls, and true runtime support. |
| Wrong output | 0 | no current owner | high | No accepted baseline row has clang and c4c both exiting `0` with output differences. |
| Timeout | 5 | unresolved, with true-runtime-support and miscompile hypotheses | low | The logs only prove the harness `20s` run limit was exceeded; timeout policy is out of scope. |

## Dynamic-Loader Assertion

Rows: `src/990106-1.c`.

Likely first owner: call lowering.

Evidence for call lowering:

- `build/rv64_gcc_c_torture_backend/src_990106-1.c/case.log` reports
  `[RV64_BACKEND_RUNTIME_MISMATCH]`, `clang_exit=0 c4c_exit=127`, and an
  `ld.so` `_dl_fixup` assertion that the relocation type is not
  `ELF_MACHINE_JMP_SLOT`.
- The assertion is raised by dynamic relocation handling before normal program
  output can matter. That points at generated call relocation shape, PLT/GOT
  use, or external symbol call lowering rather than a source-level branch or
  arithmetic result.

Evidence against other owners:

- ABI: possible only as a later consumer of the call result; the log fails in
  dynamic relocation rather than in argument passing or return value checking.
- layout: no stack, aggregate, or object layout evidence appears in the log.
- local/global memory: no local or global memory access diagnostic is visible;
  the named failure is relocation fixup.
- true runtime support: the loader assertion is not evidence that libc or qemu
  lacks a runtime feature. Clang's equivalent object exits `0`.

Rerun requirement:

- Rerun `src/990106-1.c` after external call relocation/call-lowering work.
  If it stops asserting in `ld.so` and then aborts, segfaults, or times out,
  reclassify the new symptom separately.

## Generic Abort Family

Rows: `109` normal abort rows from `01_runtime_symptom_map.md`, excluding
`src/990106-1.c`.

Likely first owner now: unresolved. The family should be split for reruns
rather than treated as true runtime support.

Representative evidence:

- `build/rv64_gcc_c_torture_backend/src_pr38533.c/case.log` reports
  `[RV64_BACKEND_RUNTIME_MISMATCH]` with `clang_exit=0
  c4c_exit=Subprocess aborted`.
- `build/rv64_gcc_c_torture_backend/src_20020510-1.c/case.log` reports the
  same runtime mismatch and abort shape.

Evidence for unresolved:

- Most gcc torture tests call `abort()` when a computed condition differs from
  the expected result. A runtime abort therefore frequently means a silent
  miscompile reached a self-check, not necessarily that process abort support
  itself is broken.
- The current per-case logs do not include the failed source assertion,
  register state, memory address, or qemu trace needed to choose among ABI,
  layout, local/global memory, call lowering, or true runtime support.
- The same scan still has `777` compile/link failures. Rows that currently run
  may still sit behind neighboring local-memory, global-data, ABI, and RV64/MIR
  codegen gaps, so owner inference from final exit code alone is unstable.

Heuristic rerun lanes:

| Lane | Example rows from the abort set | Why this lane should rerun first |
| --- | --- | --- |
| ABI | `src/vprintf-1.c`, `src/nestfunc-4.c`, `src/wchar_t-1.c`, `src/widechar-2.c` | Calls, variadic/library surfaces, and wide-character handling can expose argument, return, or external-call ABI failures before true runtime support is known. |
| layout | `src/bitfld-4.c`, `src/struct-ini-1.c`, `src/pr49161.c` | Bitfield, aggregate initializer, and structure-heavy cases can abort after layout or field-offset miscompilation. |
| local/global memory | `src/pr79737-2.c`, `src/pr80501.c`, `src/restrict-1.c`, `src/enum-1.c` | Pointer, alias, enum, and memory-heavy rows can self-abort after local or global memory facts are wrong. |
| call lowering | `src/pr38533.c`, `src/pr63641.c`, `src/pr83298.c` | Call-heavy rows may be affected by ordinary call/result lowering or external symbol calls, but the current abort log does not prove it. |
| true runtime support | `src/ieee/980619-1.c`, `src/ieee/pr67218.c`, `src/divconst-3.c` | Floating/libgcc-like helpers, IEEE behavior, or division support may require runtime support checks after ABI and call lowering are known good. |

Evidence against assigning the whole family to one owner:

- ABI alone cannot explain rows with local pointer, aggregate, or arithmetic
  self-check shapes.
- layout alone cannot explain call-heavy, library, or arithmetic helper rows.
- local/global memory alone cannot explain the dynamic-linker assertion or
  obvious call surfaces.
- call lowering alone cannot explain pure aggregate or local-memory
  self-checks.
- true runtime support is premature because many rows can reach `abort()` due
  to wrong generated code while still using ordinary libc `abort()` correctly.

Rerun requirement:

- Rerun the generic abort rows after the next single-owner compile-time or
  codegen ideas for local/global memory, layout, ABI, and call lowering. Rows
  that still abort after those prerequisites should be sub-bucketed by failed
  source predicate or qemu trace before any true-runtime-support idea is
  opened.

## Segfault Family

Rows: `102` rows listed under `Segfault` in `01_runtime_symptom_map.md`.

Likely first owner now: unresolved, with strong mixed-owner evidence.

Representative evidence:

- `build/rv64_gcc_c_torture_backend/src_20000412-2.c/case.log` reports
  `[RV64_BACKEND_RUNTIME_MISMATCH]` with `clang_exit=0
  c4c_exit=Segmentation fault`.
- `build/rv64_gcc_c_torture_backend/src_strct-pack-1.c/case.log` reports the
  same segfault shape.

Evidence for unresolved:

- A segfault proves an invalid runtime access or control transfer, but it does
  not identify whether the invalid address came from bad stack layout, aggregate
  layout, local memory lowering, global object data, call/return ABI, or a true
  runtime helper issue.
- `src/strct-pack-1.c` uses a packed/aligned structure containing `short` and
  `double`; that is layout-shaped evidence, but its log still only proves a
  segfault.
- `src/va-arg-20.c` uses `va_arg` over an `unsigned long long`; that is
  ABI-shaped evidence, but current logs do not prove whether the fault is in
  varargs setup, stack layout, or the loaded value.
- `src/strlen-1.c`, `src/strcmp-1.c`, and `src/strncmp-1.c` exercise string
  library calls over local/global buffers; those rows may be memory, ABI, call,
  or true runtime support after prerequisites.

Heuristic rerun lanes:

| Lane | Example rows from the segfault set | Why this lane should rerun first |
| --- | --- | --- |
| layout | `src/strct-pack-1.c`, `src/bf-pack-1.c`, `src/930603-3.c`, `src/950322-1.c` | Packed structures, bitfields, and aggregate-heavy rows can create invalid field addresses if layout is wrong. |
| ABI | `src/va-arg-20.c`, `src/20000603-1.c`, `src/20011126-2.c`, `src/20011219-1.c` | Varargs and call/result surfaces can put values in the wrong register or stack home and then fault. |
| local/global memory | `src/strlen-1.c`, `src/strlen-7.c`, `src/string-opt-17.c`, `src/mayalias-3.c` | String and alias tests are memory-shaped but still need local/global memory facts and call surfaces separated. |
| call lowering | `src/strcmp-1.c`, `src/strncmp-1.c`, `src/pr20466-1.c`, `src/pr20527-1.c` | External/library call rows can fault through call target, argument, or result handling. |
| true runtime support | `src/ieee/mzero2.c`, `src/divconst-1.c` | IEEE or helper-like behavior may need true runtime support checks only after ABI and call lowering are credible. |

Evidence against assigning the whole family to one owner:

- The segfault set contains packed layout rows, varargs rows, string/library
  rows, pointer/alias rows, and arithmetic/helper rows. A single owner would
  hide the mixed evidence.
- True runtime support is not the first owner for the whole family because a
  miscomputed pointer, frame slot, or call target can produce the same signal.
- ABI is plausible for some rows, but structure packing and local/global
  memory examples show it cannot own the whole family.

Rerun requirement:

- Rerun segfault rows after layout, local/global memory, ABI, and call-lowering
  prerequisites. A useful next evidence pass should capture at least the fault
  address, failing instruction, and whether the fault occurs before or after
  the first external call.

## Wrong Output Family

Rows: none in the accepted baseline.

Likely first owner now: no current owner.

Evidence:

- `01_runtime_symptom_map.md` records `0` wrong-output rows.
- The accepted scan did not find a row where `clang_exit=0` and `c4c_exit=0`
  with output diff evidence.

Rerun requirement:

- If later prerequisite fixes convert aborts or segfaults into clean exits with
  output differences, those rows should be classified as a new wrong-output
  family. Do not backfill a wrong-output owner into the current baseline.

## Timeout Family

Rows: `src/20000224-1.c`, `src/20000731-2.c`, `src/loop-2b.c`,
`src/pr24716.c`, `src/pr85582-1.c`.

Likely first owner now: unresolved. True runtime support is plausible for some
rows, but not proven. Timeout policy is explicitly out of scope.

Representative evidence:

- `build/rv64_gcc_c_torture_backend/src_20000224-1.c/case.log` reports
  `[RV64_C4C_RUN_TIMEOUT]` and that the case exceeded `20s`.
- `build/rv64_gcc_c_torture_backend/src_pr85582-1.c/case.log` reports the
  same timeout classification.

Evidence for unresolved:

- A timeout may be a loop-control miscompile, a bad branch/compare lowering, a
  memory corruption that prevents loop progress, or an actual missing runtime
  support/performance problem.
- `src/20000224-1.c` is loop-shaped source: it increments a counter under a
  flag-controlled loop and should return quickly. That is evidence for a
  branch/compare or local-memory/control-flow rerun lane before timeout policy
  or true runtime support.
- The plan forbids timeout-policy changes, so increasing or weakening the `20s`
  limit is not an owner classification.

Evidence against other broad owners:

- ABI does not explain all timeout rows without more evidence because some
  timeout cases can be pure loop/control-flow failures.
- layout and local/global memory may explain individual rows if loop state or
  pointer state is corrupted, but the current logs do not prove which.
- true runtime support is not proven because the timeout could be generated
  code failing to terminate, not a missing runtime helper.

Rerun requirement:

- Rerun timeout rows after branch/compare, local/global memory, and call/ABI
  prerequisites that can affect loop progress. A useful follow-up evidence pass
  should record whether the program is stuck in generated loop code, a runtime
  helper, or an external/library call.

## Rows To Rerun After Prerequisite Ideas

The following groups should be rerun before any implementation idea claims
runtime ownership:

| Rerun trigger | Rows or family | Reason |
| --- | --- | --- |
| External call relocation/call lowering | `src/990106-1.c` | Direct loader assertion points at relocation/call shape; new symptoms after that fix need reclassification. |
| ABI call/result and varargs | `src/va-arg-20.c`, `src/vprintf-1.c`, call-heavy abort and segfault rows | Current logs cannot separate argument homes, return movement, varargs layout, and true runtime support. |
| Layout and aggregate packing | `src/strct-pack-1.c`, `src/bf-pack-1.c`, `src/bitfld-4.c`, aggregate-heavy abort/segfault rows | Packed structs and bitfields can create wrong addresses or wrong self-check values. |
| Local/global memory | string, alias, restrict, pointer, and global-buffer rows such as `src/strlen-1.c`, `src/strlen-7.c`, `src/mayalias-3.c`, `src/restrict-1.c` | Memory fact gaps can surface only as abort or segfault once object emission succeeds. |
| Branch/compare/control flow | timeout rows and loop-shaped runtime rows | Nontermination can be caused by generated branch/compare behavior, not timeout policy. |
| True runtime support | rows still failing after the owner-specific reruns above | Only residual rows with stable ABI/layout/memory/call evidence should become true runtime support candidates. |

## Step 4 Recommendation

Step 4 should create a follow-up queue that is owner-first, not symptom-first:

- one call-lowering follow-up or rerun note for `src/990106-1.c`;
- ABI rerun/evidence work for call, return, and varargs-shaped runtime rows;
- layout rerun/evidence work for packed aggregate and bitfield-shaped rows;
- local/global memory rerun/evidence work for string, alias, pointer, and
  global-buffer rows;
- timeout investigation only as control-flow/runtime evidence, with no timeout
  policy change;
- true runtime support only for residual rows after those prerequisites.

No implementation owner is ready for the generic abort, generic segfault, or
timeout families from the current logs alone.
