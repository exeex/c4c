# AArch64 C-Testsuite Failure Family Inventory Todo

Status: Active
Source Idea Path: ideas/open/284_aarch64_c_testsuite_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify Remaining Failure Families

# Current Packet

## Just Finished

Step 2 classified the remaining non-passing AArch64 backend c-testsuite cases
from `/tmp/aarch64_backend_inventory_step1_20260518T154828Z.log`.

Inventory totals remain: 212 total, 80 passed, 129 failed non-timeout, 3
timed out, 132 failed including timeouts.

Semantic classification:
- `[FRONTEND_FAIL]` compile-stage backend diagnostics: 49 cases. These are
  mostly AArch64 assembly-route backend gaps, not C parser failures:
  - compare-branch printer gaps: 21 cases, e.g. `00030.c`, `00034.c`,
    `00037.c`, `00054.c`, `00127.c`, `00200.c`, `00215.c`.
  - scalar bitwise printer gaps: 9 cases, e.g. `00027.c` `or`, `00028.c`
    `and`, `00029.c`/`00104.c`/`00208.c` `xor`.
  - add/sub immediate-form gaps: `00024.c` immediate lhs subtract, `00031.c`
    and `00213.c` large add/sub immediate.
  - sign-extension cast operand gaps: `00134.c`, `00135.c`.
  - semantic `lir_to_bir` gaps before prepared-module handoff: 14 cases,
    e.g. anonymous aggregate/union `00046.c`, aggregate argument/varargs
    `00140.c`, stdio/string-heavy later tests `00157.c`, `00176.c`,
    `00181.c`, `00185.c`, `00195.c`, `00204.c`, `00216.c`, `00218.c`.
- `[BACKEND_FAIL]`: 2 linker failures, `00119.c` and `00123.c`, both global
  `double x` comparison programs. Linker reports undefined `x` and PIC-unsafe
  `R_AARCH64_ADR_PREL_PG_HI21`, so owner is global object/data-symbol
  materialization for floating globals.
- `[RUNTIME_NONZERO]`: 55 cases. Exit-code grouping:
  - Bus error: 28 cases, including minimal pointer/local cases `00004.c`,
    `00005.c`, `00013.c`, `00014.c`, arrays `00015.c`/`00016.c`, structs
    `00017.c`/`00042.c`, function-pointer cases `00087.c`/`00089.c`, and
    attribute/function-pointer `00210.c`. Representative generated assembly
    for `00004.c` uses `sub sp, sp, #24`, making AArch64 stack-frame/SP
    16-byte alignment the most tractable runtime owner before treating these
    as broad pointer-semantics failures.
  - Segmentation fault: `00019.c`, `00170.c`, `00179.c`, `00189.c`; likely
    pointer/string/function-pointer runtime paths and should stay separate
    from the bus-error SP-alignment cluster.
  - Wrong nonzero exits: arithmetic/condition/data cases including `00009.c`
    exit 38, `00012.c` exit 64, `00036.c`, `00039.c`, `00043.c`, `00044.c`,
    `00045.c`, `00049.c`, `00051.c`, `00064.c`, `00066.c`, `00072.c`,
    `00073.c`, `00079.c`, `00081.c`, `00082.c`, `00086.c`, `00102.c`,
    `00109.c`, `00112.c`, `00130.c`, `00139.c`, `00144.c`.
- `[RUNTIME_MISMATCH]`: 23 cases. Representative families:
  - loops/branching/control-flow output: `00156.c`, `00158.c`, `00160.c`,
    `00161.c`, `00168.c`, `00169.c`, `00183.c`, `00192.c`, `00193.c`,
    `00194.c`, `00196.c`; generated `00156.c` assembly compares `Count`
    against `0` for a `Count <= 10` loop, so compare/branch lowering is a
    plausible runtime owner.
  - printf/argument passing and calls: `00056.c`, `00159.c`, `00175.c`.
  - pointer/string/libc and memory writes: `00172.c`, `00180.c`, `00186.c`,
    `00187.c`, `00217.c`.
  - floating arithmetic/calls: `00174.c`.
  - macro/generic semantic output: `00202.c`, `00219.c`; `00219.c` differs on
    `_Generic` const-selection output and should not be folded into backend
    compare/branch work without frontend/source-semantics review.
- Timeout-sensitive cases: `00132.c`, `00173.c`, `00220.c`. Keep these as a
  timeout/hang bucket; do not mix them into ordinary mismatch/nonzero repair
  slices until timeout-specific evidence is gathered.

Best Step 3 split candidate: AArch64 stack-frame/SP 16-byte alignment for the
bus-error runtime cluster, starting with `00004.c` plus nearby pointer/array
locals (`00005.c`, `00013.c`, `00014.c`, `00015.c`, `00016.c`). This is
semantic, tractable, and timeout-independent. Secondary candidate:
AArch64 compare-branch lowering/printing, which has 21 compile-stage printer
failures and likely contributes to runtime mismatches such as `00156.c`.

## Suggested Next

Supervisor should split Step 3 into a focused repair idea for AArch64
stack-frame/SP 16-byte alignment, proved first on `00004.c` and then nearby
bus-error local pointer/array cases. Keep compare-branch lowering/printing as
the next focused split if the supervisor wants a compile-stage backend family
instead.

## Watchouts

- This umbrella plan is inventory and lifecycle routing only; do not implement
  compiler repairs under it.
- Do not change tests, expected outputs, runner behavior, CTest registration,
  allowlists, unsupported classifications, or timeout policy.
- The refreshed scan still has 3 timeout cases: `00132.c`, `00173.c`,
  `00220.c`. Keep timeout/hang work separate from ordinary
  mismatch/nonzero/backend/frontend families unless a timeout-specific focused
  idea is created.
- Do not treat all bus errors as pointer-semantics bugs before checking SP
  alignment; `00004.c` has a minimal generated frame of 24 bytes, which is not
  AArch64 16-byte aligned.
- Do not combine `_Generic`/frontend semantic mismatches such as `00219.c`
  with backend compare/branch work.
- Split a focused `ideas/open/*.md` repair idea before implementation starts.

## Proof

Primary evidence:
`ctest --test-dir build-aarch64-scan -L '^aarch64_backend$' -j 8 --timeout 5 --output-on-failure`

Result from Step 1 log: CTest exited `8` with 80 passed, 129 failed
non-timeout, and 3 timed out out of 212 total tests. Raw log:
`/tmp/aarch64_backend_inventory_step1_20260518T154828Z.log`.

Additional read-only inspection used generated/source artifacts for
representative classification only, including `00004.c.s` and `00156.c.s`.
No tests or broad scans were run for Step 2, and no `test_after.log` was
created per the delegated packet.
