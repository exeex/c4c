# Prepared `va_start` Destination Address Helper Operand Publication

Status: Closed
Type: Producer-side follow-up repair idea
Parent: `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`
Split From: `ideas/closed/398_rv64_object_route_stack_frame_and_param_home_edges.md`

## Goal

Publish explicit prepared helper operand metadata for the destination
`va_list` address consumed by RV64 `va_start` helper lowering.

## Why This Exists

The 2026-06-26 Step 1 refresh for idea 398 found that the RV64 object-route
stack-frame, callee-saved save-slot, parameter-home, and function-admission
diagnostics are stale or passing for the current representatives.

The remaining `tests/c/external/gcc_torture/src/va-arg-21.c` evidence is a
producer-side prepared metadata gap. The case compiles and dumps prepared BIR,
and the prepared facts include:

- `variadic_entry=yes`
- explicit `va_list_layout`
- explicit overflow-area base slot and stack offset
- incoming variadic GPR publications from `a1` through `a7`
- `helper kind=va_start`
- prepared `llvm.va_start.p0` call sites and call-argument routes

But the prepared dump still records:

```text
missing fact=helper_operand_homes.va_start.destination_va_list_address
```

That is not an RV64 object-emitter stack-frame or parameter-home packet. The
producer that owns variadic helper operand homes must publish the destination
`va_list` address fact when the destination object address is available, or
produce a precise unsupported producer diagnostic when it is not.

## In Scope

- Classify the `va-arg-21.c` prepared facts for the missing
  `helper_operand_homes.va_start.destination_va_list_address` publication.
- Identify the producer authority that knows the destination `va_list` object
  address for `llvm.va_start.p0`.
- Publish explicit prepared helper operand metadata for supported destination
  address homes, including non-GPR/frame-slot-backed address shapes when the
  existing prepared facts make them unambiguous.
- Preserve precise rejection for unsupported or ambiguous destination address
  shapes.
- Prove `va-arg-21.c` and focused backend/prepared contract coverage.

## Out Of Scope

- Reopening RV64 object-route stack-frame, callee-saved save-slot, or generic
  parameter-home lowering from idea 398.
- Reopening closed target-side `va_start` helper lowering or destination
  address materialization ideas unless fresh evidence proves their exact
  failure mode regressed.
- Teaching RV64 object emission to infer the destination `va_list` address
  from source, BIR instruction shape, or stack layout when prepared metadata
  is missing.
- Broad scalar `va_arg`, aggregate `va_arg`, `va_copy`, or external variadic
  call support unrelated to destination-address helper operand publication.
- Expectation rewrites, allowlist filtering, unsupported downgrades, or
  testcase-specific shortcuts.

## Acceptance

- `src/va-arg-21.c` prepared output no longer records missing
  `helper_operand_homes.va_start.destination_va_list_address` when the
  destination address is supportable.
- The prepared helper metadata identifies the destination `va_list` address
  explicitly enough for RV64 helper lowering to consume without reconstructing
  producer semantics.
- Unsupported destination address shapes remain rejected with a precise
  producer-side diagnostic.
- Backend/prepared proof shows no regression in existing variadic entry,
  `va_start`, frame-stack-call, and RV64 object-emission coverage.

## Reviewer Reject Signals

- Reject filename-specific handling for `va-arg-21.c` or source-shape matching
  around its local `va_list`.
- Reject RV64 object-emitter code that synthesizes the missing destination
  address instead of consuming a producer-published helper operand fact.
- Reject merely renaming the missing fact while
  `helper_operand_homes.va_start.destination_va_list_address` remains absent
  for the supportable representative.
- Reject broad variadic helper rewrites that do not directly prove destination
  address helper operand publication.
- Reject expectation downgrades, unsupported markers, allowlist filtering, or
  diagnostic-only churn claimed as capability progress.

## Lifecycle Notes

- 2026-06-27: Closed after producer-side publication repair in
  `src/backend/prealloc/variadic_entry_plans.cpp` made RV64 `va_start` helper
  operand authority keep `destination_va_list_address` for complete
  stack-slot/frame-slot homes as well as register homes, while preserving
  fail-closed behavior for unsupported or incomplete homes.
- 2026-06-27: Focused coverage in
  `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp`
  proves a frame-slot-backed `va_start` destination address path while
  preserving the existing register-backed path.
- 2026-06-27: Fresh `va-arg-21.c` prepared proof publishes both helper operand
  address homes:
  `dst_va_list_addr=%t1:stack_slot:slot=#17:offset=136` and
  `dst_va_list_addr=%t6:stack_slot:slot=#18:offset=144`. The old
  `missing fact=helper_operand_homes.va_start.destination_va_list_address`
  line is absent, and `build/c4cll --target riscv64-linux-gnu
  tests/c/external/gcc_torture/src/va-arg-21.c` completes.
- 2026-06-27: Close gate passed with the backend regression guard over
  `ctest --test-dir build -j --output-on-failure -R '^backend_'`. The
  rolled-forward `test_before.log` and regenerated `test_after.log` both
  reported 326/326 passing backend tests with no new failures.
