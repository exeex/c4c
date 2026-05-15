# AArch64 Cache Hint Builtin Intrinsic Carriers

Status: Closed
Created: 2026-05-15
Closed: 2026-05-15

Parent Context: ideas/closed/242_aarch64_barrier_cache_hint_builtin_intrinsic_carriers.md

## Closure Summary

Closed after adding carrier-only support for `llvm.aarch64.dc.cvau` and
`llvm.aarch64.hint(i32 1)`, with BIR semantic facts, prepared carrier
visibility, fail-closed tests, and dispatch/printer boundary tests proving the
new carriers are not selected or printed by this route.

Builtin-address scope was concluded as an ownership boundary: no concrete
non-materialization representative currently belongs to this intrinsic-carrier
route. Existing direct, GOT, string, label, and TLS address cases remain owned
by address-materialization or TLS materialization routes.

Closure validation used matching `^backend_` before/after logs with 139/139
tests passing and no new failures.

## Goal

Define structured semantic and prepared carrier authority for accepted AArch64
cache-maintenance, pause/hint, and builtin-address intrinsic families before
any target machine selection or printer support claims them as supported.

## Why This Exists

The barrier/cache/hint/builtin carrier route completed only the DMB barrier
packet. Cache-maintenance, pause/hint, and builtin-address representatives
remain without complete BIR intrinsic family facts plus prepared intrinsic
carriers. Builtin-address also overlaps address-materialization and TLS
authority, so it needs an explicit ownership boundary before a carrier can be
treated as machine-node-ready.

## In Scope

- Define BIR or equivalent semantic facts for accepted AArch64
  cache-maintenance, pause/hint, and builtin-address representatives.
- Preserve feature, operation, operand, result, side-effect, memory-ordering,
  address-space, immediate, and register/home authority needed by later
  AArch64 selection.
- Publish prepared intrinsic carriers that distinguish complete supported
  representatives from incomplete, malformed, target-mismatched, or unsupported
  calls.
- Add diagnostics and proof surfaces showing complete carrier fields without
  implying selected-machine support.
- Add tests for at least one accepted representative per completed family and
  nearby fail-closed cases.
- Record any builtin-address handoff that must remain owned by
  address-materialization or TLS routes instead of intrinsic carriers.

## Out Of Scope

- Selecting or printing AArch64 machine records for these families.
- Replacing existing atomic fence, inline asm, ordinary call, frame lowering,
  binary128, TLS, or address-materialization routes.
- Treating x86-only intrinsic names as supported on AArch64.
- Emitting assembly text from intrinsic spelling, archived scratch registers,
  or generic call plans.
- Reopening DMB barrier carrier support unless a regression is found.

## Acceptance Criteria

- Accepted cache-maintenance, pause/hint, and builtin-address cases have
  explicit semantic family and operation facts rather than name-only
  recognition.
- Prepared intrinsic carriers preserve the complete facts needed by a later
  AArch64 selector, including side effects and register/home authority where
  applicable.
- Incomplete, malformed, unsupported, target-mismatched, and missing-authority
  cases remain diagnostic-only or missing rather than producing selected
  machine records.
- Tests prove complete carrier representatives and fail-closed neighbors for
  every family completed by this idea.
- Builtin-address ownership is explicit: accepted cases are real intrinsic
  carrier facts, while address-materialization or TLS-owned cases remain on
  their existing routes.
- A later AArch64 machine-node route can consume these carriers without
  recovering semantics from intrinsic names, text output, or unrelated carrier
  systems.

## Reviewer Reject Signals

- A diff selects or prints cache-maintenance, pause/hint, or builtin-address
  AArch64 instructions before complete semantic and prepared intrinsic carrier
  facts exist.
- A diff treats atomic fence carriers, prepared address materialization, TLS
  metadata, or ordinary call plans as intrinsic-family authority without
  defining the missing carrier contract.
- A diff relies on intrinsic-name matching, archived scratch-register
  conventions, rendered assembly text, generic call plans, or fixture-shaped
  shortcuts as proof of support.
- A diff downgrades unsupported-path tests, weakens diagnostics, or claims
  capability progress through expectation rewrites only.
- A diff broadly rewrites unrelated backend routes instead of establishing the
  specific cache/hint/builtin carrier facts this idea owns.
- A diff keeps builtin-address behavior on the exact old failure path while
  renaming helpers or classifications as if carrier authority had been added.
