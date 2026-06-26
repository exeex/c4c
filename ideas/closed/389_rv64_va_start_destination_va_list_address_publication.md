# RV64 `va_start` Destination `va_list` Address Publication

Status: Closed
Type: Target lowering follow-up
Parent: `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`
Split From: closure of `ideas/closed/388_rv64_object_route_variadic_va_end_boundary.md`
Closed By: implementation commit `23b47a43` and Step 5 rerun commit `35c0cd2a`

## Goal

Fix the RV64 object-route runtime boundary where
`fragment_for_prepared_variadic_va_start` stores through the prepared
destination `va_list` address register before that address has been
materialized or published.

## Why This Exists

Idea 388 resolved the selected `llvm.va_end.p0` boundary: `va-arg-13.c` now
links without unresolved `llvm.va_end.p0` symbols or relocations. The next
representative failure is a runtime mismatch:

```text
clang_exit=0 c4c_exit=Segmentation fault
```

Step 5 evidence for idea 388 shows the first `va_start` sequence in `test`
stores the overflow-area pointer with `sd t1,0(s1)` before any preceding
instruction materializes the destination `va_list` address into `s1`. The
prepared evidence identifies the helper operand as
`dst_va_list_addr=%lv.state.8:register:reg=s1`.

This is a `va_start` destination address materialization/publication problem,
not a `va_end` lowering issue, not the frame-slot-address call argument route
from idea 386, and not the same-module sret family owned by idea 387.

## In Scope

- Capture the prepared/BIR facts for RV64 `va_start` destination `va_list`
  address ownership in `tests/c/external/gcc_torture/src/va-arg-13.c`.
- Determine whether the destination address should be materialized by the
  `va_start` object fragment itself or published earlier by prepared move /
  address-materialization facts.
- Add focused RV64 backend object-emission coverage proving the destination
  address is available before stores through it.
- Preserve fail-closed behavior for malformed or unsupported `va_start`
  helper shapes.
- Rerun `va-arg-13.c` and record the next boundary if the runtime segfault
  advances.

## Out of Scope

- Reopening `llvm.va_end.p0` no-op lowering from idea 388.
- Reopening frame-slot-address call argument lowering from idea 386.
- Same-module memory-return/sret calls owned by idea 387.
- Broad aggregate `va_arg`, `va_copy`, or unrelated variadic helper rewrites.
- Hard-coding register `s1`, object offsets, or the source testcase name as
  the route condition.

## Acceptance Criteria

- The exact missing destination-address publication/materialization fact is
  identified from prepared/BIR evidence.
- Focused backend coverage proves `fragment_for_prepared_variadic_va_start`
  does not store through an unmaterialized destination `va_list` address.
- `va-arg-13.c` advances past the current runtime segmentation fault, or the
  route records a narrower fail-closed diagnostic with a clear owner.
- Any later boundary is routed to an existing or new idea instead of being
  silently absorbed.

## Closure Notes

Idea 389 is complete. The RV64 object route now materializes the prepared
`va_start` destination `va_list` address before stores through it. Step 5
evidence in `build/agent_state/389_step5_va-arg-13.objdump.log` shows the
representative `test` function now emits:

```text
addi s1,sp,72
addi t1,sp,72
sd t1,0(s1)
```

for the first `va_start` sequence, with the same destination-address
materialization before the later sequence. The representative advances past
the previous segmentation fault and now reaches a later
`[RV64_BACKEND_RUNTIME_MISMATCH]` with `clang_exit=0` and
`c4c_exit=Subprocess aborted`.

The later abort is outside this idea's destination-address owner. It is tracked
separately by
`ideas/open/390_rv64_va_list_value_publication_copy_runtime_abort.md`.

Close gate:

- Fresh build plus backend CTest after log:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
- Regression guard:
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
- Result: PASS, 326/326 before and 326/326 after, no new failures.

## Reviewer Reject Signals

- Reject named-case handling for `va-arg-13.c`, function `test`, register
  `s1`, or object offsets such as `0x7c` / `0xac`.
- Reject fixes that merely reshuffle or suppress the segfault without proving
  the prepared destination `va_list` address is materialized before the store.
- Reject adding fake initialization, linker workarounds, or expectation
  downgrades instead of repairing the `va_start` prepared/object route.
- Reject broad variadic helper rewrites that do not include focused proof for
  the destination-address publication boundary.
- Reject reopening idea 386, 387, or 388 behavior under this owner unless a
  reviewer proves the current failure is caused by one of those closed or
  separate routes.
