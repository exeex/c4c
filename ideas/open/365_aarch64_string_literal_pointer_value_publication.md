# AArch64 String Literal Pointer Value Publication

Status: Open
Created: 2026-05-21
Split From: ideas/open/356_semantic_bir_pointer_derived_string_loads.md

## Goal

Publish string literal addresses as pointer values before local storage,
prepared-addressing, or generated AArch64 consumers follow those pointers.

## Why This Exists

Idea 356 repaired the semantic BIR bug where dynamic pointer-derived byte loads
from string data collapsed to fixed global-byte loads. After that repair,
`00173` advanced from the old runtime output mismatch (`copied string is`) to
`[RUNTIME_NONZERO] exit=Segmentation fault`.

The new first bad fact is earlier than the repaired dynamic byte loads:
`char *a = "hello"` stores `%t2` into `%lv.a`, but `%t2` never gets a `.str0`
string-address materialization. Prepared BIR treats `%t2` as an ordinary frame
spill (`slot#26+stack72`), and generated AArch64 emits
`add x9, sp, #72; str x9, [sp, #16]`. Runtime pointer consumers therefore
follow `sp+72` instead of the `.str0` address. The dynamic `*b` and `*src`
loads are now doing the right kind of load through the current pointer value;
the pointer value they receive is wrong.

This is downstream AArch64/prepared-addressing pointer-address publication
work, not semantic dynamic pointer-derived byte-load preservation.

## In Scope

- Localize where a string literal address assigned to a pointer carrier loses
  its global-symbol address before `StoreLocal` or equivalent local-pointer
  publication.
- Repair the general path that materializes a string literal or global data
  address as a pointer value for local storage and later dynamic pointer
  consumers.
- Keep semantic BIR dynamic pointer-derived byte loads from idea 356 dynamic;
  do not reintroduce fixed `LoadGlobalInst @.str0` byte loads for `*b`,
  `*src`, or equivalent pointer-derived consumers.
- Add focused backend or prepared-route coverage for a string-literal pointer
  value stored into a local pointer and later consumed dynamically.
- Prove `c_testsuite_aarch64_backend_src_00173_c` advances beyond the current
  segmentation fault or passes, while preserving the focused idea 356
  semantic/backend contracts.

## Out Of Scope

- Reopening semantic-BIR dynamic pointer-derived byte-load preservation from
  idea 356 except to keep it stable.
- Reopening direct external-call string/global argument lowering from idea 287
  unless fresh evidence shows this local pointer publication path shares the
  same owner and the route is documented.
- Reopening AArch64 address-valued memory or call-argument publication from
  idea 355 without fresh first-bad-fact evidence.
- Same-module recursive pointer-formal/callee-saved-home publication for
  `00181`.
- Frontend admission work for `00005`.
- ABI composite/byval/HFA/f128, variadic floating, dynamic stack, unrelated
  AArch64 call publication work, runner behavior, timeout policy, CTest
  registration, proof-log handling, expectation changes, or unsupported
  classification changes.

## Acceptance Criteria

- The first bad publication boundary is localized with expected `.str0`
  pointer-address materialization, actual stack-spill publication, producer
  carrier, local pointer store, and first runtime consumer.
- Focused coverage fails before the repair and passes after it for a string
  literal or global data address flowing as a pointer value into a local and
  then into a dynamic pointer consumer.
- `00173` advances beyond the current segmentation fault or passes without
  expectation, timeout, runner, CTest-registration, proof-log, or
  filename-specific changes.
- Focused idea 356 coverage remains stable, proving dynamic pointer-derived
  byte loads still use the current pointer value as the memory base.
- Existing direct fixed global/string byte-load behavior remains distinct from
  local pointer-address publication.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00173`, one string spelling, `.str0`, one local slot, one
  stack offset, one ABI register, one loop body, or one emitted instruction
  sequence instead of repairing general string/global address pointer-value
  publication;
- weakens expectations, unsupported classifications, runner behavior, timeout
  policy, proof-log policy, CTest registration, or external test contracts;
- claims progress through helper renames, emitted-text rewrites, or
  classification notes while `char *p = "literal"` still publishes a stack
  spill address instead of the literal/global address;
- reintroduces fixed global-byte loads for dynamic `*p` / `*src` consumers
  that idea 356 repaired;
- broadens into unrelated external-call argument lowering, recursive formal
  homes, frontend admission, ABI composite, variadic/floating, or dynamic
  stack work without fresh first-bad-fact evidence and a lifecycle split;
- hides the exact old `sp+offset` pointer publication failure behind a new
  abstraction name without proving the published pointer value is the
  literal/global address.
