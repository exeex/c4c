# RV64 Object Route Variadic `va_end` Boundary

Status: Closed
Type: Target lowering follow-up
Parent: `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`
Split From: closure of `ideas/closed/386_rv64_object_route_unsupported_instruction_fragment.md`

## Goal

Decide and implement the RV64 object-route handling for prepared
`llvm.va_end.p0` call boundaries that remain after frame-slot-address call
argument lowering has advanced `tests/c/external/gcc_torture/src/va-arg-13.c`
past c4c object compilation.

## Why This Exists

Idea 386 completed the selected frame-slot-address GPR call-argument lowering
for same-module `dummy(ptr %t7)` and `dummy(ptr %t14)` calls. After that
support, `va-arg-13.c` no longer fails during c4c RV64 object emission. The
next observed boundary is link failure from unresolved prepared calls to
`llvm.va_end.p0` at `.text+0xac` and `.text+0xdc` in `test`.

That boundary is a variadic helper/intrinsic handling issue, not part of the
frame-slot-address call-argument route and not part of idea 387's same-module
sret call family.

## In Scope

- Capture the prepared/BIR call shape for `llvm.va_end.p0` in the RV64 object
  route.
- Decide whether RV64 object emission should erase `va_end`, lower it to a
  no-op fragment, or route it through an existing prepared variadic-helper
  abstraction.
- Add focused backend/object-route coverage for the selected `va_end` behavior.
- Rerun `tests/c/external/gcc_torture/src/va-arg-13.c` and record the next
  boundary after `llvm.va_end.p0` is handled.

## Out of Scope

- Reopening frame-slot-address GPR call-argument lowering from idea 386.
- Same-module memory-return/sret calls owned by idea 387.
- Broad variadic `va_arg` aggregate helper lowering unrelated to `va_end`.
- Linker hacks that hide unresolved symbols without defining the intended
  object-route behavior.

## Acceptance Criteria

- The exact `llvm.va_end.p0` prepared call boundary is identified from
  prepared/BIR evidence.
- Focused coverage proves the selected RV64 object-route behavior for `va_end`.
- `va-arg-13.c` advances past the unresolved `llvm.va_end.p0` link failure, or
  the route records a narrower fail-closed diagnostic with a clear owner.
- Any later boundary is routed to an existing or new idea instead of being
  silently absorbed.

## Closure Note

Closed after implementing exact-guarded RV64 object-route no-op lowering for
direct extern `llvm.va_end.p0` calls. Focused backend coverage proved the
selected `va_end` behavior and fail-closed adjacent shapes.

`tests/c/external/gcc_torture/src/va-arg-13.c` now advances past the previous
unresolved `llvm.va_end.p0` link boundary; readelf evidence contains no
`llvm.va_end.p0` symbol or relocation. The next boundary is a runtime
segmentation fault caused by RV64 `va_start` storing through the prepared
destination `va_list` address register before that address is materialized.
That follow-up is tracked separately in
`ideas/open/389_rv64_va_start_destination_va_list_address_publication.md`.

## Reviewer Reject Signals

- Reject named-case handling for `va-arg-13.c`, `test`, or hard-coded text
  offsets such as `.text+0xac`.
- Reject adding a fake external symbol or linker workaround that merely hides
  unresolved `llvm.va_end.p0` without proving the prepared variadic-helper
  semantics.
- Reject expectation downgrades, allowlist changes, or diagnostic renames
  claimed as capability progress.
- Reject reopening idea 386's frame-slot-address argument path or idea 387's
  sret admission gate under this owner.
- Reject broad variadic helper rewrites that do not include focused proof for
  the `llvm.va_end.p0` boundary.
