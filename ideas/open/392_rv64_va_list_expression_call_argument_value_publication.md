# RV64 Va List Expression Call-Argument Value Publication

Status: Open
Type: Target lowering follow-up
Parent: `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`
Split From: `ideas/open/391_rv64_variadic_prologue_save_area_publication.md`

## Goal

Fix the RV64 object-route boundary where a `va_list` expression passed as a
call argument publishes the address of the `va_list` storage object instead of
the initialized save-area pointer payload stored in that object.

## Why This Exists

Idea 391 implemented and proved the variadic prologue save-area publication:
`test` now stores incoming variadic GPR payloads into the prepared backing area
before `va_start` exposes it. The representative
`tests/c/external/gcc_torture/src/va-arg-13.c` still aborts, but Step 5
evidence shows a later owner:

- `va_start` initializes the local `va_list` slot with the save-area pointer
  payload.
- The first `dummy` call builds the parameter object from the `va_list` slot
  address, not from the initialized pointer payload.
- `dummy` therefore reads through the pointer slot object instead of the saved
  variadic payload and reaches `abort()`.

This is a `va_list` expression / call-argument value publication problem, not
additional RV64 variadic prologue save-area publication.

## In Scope

- Capture prepared/BIR/object evidence for the local `va_list` object, its
  initialized pointer payload, and the call argument object that should receive
  that payload.
- Identify the authoritative prepared facts needed to copy the initialized
  `va_list` value into call-argument storage when the source expression is a
  `va_list` object.
- Add focused RV64 backend coverage for accepted value publication and
  adjacent fail-closed shapes.
- Implement the narrow publication route using explicit prepared facts, with
  precise rejection for missing, duplicate, ambiguous, or mismatched facts.
- Rerun `va-arg-13.c` and route any later boundary separately.

## Out of Scope

- Reopening RV64 variadic prologue save-area publication from idea 391.
- Reopening `va_start` destination-address materialization from idea 389.
- Reopening the prepared-call frame-slot-address publication route from idea
  390 except where evidence proves this expression/value path needs a distinct
  prepared fact or guard.
- Reopening `llvm.va_end.p0` lowering from idea 388.
- Reopening ordinary frame-slot-address GPR call-argument lowering from idea
  386.
- Broad variadic, aggregate, generic call ABI, or full `va_arg` redesigns.
- Hard-coding `va-arg-13.c`, `test`, `dummy`, concrete stack offsets, or the
  current abort branch as the route condition.

## Acceptance Criteria

- The exact prepared/BIR/object fact gap for copying the initialized `va_list`
  pointer payload into a call-argument object is identified.
- Focused backend coverage proves the accepted value-publication route and
  fail-closed variants for absent, duplicate, ambiguous, or mismatched
  publication facts.
- The implementation copies the initialized `va_list` payload value, not the
  source object's address, into the argument object for supported shapes.
- `va-arg-13.c` advances past the current abort, or any remaining later
  boundary is recorded with a clear owner and split instead of silently
  expanding this idea.

## Reviewer Reject Signals

- Reject named-case handling for `va-arg-13.c`, `test`, `dummy`, literal stack
  offsets, or the current abort branch.
- Reject fixes that treat the address of the `va_list` storage object as the
  value payload passed to `dummy`.
- Reject expectation rewrites, unsupported downgrades, or abort suppression
  claimed as progress without proving payload publication into the call
  argument object.
- Reject broad variadic, aggregate, or call-ABI rewrites that do not include
  focused proof for `va_list` expression call-argument value publication.
- Reject changes that reopen idea 391's save-area publication route unless
  evidence proves the saved incoming variadic payload is no longer present.
