Status: Active
Source Idea Path: ideas/open/310_rv64_c_testsuite_qemu_nonzero_runtime_triage.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Rank Repair Families

# Current Packet

## Just Finished

Step 4: Build Semantic Root-Cause Buckets classified all 123 real
`QEMU_NONZERO`/`QEMU_TIMEOUT` rows from
`build/rv64_c_testsuite_probe_latest/summary.tsv` using the saved assembly and
Step 3 evidence. The full per-case table is in
`build/rv64_c_testsuite_probe_latest/triage_step4/classification.tsv`, with a
human summary in `build/rv64_c_testsuite_probe_latest/triage_step4/summary.md`.

| Bucket | Count | Status split | Representative cases | Mechanism evidence |
| --- | ---: | --- | --- | --- |
| `unresolved_external_or_empty_stub_emission` | 59 | `QEMU_NONZERO 132=47`, `QEMU_NONZERO 139=11`, `QEMU_TIMEOUT 124=1` | `src/00025.c`, `src/00056.c`, `src/00125.c`, `src/00179.c`, `src/00220.c` | Calls to libc/libm/string/extern functions are followed by bodyless `.globl` labels such as `strlen:`, `printf:`, `__overflow:`, `strlcpy:`, or `fmal:`. `src/00025.c` is the timeout exemplar. |
| `incomplete_control_or_expression_emission` | 23 | `QEMU_NONZERO 132=18`, `QEMU_NONZERO 139=5` | `src/00006.c`, `src/00008.c`, `src/00030.c`, `src/00105.c`, `src/00143.c` | Assembly enters loop/condition/arithmetic/call-result code and stops before the needed branch, value finalization, epilogue, or `ret`; the Step 3 `src/00008.c` loop tail is the anchor. |
| `stack_pointer_local_slot_materialization` | 21 | `QEMU_NONZERO 132=14`, `QEMU_NONZERO 139=7` | `src/00005.c`, `src/00019.c`, `src/00032.c`, `src/00072.c`, `src/00130.c` | Local pointers, arrays, aggregates, function pointers, or address-taken stack objects lose the effective address/value path before use; the Step 3 `src/00005.c` pointer-to-pointer tail is the anchor. |
| `global_storage_or_global_address_flow` | 8 | `QEMU_NONZERO 132=6`, `QEMU_NONZERO 139=2` | `src/00033.c`, `src/00051.c`, `src/00090.c`, `src/00147.c`, `src/00151.c` | Data symbols and first `lla`/`lw` loads are emitted, but comparison/switch/aggregate-read control does not complete. |
| `wide_or_narrow_scalar_storage_lowering` | 7 | `QEMU_NONZERO 132=1`, `QEMU_NONZERO 139=6` | `src/00081.c`, `src/00082.c`, `src/00086.c`, `src/00104.c`, `src/00135.c` | Long/long long, short, or fixed-width scalar local lowering opens a frame but omits the required load/store/compare/return sequence. |
| `unmaterialized_value_or_comparison_result` | 5 | `QEMU_NONZERO 112=1`, `QEMU_NONZERO 132=4` | `src/00042.c`, `src/00064.c`, `src/00066.c`, `src/00112.c`, `src/00113.c` | A value/comparison exists in source or BIR but RV64 emission leaves an uninitialized/no-op temporary or empty body; Step 3 `src/00112.c` returns `t0` instead of materializing `@.str0 == 0`. |

No confirmed genuine non-crashing nonzero-semantics bucket was found. The lone
status `112` row is an unmaterialized scratch return, not a deliberate semantic
result. The two allowlist/comment rows, `CLANG_FAIL` temporary-label
diagnostics, `EMIT_FAIL` semantic handoff failures, and unsupported
global-storage emit failures are visibly excluded from these runtime buckets.

## Suggested Next

Start Step 5 from `plan.md`: rank repair families by likely runtime-yield and
implementation risk. The top-ranking candidates should compare the 59-row empty
external/stub family against the 23-row incomplete control/expression family
and the 21-row stack/local materialization family.

## Watchouts

This active idea remains triage-only. Do not implement runtime fixes or weaken
test contracts while ranking Step 5. Some rows could fit more than one symptom,
especially stdio/string cases that also exercise arrays or loops; Step 4 assigns
the most direct emitted-code mechanism visible in the saved assembly, with
external empty-stub emission taking precedence when bodyless call targets are
present.

## Proof

No build or CTest proof required for this triage-only classification update.
Proof was read-only inspection of
`build/rv64_c_testsuite_probe_latest/summary.tsv`,
`build/rv64_c_testsuite_probe_latest/asm/*.s`, and the Step 3 artifacts. The
classification artifact was checked to contain 123 rows and preserve the
original runtime status split: `QEMU_NONZERO 132=90`, `QEMU_NONZERO 139=31`,
`QEMU_NONZERO 112=1`, `QEMU_TIMEOUT 124=1`. No `test_after.log` was produced
because the delegated proof explicitly did not require build/ctest proof.
