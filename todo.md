Status: Active
Source Idea Path: ideas/open/388_rv64_object_route_variadic_va_end_boundary.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Capture The Prepared `va_end` Boundary

# Current Packet

## Just Finished

Lifecycle activation created the active runbook for Step 1 of
`ideas/open/388_rv64_object_route_variadic_va_end_boundary.md`.

## Suggested Next

Execute Step 1: capture the prepared/BIR shape for the unresolved
`llvm.va_end.p0` boundary in `va-arg-13.c` and identify the likely owned RV64
object-emission helper.

## Watchouts

- Keep idea 386 closed; do not reopen frame-slot-address call argument
  lowering under this plan.
- Keep idea 387 sret work out of scope.
- Do not hide unresolved `llvm.va_end.p0` with fake symbols or linker
  workarounds.

## Proof

Lifecycle-only activation. No build or test proof was required.
