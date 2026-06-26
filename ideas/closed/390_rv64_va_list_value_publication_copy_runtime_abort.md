# RV64 `va_list` Value Publication/Copy Runtime Abort Boundary

Status: Closed
Type: Target lowering follow-up
Parent: `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`
Split From: closure of `ideas/closed/389_rv64_va_start_destination_va_list_address_publication.md`
Closed By: prepared-call publication/copy implementation plus Step 5 routing evidence

## Goal

Fix the RV64 object-route boundary where a `va_list` value initialized by
`va_start` is not the value later observed by `va_list` copy/call-argument
uses, causing `tests/c/external/gcc_torture/src/va-arg-13.c` to abort at
runtime after the destination-address materialization issue is fixed.

## Why This Exists

Idea 389 fixed the previous crash: `fragment_for_prepared_variadic_va_start`
now materializes the prepared destination `va_list` address before stores
through it. The representative no longer segfaults. Step 5 evidence shows the
first `va_start` sequence now emits:

```text
addi s1,sp,72
addi t1,sp,72
sd t1,0(s1)
```

The next failure is later and different:

```text
[RV64_BACKEND_RUNTIME_MISMATCH]
clang_exit=0
c4c_exit=Subprocess aborted
```

The current evidence points at a `va_list` value publication/copy boundary:
after `va_start` writes through the prepared destination address, later
`dummy(statep->ap)` / `dummy(state.ap)` call-argument setup still appears to
copy from the ordinary `state.ap` storage path instead of from the newly
initialized prepared `va_start` destination value. `dummy` then reads the
wrong `va_list` contents and reaches the testcase `abort()` path.

This is not the destination-address validity problem closed by idea 389. It is
also not the `llvm.va_end.p0` no-op route from idea 388, the frame-slot-address
GPR call-argument route from idea 386, or the same-module sret family owned by
idea 387.

## In Scope

- Capture prepared/BIR/object evidence for the `va_list` value produced by
  `va_start` and the later `dummy(statep->ap)` / `dummy(state.ap)` call
  argument values in `va-arg-13.c`.
- Determine whether the initialized `va_list` value must be published back to
  the ordinary aggregate/member storage path, copied from the prepared
  destination slot, or represented through a prepared value alias.
- Add focused RV64 backend object-emission coverage for `va_list` value
  publication/copy after `va_start`.
- Preserve fail-closed behavior for ambiguous `va_list` copy/publication
  shapes.
- Rerun `va-arg-13.c` and record the next boundary if the runtime abort
  advances.

## Out of Scope

- Reopening `va_start` destination-address materialization from idea 389.
- Reopening `llvm.va_end.p0` lowering from idea 388.
- Reopening frame-slot-address call-argument lowering from idea 386.
- Same-module memory-return/sret calls owned by idea 387.
- Broad aggregate rewrite, full `va_arg` redesign, or generic call ABI changes
  not required to publish/copy the initialized `va_list` value.
- Hard-coding `va-arg-13.c`, function `test`, function `dummy`, register
  names, stack offsets, or the current abort branch as the route condition.

## Acceptance Criteria

- The exact prepared/BIR fact gap for the initialized `va_list` value and its
  later copy/call-argument use is identified.
- Focused backend coverage proves the value observed by the later `va_list`
  call argument comes from the initialized `va_start` destination value, or the
  route fails closed with a precise diagnostic owner.
- `va-arg-13.c` advances past the current runtime abort, or the route records
  a narrower fail-closed diagnostic with a clear owner.
- Any later boundary is routed to an existing or new idea instead of being
  silently absorbed.

## Closure Notes

Idea 390 is complete as the prepared-call `va_list` value publication/copy
owner. The implemented RV64 object route requires the missing-publication need
plus exactly one matching `StoreLocalPublication` fact, then stores the
initialized pointer payload into the frame-slot-address argument object before
materializing that object's address into the ABI register. Missing, duplicate,
or mismatched publication facts fail closed.

Focused backend coverage now records the missing-publication need without
treating address materialization as payload authority, and covers malformed
publication shapes including duplicate exact address materialization facts and
non-frame-slot materialization kinds.

Step 5 reran
`tests/c/external/gcc_torture/src/va-arg-13.c`. The representative still
returns:

```text
[RV64_BACKEND_RUNTIME_MISMATCH]
clang_exit=0 c4c_exit=Subprocess aborted
```

The later abort is outside this idea's prepared-call publication/copy owner.
Evidence points to RV64 variadic function prologue / `va_start` backing
register-save-area publication for incoming varargs payloads: `dummy` receives
a va-list pointer object, but the pointed-to save area is not known to contain
the expected incoming `a1=1234` payload. That boundary is tracked separately by
`ideas/open/391_rv64_variadic_prologue_save_area_publication.md`.

Close gate:

- Fresh build plus backend CTest after log:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
- Regression guard:
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
- Result: PASS, 326/326 before and 326/326 after, no new failures.

## Reviewer Reject Signals

- Reject named-case handling for `va-arg-13.c`, `test`, `dummy`, specific
  registers, literal stack offsets, or the current abort branch.
- Reject fixes that only suppress `abort()` or alter expected outcomes without
  proving the initialized `va_list` value reaches the later call argument.
- Reject copying from the ordinary aggregate/member path when prepared evidence
  shows the authoritative initialized value lives in the `va_start`
  destination slot, unless the route first publishes that value coherently.
- Reject broad variadic, aggregate, or call-ABI rewrites that do not include
  focused proof for the `va_list` value publication/copy boundary.
- Reject reopening ideas 386, 387, 388, or 389 under this owner unless a
  reviewer proves the current abort is caused by one of those routes.
