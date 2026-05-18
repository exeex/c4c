# AArch64 C-Testsuite Failure Family Inventory Todo

Status: Active
Source Idea Path: ideas/open/284_aarch64_c_testsuite_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify Representative Failure Families

# Current Packet

## Just Finished

Step 2 classified representative current AArch64 backend c-testsuite failure
families from the refreshed 212-case Step 1 evidence:
`/tmp/c4c_aarch64_backend_scan_212.log` and
`/tmp/c4c_aarch64_backend_scan_212.classified`.

Major families and representative cases:

- `FRONTEND_FAIL` is mostly backend compile-time gating, not C frontend parsing.
  The runner labels compiler nonzero as frontend failure, but representative
  errors come from AArch64 semantic lowering or machine-node printing:
  - machine printer compare-branch operands: 21 cases, e.g. `src/00030.c`,
    `src/00076.c`, `src/00101.c`, `src/00214.c`.
  - machine printer bitwise scalar opcodes outside add/sub: 9 cases, e.g.
    `src/00027.c`, `src/00028.c`, `src/00029.c`, `src/00151.c`,
    `src/00208.c`.
  - machine printer immediate/cast operand gaps: `src/00024.c` sub immediate
    lhs, `src/00031.c` and `src/00213.c` add/sub immediate range,
    `src/00134.c` and `src/00135.c` sign-extension operand shape.
  - semantic `lir_to_bir` local-memory admissions: GEP family in 9 cases
    (`src/00143.c` Duff-device array copy, `src/00157.c` local array indexing,
    `src/00176.c` quicksort/global array indexing, `src/00181.c` Hanoi,
    `src/00182.c` LED buffer indexing, `src/00205.c`, `src/00209.c`), plus
    load/store local-memory in `src/00046.c`, `src/00140.c`, `src/00216.c`,
    and `src/00218.c`.
  - semantic global aggregate/data admission: `src/00204.c` large aggregate,
    struct/HFA/ABI data case.
  Likely owner boundary: backend semantic lowering and AArch64 machine printer,
  not test runner, parser, or expectations.

- `RUNTIME_MISMATCH` is dominated by backend data/string/call argument
  materialization issues after successful assembly and execution. Representative
  `printf`/stdio cases either print nothing or binary garbage:
  `src/00125.c` basic `printf("hello world")` compiles to `mov x0, x13; bl
  printf`, `src/00131.c` repeated string prints produce garbage bytes,
  `src/00154.c` struct field `printf` output is garbage, `src/00161.c` looped
  integer output is garbage, and `src/00211.c` macro integer expression output
  is garbage. Related mismatch cases cover loops/control-flow plus stdio
  (`src/00156.c`, `src/00158.c`, `src/00160.c`, `src/00168.c`, `src/00169.c`,
  `src/00183.c`, `src/00192.c`), function calls (`src/00159.c`,
  `src/00190.c`), size/static/global/preprocessor data (`src/00178.c`,
  `src/00184.c`, `src/00191.c`, `src/00197.c`, `src/00201.c`,
  `src/00206.c`). Likely owner boundary: backend constant/global/string
  address materialization, call-argument lowering, and data layout/runtime
  interop.

- `RUNTIME_NONZERO` splits into assertion-return miscompiles and invalid
  memory/control crashes:
  - local pointer, array, and aggregate addressing: `src/00004.c`,
    `src/00005.c`, `src/00013.c`, `src/00016.c`, `src/00019.c`,
    `src/00032.c`.
  - scalar arithmetic/operators/control-flow that compile but return wrong
    assertion codes: `src/00009.c` mul/div/rem, `src/00012.c` folded
    arithmetic, `src/00036.c` compound assignment, `src/00064.c` macro
    arithmetic, `src/00109.c` ternary.
  - globals/statics and simple calls: `src/00023.c`, `src/00107.c`,
    `src/00116.c`, with broader static/global evidence in `src/00197.c`.
  - crashes around strings/preprocessor/generic/wide data and ABI-ish cases:
    `src/00137.c` stringification/string literal, `src/00175.c` char/int/float
    conversion plus stdio, `src/00196.c` short-circuit calls, `src/00202.c`
    token pasting, `src/00210.c` attributes/function-pointer casts,
    `src/00217.c` unaligned pointer arithmetic/global string, `src/00219.c`
    `_Generic`, and `src/00220.c` wide characters. The refreshed log has
    `RUNTIME_NONZERO` exit-shape counts: 43 exit `1`, 28 bus errors, 25
    segmentation faults, and five one-off numeric exits (`2`, `29`, `111`,
    `128`, `230`). Likely owner boundary: backend runtime semantics and AArch64
    lowering; only cases whose C feature is accepted but semantically dubious
    should be deferred to frontend triage after a focused backend owner is
    chosen.

- `TIMEOUT` has one representative: `src/00132.c`. The source is a finite
  `printf` plus `for (Count = -5; Count <= 5; Count++)` case, but generated
  assembly loads `Count` from stack without the visible `-5` initialization and
  loops through a block that does not visibly increment it. The saved run output
  file is about 1.26 GB of repeated garbage bytes, consistent with an infinite
  or effectively unbounded loop plus bad `printf` format/string materialization.
  Likely owner boundary: backend loop/control-flow and local-store lowering,
  compounded by the same string/call lowering family as `RUNTIME_MISMATCH`.

Deferred families for Step 3 route selection:

- Do not treat the runner's `FRONTEND_FAIL` bucket name as proof of parser or
  sema ownership; first split printer-vs-lir_to_bir backend compile-time
  failures.
- Do not select a named-case repair from this inventory alone. The best focused
  candidates appear to be semantic backend families: string/global address and
  call-argument materialization for stdio cases, or local-memory GEP/load/store
  lowering for arrays/aggregates, or machine-printer compare-branch/bitwise
  coverage.
- Keep complex ABI/aggregate cases such as `src/00204.c`, `src/00210.c`,
  `src/00216.c`, `src/00217.c`, `src/00219.c`, and `src/00220.c` deferred
  unless the next focused idea explicitly targets those broader semantics.

## Suggested Next

Step 3 should create or select one focused idea from the classified families.
The most routeable backend targets are: string/global address plus call-argument
materialization for stdio mismatch/timeout cases; local-memory GEP/load/store
semantics for array/aggregate runtime and compile-time cases; or machine-printer
coverage for compare-branch and bitwise scalar opcodes.

## Watchouts

- This umbrella route is planning and classification only; do not implement
  compiler, runner, or test expectation changes here.
- `FRONTEND_FAIL` is a runner bucket for compiler nonzero. Most sampled cases
  are backend route failures after the AArch64 assembly path starts.
- `src/00132.c.bin.run-output` is very large because the timeout generated
  repeated garbage output; avoid opening it without size-limited tools.
- Numeric `RUNTIME_NONZERO` exits often mean assertion-return miscompiles; bus
  error and segmentation fault exits point at invalid address/data/ABI lowering.
- Split and switch to a focused idea before implementation work begins.

## Proof

Evidence used:

- `/tmp/c4c_aarch64_backend_scan_212.log`
- `/tmp/c4c_aarch64_backend_scan_212.classified`
- representative sources under `tests/c/external/c-testsuite/src/`
- representative generated assembly/run-output under
  `build-aarch64-scan/c_testsuite_aarch64_backend/src/`

No broad scan was rerun. No root-level proof log was created or modified for
this evidence-only classification packet.
