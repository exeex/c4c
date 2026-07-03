# RV64 F64 Global-Memory Consumption

Status: Closed
Type: RV64 object-route implementation
Parent: `ideas/open/420_rv64_gcc_torture_post_contract_umbrella.md`
Derived From: `ideas/closed/548_prepared_global_stack_frame_infrastructure_review.md`
Owning Layer: RV64 global-memory object-route consumption

## Goal

Support non-F128 floating-point prepared global-memory accesses in the RV64
object route, starting with `double` / `F64` global loads where prepared facts
already reach the target consumer.

## Why This Exists

Step 2 of the prepared infrastructure review classified `src/20001121-1.c` as
an RV64 consumer gap. Prepared global-memory facts are present enough for the
object route to classify the operation, but the RV64 scalar global-memory gate
only accepts 1-, 2-, 4-, and 8-byte integer or pointer accesses.

## Representative Rows

- `src/20001121-1.c`

Evidence:

- `build/agent_state/548_step2_global_data_classification/classification.md`
- `build/rv64_gcc_c_torture_backend/src_20001121-1.c/case.log`

Observed diagnostic:

```text
unsupported_global_data: RV64 object route supports only 1-, 2-, 4-, and
8-byte prepared global memory accesses
```

## In Scope

- Add RV64 object-route support for prepared global `F64` loads or the minimal
  non-F128 floating global-memory lane required by the representative.
- Preserve the prepared/RV64 boundary: require prepared facts to exist before
  target lowering consumes the access.
- Add focused target or backend tests for prepared `F64` global-memory
  consumption.
- Re-run the representative row and record the first new owner if it moves to a
  later unsupported bucket.

## Out Of Scope

- Prepared object-data production or zero-fill contract repair.
- F128, long-double, external soft-float, or quarantine policy work.
- General global initializer shape production.
- Stack-frame lowering or callee-saved slot support.
- Changing expectations, unsupported markers, allowlists, or pass/fail counts.

## Acceptance Criteria

- RV64 consumes a prepared `double` / `F64` global-memory access without the
  current scalar global-memory type-gate diagnostic.
- Tests cover the target type/fact path, not only the representative filename.
- Prepared fact absence remains a rejection path; RV64 does not infer missing
  symbols, object data, or relocation authority.
- Any residual failure for `src/20001121-1.c` is documented with a concrete
  downstream owner.

## Closure Notes

Closed after `e2d5d1ec3 consume prepared F64 global loads`.

The RV64 object route now consumes prepared `double` / `F64` global loads from
explicit prepared global-symbol access facts and keeps missing prepared access
facts fail-closed. Backend regression proof passed with `345/345` before and
after, and the representative row moved off the old scalar global-memory
type-gate diagnostic.

The remaining representative failure is downstream:

```text
unsupported_terminator_fragment: BIR terminator requires unsupported RV64 object lowering
```

No duplicate follow-up idea was created because
`ideas/open/549_rv64_runtime_and_no_diagnostic_triage.md` already covers
residual first-owner reconstruction and triage.

## Reviewer Reject Signals

- Reject changes that fabricate missing prepared global facts in RV64 lowering
  or bypass the prepared object route.
- Reject mixing prepared zero-fill object-data repair with F64 target
  consumption in one implementation slice.
- Reject F128 or long-double helper work under this non-F128 global-memory
  idea.
- Reject filename-shaped special cases for `src/20001121-1.c` or diagnostic
  string filtering as lowering.
- Reject expectation rewrites, unsupported downgrades, allowlist filtering, or
  pass/fail accounting changes as progress.
- Reject retaining the exact old scalar global-memory type rejection behind a
  renamed target helper.
