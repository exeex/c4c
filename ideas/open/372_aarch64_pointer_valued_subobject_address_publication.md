# AArch64 Pointer-Valued Subobject Address Publication

Status: Open
Created: 2026-05-21
Split From: ideas/open/295_backend_regex_failure_family_inventory.md

## Goal

Repair AArch64 lowering where an address-of global object or subobject value
assigned to a scalar pointer local is not published to the pointer home
consumed by later dereferences.

## Why This Exists

The post-371 backend inventory selected `00163` as the smallest concrete next
first bad fact. Source `tests/c/external/c-testsuite/src/00163.c` assigns a
local pointer `b` first to `&a`, then later to `&(bolshevic.b)`. The final
`*b` should load the global struct member value `34`, but generated AArch64
keeps using the old local pointer home for `&a`, so it prints `42`.

This is adjacent to older closed address owners, but the current evidence is
not a count-only reopen:

- closed idea 294 covered broad pointer-derived lvalue authority;
- closed idea 355 covered address-valued memory and call-argument
  publication;
- the current first bad fact is scalar pointer-local publication of a global
  member address assigned by `b = &(bolshevic.b)`, followed by a dereference
  through that scalar pointer local.

## In Scope

- Localize how address-of global objects and subobjects are represented when
  assigned to scalar pointer locals.
- Trace the path from semantic/prepared address value through stack home or
  register publication to the later dereference consumer.
- Repair the general AArch64 publication path so pointer locals receive the
  current address-valued assignment instead of retaining an older pointer
  home.
- Add focused backend coverage for scalar pointer-local assignment from a
  global/member address and later dereference.
- Prove `c_testsuite_aarch64_backend_src_00163_c` advances past the stale
  pointer-home first bad fact or passes.

## Out Of Scope

- External/libc call return publication such as `00187`.
- Static local/global selected array lane store/readback such as `00182`.
- Aggregate initializer/layout and compound literal publication such as
  `00205` and `00216`.
- Scalar FP expression/constant publication, unsigned enum bit-field layout,
  timeout-specific work, runner behavior, proof-log policy, CTest
  registration, or expectation changes.
- Reopening closed ideas 294 or 355 unless fresh localization proves the
  current first bad fact is exactly inside their archived closure boundary.
- Fixing only `00163`, one variable name, one global symbol, one struct field,
  one stack offset, one register, or one emitted instruction sequence.

## Acceptance Criteria

- The stale pointer-home first bad fact is localized to a concrete semantic,
  prepared, stack-home, MIR, or AArch64 publication boundary.
- Focused backend coverage guards scalar pointer-local assignment from a
  global/member address followed by dereference without relying only on
  `00163`.
- `c_testsuite_aarch64_backend_src_00163_c` no longer fails because `b =
  &(bolshevic.b)` leaves the later `*b` consuming the old `&a` home.
- If `00163` still fails, `todo.md` records the new first bad fact and whether
  it remains in this owner or needs a lifecycle split.
- The supervisor-selected proof scope introduces no new backend-regex
  failures and does not regress closed address/lvalue publication behavior.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00163`, `bolshevic`, `b`, `a`, one struct field, one stack
  offset, one register, or one emitted instruction sequence instead of
  repairing scalar pointer-local address publication generally;
- weakens expectations, unsupported classifications, runner behavior, timeout
  policy, proof-log policy, CTest registration, or external test contracts;
- claims progress through helper renames, emitted-text reshuffling, or
  classification-only notes while later dereferences can still consume stale
  pointer homes after an address-valued assignment;
- reopens or rewrites closed ideas 294 or 355 instead of using fresh evidence
  to drive this focused route;
- folds external-call return publication, selected array store/readback,
  aggregate initializer/layout, scalar FP, bit-field, or timeout work into
  this route without a fresh first-bad-fact handoff;
- proves only the external representative while leaving focused pointer-local
  address assignment behavior unguarded.
