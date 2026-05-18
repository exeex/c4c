# AArch64 Stack Frame SP Alignment

Status: Open
Created: 2026-05-18
Source Inventory: ideas/open/284_aarch64_c_testsuite_failure_family_inventory.md

## Intent

Repair AArch64 backend frame sizing so every generated function preserves the
ABI-required 16-byte stack-pointer alignment across stack allocation, local
slot access, calls, and returns.

## Why This Exists

The refreshed AArch64 backend c-testsuite inventory found a 28-case `Bus
error` runtime cluster after prior focused repairs completed. The smallest
representative, `src/00004.c`, uses local pointer state and generated
assembly shows:

```asm
sub sp, sp, #24
```

That frame size leaves `sp` misaligned for AArch64. Nearby pointer and array
local cases such as `src/00005.c`, `src/00013.c`, `src/00014.c`,
`src/00015.c`, and `src/00016.c` fail in the same bus-error cluster, making
SP/frame alignment a tractable semantic owner to repair before treating these
as broad pointer-semantics failures.

## In Scope

- AArch64 backend frame-size computation and prologue/epilogue stack
  adjustment.
- Local stack-slot layout when padding is needed to keep `sp` 16-byte aligned.
- Addressing of local scalar, pointer, and array slots after frame-size
  rounding.
- Direct-call safety only as it relates to preserving aligned `sp` at call
  boundaries.
- Focused proof starting with `src/00004.c`, then nearby bus-error local
  pointer/array cases `src/00005.c`, `src/00013.c`, `src/00014.c`,
  `src/00015.c`, and `src/00016.c`.

## Out of Scope

- Rewriting runner behavior, expected outputs, allowlists, unsupported
  classifications, timeout policy, or CTest registration.
- Parser or sema changes.
- Named-case shortcuts for `00004.c`, exact stack sizes, exact local variable
  spellings, or emitted instruction-text matching.
- Broad pointer semantics, array semantics, struct layout, function-pointer
  semantics, aggregate ABI, variadic ABI, or libc behavior unless a remaining
  failure is proven to be directly downstream of the same SP/frame-alignment
  defect.
- Timeout-sensitive cases such as `src/00132.c`, `src/00173.c`, and
  `src/00220.c`.
- The compare/branch printer/lowering family and other compile-stage backend
  gaps from the inventory.

## Acceptance Criteria

- `src/00004.c` no longer fails from AArch64 stack-pointer misalignment, and
  generated function frames keep `sp` 16-byte aligned.
- The repair generalizes across nearby local pointer/array bus-error cases:
  `src/00005.c`, `src/00013.c`, `src/00014.c`, `src/00015.c`, and
  `src/00016.c`, subject to unrelated remaining blockers being documented.
- Any remaining bus-error cases are inspected enough to separate true
  SP/frame-alignment defects from distinct pointer, aggregate, function
  pointer, or local-state owners.
- No progress is claimed through test expectation rewrites, allowlist changes,
  unsupported classifications, runner changes, timeout changes, or CTest
  contract changes.

## Reviewer Reject Signals

Reject the route if it:

- fixes only `src/00004.c`, one local variable layout, one exact frame size, or
  one emitted instruction spelling instead of enforcing semantic 16-byte
  AArch64 `sp` alignment for generated frames;
- changes local-slot offsets or frame padding in a way that preserves aligned
  `sp` but breaks existing stack local loads/stores or call boundaries;
- claims backend progress through expected-output rewrites, allowlist changes,
  unsupported classifications, runner changes, timeout changes, or CTest
  contract changes;
- broadens into parser/sema work, broad pointer semantics, aggregate ABI,
  function-pointer semantics, libc behavior, or timeout-case repair before the
  local pointer/array alignment family is proven;
- treats all bus-error cases as the same owner without separating remaining
  non-alignment failures after the minimal alignment repair;
- renames frame helpers or reorganizes emission while preserving the old
  failure mode where generated AArch64 functions allocate stack frames whose
  `sp` adjustment is not a multiple of 16.
