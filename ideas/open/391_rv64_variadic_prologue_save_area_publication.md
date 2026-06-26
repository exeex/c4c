# RV64 Variadic Prologue Save-Area Publication

Status: Open
Type: Target lowering follow-up
Parent: `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`
Split From: closure of `ideas/closed/390_rv64_va_list_value_publication_copy_runtime_abort.md`

## Goal

Fix the RV64 object-route boundary where a variadic callee's incoming variadic
argument payload is not published into the backing save area consumed by
`va_start` / `va_arg`, causing
`tests/c/external/gcc_torture/src/va-arg-13.c` to abort after the prepared-call
`va_list` publication/copy route is fixed.

## Why This Exists

Ideas 389 and 390 repaired earlier `va_list` boundaries: `va_start` now
materializes its destination address before stores through it, and later
prepared-call frame-slot-address uses copy the initialized `va_list` pointer
payload into the argument object through explicit publication facts.

Step 5 evidence from idea 390 still leaves the representative failing with:

```text
[RV64_BACKEND_RUNTIME_MISMATCH]
clang_exit=0 c4c_exit=Subprocess aborted
```

The disassembly evidence points to a later owner. The c4c `test` body passes a
va-list pointer object into `dummy`, but the save area reached by that pointer
does not appear to contain the incoming variadic argument payload that clang
saves from `a1=1234` for later `va_arg` consumption. This is a variadic
function prologue / backing save-area publication problem, not another
prepared-call frame-slot-address copy, destination-address materialization, or
`va_end` lowering issue.

## In Scope

- Capture prepared/BIR/object and disassembly evidence for the RV64 variadic
  callee prologue around incoming variadic argument registers and the save area
  later reached by `va_start` / `va_arg`.
- Identify the authoritative prepared facts for publishing incoming variadic
  GPR payloads into the backing save area.
- Add focused RV64 backend coverage for supported variadic prologue save-area
  publication.
- Preserve fail-closed behavior for ambiguous save-area ownership, missing
  variadic payload facts, or unsupported register/save-area shapes.
- Rerun `va-arg-13.c` and route any later boundary instead of expanding this
  idea silently.

## Out of Scope

- Reopening `va_start` destination-address materialization from idea 389.
- Reopening prepared-call frame-slot-address `va_list` value publication/copy
  from idea 390.
- Reopening `llvm.va_end.p0` lowering from idea 388.
- Reopening ordinary frame-slot-address GPR call-argument lowering from idea
  386.
- Same-module memory-return/sret calls owned by idea 387.
- Broad aggregate, generic call ABI, full `va_arg`, or full variadic redesigns
  beyond the RV64 incoming variadic payload save-area publication route.
- Hard-coding `va-arg-13.c`, function `test`, function `dummy`, register
  names, literal stack offsets, or the current abort branch as the route
  condition.

## Acceptance Criteria

- The exact prepared/BIR/object fact gap for incoming variadic payload
  publication into the RV64 backing save area is identified.
- Focused backend coverage proves supported incoming variadic GPR payloads are
  stored into the save area consumed by `va_start` / `va_arg`, or the route
  fails closed with a precise diagnostic owner.
- `va-arg-13.c` advances past the current runtime abort, or the route records
  a narrower later boundary with a clear owner.
- Any later boundary is routed to an existing or new idea instead of being
  silently absorbed.

## Reviewer Reject Signals

- Reject named-case handling for `va-arg-13.c`, `test`, `dummy`, specific
  registers, literal stack offsets, or the current abort branch.
- Reject fixes that only suppress `abort()` or alter expected outcomes without
  proving incoming variadic payloads reach the save area used by `va_arg`.
- Reject changes that treat `va_list` pointer/address materialization as proof
  that the backing payload save area was published.
- Reject broad variadic, aggregate, or call-ABI rewrites that do not include
  focused proof for RV64 incoming variadic payload save-area publication.
- Reject reopening ideas 386, 387, 388, 389, or 390 under this owner unless a
  reviewer proves the current abort is caused by one of those routes.
