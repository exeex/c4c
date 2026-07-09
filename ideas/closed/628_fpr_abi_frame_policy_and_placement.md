# FPR ABI Frame Policy And Placement

Status: Closed
Type: Implementation
Parent: `ideas/closed/613_abi_call_result_stack_frame_lowering.md`
Related:
- `ideas/closed/613_abi_call_result_stack_frame_lowering.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/high_yield_followup_plan.md`
Owning Layer: FPR ABI policy and prepared frame authority
Queue Order: 28
Prerequisites: floating-point ABI classification and frame planning must expose FPR register/result homes and any required FPR save-slot placements
Estimated Evidence Breadth: FPR ABI/frame rows represented by `src/980605-1.c`, `src/ieee/compare-fp-2.c`, `src/ieee/unsafe-fp-assoc.c`, `src/pr39501.c`, and FPR-heavy stack-frame rows
Proof Surface: prepared FPR call/result/frame facts, FPR frame placement diagnostics, RV64 FPR consumer guard rows

## Goal

Separate FPR ABI and frame-placement work from the scalar GPR ABI route by
publishing explicit floating-point call/result homes and frame save-slot
placements before RV64 object emission consumes them.

## Why This Exists

Idea 613 intentionally focused on ordinary scalar GPR ABI consumers with
complete prepared facts. Its final residual classification found FPR-heavy
call/result and stack-frame rows whose ownership is policy or prepared-frame
authority, not scalar GPR RV64 consumption. Lowering those rows by reusing GPR
assumptions would blur ABI policy and risk testcase-shaped assembly output.

## In Scope

- Classify FPR ABI call arguments, call results, and frame save/restore
  requirements that block RV64 object-route progress.
- Publish explicit prepared FPR register homes, result destinations, widths,
  and frame save-slot placements when upstream authority exists.
- Add focused producer or consumer tests that prove FPR facts are explicit
  before RV64 lowering relies on them.
- Keep diagnostics fail-closed when FPR ABI policy, frame placement, width, or
  result-home authority is missing.

## Out Of Scope

- Scalar GPR call/result transport already handled by idea 613.
- GPR dynamic-frame callee-saved placement covered by idea 626.
- Pointer stack-result policy covered by idea 627.
- Floating comparison semantics, floating casts, runtime mismatch triage,
  local/global producer repair, variadic/library policy, expectation changes,
  unsupported marker changes, allowlists, timeouts, or accounting.
- f128 issues, handle by future project

## Acceptance Criteria

- A refreshed FPR ABI/frame probe identifies shared floating-point authority
  gaps rather than a named-case-only target.
- At least one FPR ABI or FPR frame row exposes explicit prepared FPR homes or
  placements, or is reclassified to a more precise owner with diagnostics.
- RV64 FPR consumers require explicit FPR homes/placements and stay
  fail-closed when those facts are absent or ambiguous.
- Scalar GPR, pointer stack-result, aggregate outgoing-stack, runtime,
  variadic/library, local/global, and unsupported semantic rows remain outside
  this idea.

## Closure Notes

Closed after Step 5 representative reclassification. Prepared-layer coverage
now proves explicit FPR immediate/literal argument homes, result authority, and
callee-saved FPR preservation facts before object emission. RV64 object
emission now admits only explicit F32/F64 immediate FPR call arguments and
register-to-register FPR callee-saved preservation effects, with fail-closed
coverage for missing or mismatched FPR authority.

The five-row representative scan passed `src/ieee/compare-fp-2.c`,
`src/ieee/unsafe-fp-assoc.c`, and `src/pr39501.c`. The remaining failures are
outside this idea: `src/980605-1.c` is blocked by ordinary scalar same-module
call/result lowering for `getval()`, and `src/ieee/unsafe-fp-assoc-1.c` is
blocked by prepared move bundle fan-in to one stack destination.

## Reviewer Reject Signals

- Reject treating FPR rows as scalar GPR rows with different register names.
- Reject testcase-shaped shortcuts for the representative source files named
  in this idea.
- Reject RV64 inference of FPR homes or save slots from final assembly layout,
  source filenames, frame size, or register order instead of explicit prepared
  facts.
- Reject expectation, unsupported-marker, allowlist, timeout, runtime, or
  accounting changes as capability progress.
- Reject broad floating semantic rewrites or local/global producer repairs
  that are not required to publish FPR ABI/frame authority.
