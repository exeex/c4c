# AArch64 External Call Symbol Home Publication

Status: Active
Created: 2026-05-21
Split From: ideas/closed/348_aarch64_indexed_aggregate_address_writeback.md

## Goal

Repair AArch64 generated-code publication for values that must be available in
symbol, local, or argument homes before an external or direct call consumes
them, starting from the current `00187` segmentation fault.

## Why This Exists

Idea 348 completed the indexed aggregate selected-snapshot/writeback route:
`00130`, `00176`, and `00195` now pass in the focused proof subset. The only
remaining red test in that subset is
`c_testsuite_aarch64_backend_src_00187_c`, which still exits with
`RUNTIME_NONZERO` from a segmentation fault.

The latest classification treats that failure as outside indexed aggregate
selected-address/writeback. The residual is external call argument/symbol home
publication: generated AArch64 must publish the value or address expected by a
call boundary from its prepared home into the actual ABI argument, symbol, or
local storage location before the callee or runtime path dereferences it.

## In Scope

- Localize the current `00187` segfault to the concrete AArch64 publication
  boundary for call operands, symbol homes, local homes, or preserved stack
  homes.
- Repair publication from prepared MIR/BIR homes into the AAPCS64 call
  argument registers or stack slots when a direct or external call consumes
  the value.
- Repair symbol/local home publication when the call path needs an address or
  value that currently remains only in a stale stack slot, register, or fixed
  symbol snapshot.
- Add focused backend coverage for the localized call/symbol-home shape before
  using `00187` as external proof.
- Preserve the completed indexed aggregate selected-address/writeback behavior
  from idea 348.

## Out Of Scope

- Dynamic indexed aggregate selected-address/writeback repairs already closed
  under idea 348.
- Recursive post-call argument preservation, block label emission ordering,
  formal-to-local frame-slot publication, unsigned div/rem producer
  publication, scalar cast source publication, return-result publication,
  variadic `va_arg`, byval aggregate lane publication, HFA/floating aggregate
  publication, static aggregate initializer materialization, semantic
  admission, runner behavior, expectation changes, unsupported
  classifications, CTest registration, timeout policy, or proof-log behavior
  unless fresh generated-code evidence reaches those exact owners.
- Fixing only `00187`, one libc/external callee, one symbol name, one stack
  offset, one register, one source buffer, or one emitted instruction
  neighborhood.

## Acceptance Criteria

- The first bad fact for the current `00187` segfault is localized to a
  concrete call operand, symbol home, local home, preserved home, or
  argument-home publication boundary.
- Focused backend coverage fails without the repair and passes with it for
  the localized publication shape.
- `c_testsuite_aarch64_backend_src_00187_c` advances past the current
  segmentation fault or passes; any new first bad fact is reclassified before
  this idea is closed.
- Guardrails for completed adjacent owners remain stable, including the
  supervisor-selected `00130`, `00176`, `00195`, focused call-publication
  tests, selected-address/writeback tests, and backend guard bucket.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00187`, one source function, one external callee, one symbol,
  one local variable, one stack offset, one register, or one emitted call
  neighborhood instead of repairing general call/symbol-home publication;
- weakens expectations, unsupported classifications, runner behavior, timeout
  policy, proof-log policy, CTest registration, or external test contracts;
- claims progress through helper renames, diagnostic rewrites,
  classification-only notes, or narrower proof selection without changing
  generated publication behavior;
- broadens back into indexed aggregate selected-address/writeback, recursive
  call preservation, block label ordering, unsigned div/rem publication,
  scalar cast publication, return publication, variadic/byval/HFA aggregate
  publication, or static initializer work without fresh first-bad-fact
  evidence;
- leaves generated AArch64 able to reach an external or direct call with a
  stale, unpublished, or uninitialized call argument, symbol home, local home,
  or preserved stack home when the source value is live at that boundary.
