# RV64 Function Pointer Local And Indirect Call Flow

## Goal

Repair RV64 prepared lowering for function-address values stored in local
objects, returned function-pointer values, and indirect calls through
materialized function pointers.

## Why This Exists

Idea 312 found function-pointer local residuals during the aggregate/function
pointer sweep, but a narrow repair for only `src/00087.c` would be
testcase-shaped because `src/00124.c` proves the same family continues into
returned function-address values and indirect calls.

## In Scope

- Materializing function addresses as pointer values on RV64.
- Storing and loading function pointers through local scalar or aggregate
  homes.
- Returning function pointer values when the value is consumed by RV64 caller
  code.
- Indirect call emission through loaded function-pointer values.
- Backend tests that cover function-address storage and indirect-call use as a
  coherent feature family.

## Out Of Scope

- General external empty-stub policy.
- Ordinary direct call argument/result lowering already covered by prior RV64
  runtime work.
- Aggregate subobject/byval repair unless needed only to reach a function
  pointer field in a focused test.
- Global data object materialization outside function-symbol address handling.

## Candidate Evidence

- `src/00087.c`: function address stored into a local struct field; first stop
  is function-address local store.
- `src/00124.c`: function pointer value flow spans function-address return and
  indirect call through the returned pointer.

## Acceptance Criteria

- Focused backend tests cover local function-address store/load and an indirect
  call through a loaded function pointer.
- `src/00087.c` and `src/00124.c` either pass qemu or are split with concrete
  evidence showing a non-function-pointer residual.
- RV64 emission uses semantic function-address and indirect-call lowering, not
  hard-coded symbol names or candidate-specific control flow.

## Reviewer Reject Signals

- The route fixes only `00087` by hard-coding `foo`, a field offset, or a local
  aggregate shape while `00124` remains the same function-pointer-use failure.
- A patch stores a placeholder integer or direct-call target instead of a real
  function pointer value.
- Indirect calls are converted into direct calls without proving equivalent
  function-pointer semantics.
- Tests assert only printed assembly text for one symbol and do not exercise
  runtime use through a function pointer.
- External-call or empty-stub policy changes are claimed as local
  function-pointer progress without matching candidate evidence.
