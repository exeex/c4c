# RV64 Undefined-Main Classification

Input list: `build/rv64_c_testsuite_probe_v2/undefined_main_cases.txt`

Tabular classification: `build/rv64_c_testsuite_probe_v2/classification.tsv`

Per-case prepared-BIR evidence:
`build/rv64_c_testsuite_probe_v2/classification_work/prepared_bir/`

Exact bucket case lists:
`build/rv64_c_testsuite_probe_v2/classification_work/buckets/`

## Primary Root Cause

All 93 undefined-main cases are classified under
`rv64_text_only_fail_closed`.

Evidence used:

- Every case in `undefined_main_cases.txt` has a corresponding prepared-BIR
  dump under `classification_work/prepared_bir/`.
- Each prepared-BIR dump still contains `prepared.func @main`.
- Each existing RV64 assembly output under
  `build/rv64_c_testsuite_probe_v2/asm/` is `.text` only, with no
  `.globl main`, no `main:` label, and no function body.
- Representative Step 2 evidence in
  `build/rv64_c_testsuite_probe_v2/representative_evidence.md` shows this for
  `src/00024.c`, `src/00025.c`, and `src/00094.c`.

This separates the common output-contract failure from secondary unsupported
features: these cases do not lose `main` before prepared BIR. They fail later
because the RV64 assembly route accepts the prepared module but emits a
fail-closed `.text`-only file.

Exact primary case list:
`build/rv64_c_testsuite_probe_v2/classification_work/buckets/rv64_text_only_fail_closed.txt`

## Secondary Follow-Up Buckets

| Secondary bucket | Count | Exact case list | Repair meaning |
| --- | ---: | --- | --- |
| `string_literals_and_extern_calls` | 59 | `classification_work/buckets/string_literals_and_extern_calls.txt` | String constants, formatted output, libc calls, or other direct external-call/address materialization remain follow-up hazards after function emission is repaired. |
| `aggregate_global_storage` | 19 | `classification_work/buckets/aggregate_global_storage.txt` | Aggregate, array, union, or struct global storage needs data emission and global-symbol address materialization. |
| `pointer_global_storage` | 8 | `classification_work/buckets/pointer_global_storage.txt` | Global pointer or function-pointer storage/initializers need data relocation and indirect-address support. |
| `floating_global_storage` | 2 | `classification_work/buckets/floating_global_storage.txt` | Global `double` storage needs floating-data representation and load/lower support. |
| `scalar_global_storage` | 2 | `classification_work/buckets/scalar_global_storage.txt` | Scalar global/static storage needs ordinary data emission and accesses. |
| `aggregate_or_control_only_shape` | 2 | `classification_work/buckets/aggregate_or_control_only_shape.txt` | Source has aggregate/control-flow shape but no observed global or string address demand in the prepared evidence. |
| `unused_extern_no_storage` | 1 | `classification_work/buckets/unused_extern_no_storage.txt` | Control case: `src/00094.c` has an unused extern declaration, empty prepared storage/addressing, and still emits `.text` only. |

## Representative Cases

| Case | Primary | Secondary | Evidence |
| --- | --- | --- | --- |
| `src/00094.c` | `rv64_text_only_fail_closed` | `unused_extern_no_storage` | Prepared BIR has `prepared.func @main`, `frame_size=0`, and no storage/addressing section demand, but asm is `.text` only. |
| `src/00024.c` | `rv64_text_only_fail_closed` | `aggregate_global_storage` | Prepared BIR records global-symbol accesses to `v` at offsets `0` and `4`; asm still omits `main` entirely. |
| `src/00025.c` | `rv64_text_only_fail_closed` | `string_literals_and_extern_calls` | Prepared BIR records a direct extern `strlen` call and string-constant address materialization for `.str0`; asm still emits only `.text`. |
| `src/00045.c` | `rv64_text_only_fail_closed` | `pointer_global_storage` | Source has global pointer storage initialized from `&x`; prepared main reaches RV64 handoff but output omits all code/data. |
| `src/00119.c` | `rv64_text_only_fail_closed` | `floating_global_storage` | Source has global `double x`; prepared main reaches RV64 handoff but output omits all code/data. |

## Interpretation

Step 4 repair-order planning should start with the common RV64 assembly
emission contract: a prepared `main` function must produce a real symbol and
body instead of a `.text`-only file. Once that is fixed, the secondary buckets
provide follow-up order for data and address materialization work. The unused
extern case is the minimal control, while global storage, pointer globals,
floating globals, string literals, and direct external calls are separate
follow-up feature families rather than the root cause of undefined `main`.
