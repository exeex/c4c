# AArch64 Barrier Cache Hint Builtin Intrinsic Carriers

Status: Closed
Created: 2026-05-15

Parent Context: ideas/closed/239_aarch64_intrinsic_machine_nodes.md

## Closure Note

Closed after completing the implementable DMB barrier carrier route: semantic
BIR records the no-result AArch64 `BarrierDmb` carrier with immediate operand
authority, prepared carriers preserve the DMB family, feature, operand role,
and immediate value without inventing value homes, and AArch64 dispatch plus
machine-printer tests prove the complete carrier remains non-selected.

The non-DMB scope from the original idea is intentionally split to
`ideas/open/243_aarch64_cache_hint_builtin_intrinsic_carriers.md` because
cache-maintenance, pause/hint, and builtin-address still need separate carrier
authority and builtin-address overlaps address-materialization/TLS ownership.

## Goal

Define structured semantic and prepared carrier authority for accepted AArch64
barrier, cache-maintenance, pause/hint, and builtin-address intrinsic families
before any target machine selection or printer support claims them as
supported.

## Why This Exists

The AArch64 intrinsic machine-node route completed the families that already
had carrier authority: scalar `fabs`, CRC32W, vector load, and vector add. The
remaining barrier/cache/pause-hint/builtin-address families are still blocked:
there are no complete BIR intrinsic family facts plus prepared intrinsic
carriers for them.

Atomic fence lowering and prepared address materialization are separate
structured routes. They are not enough authority to select or print these
families as intrinsic machine nodes without an explicit carrier contract.

## In Scope

- Define BIR or equivalent semantic facts for accepted barrier,
  cache-maintenance, pause/hint, and builtin-address intrinsic cases.
- Preserve feature, operation, operand, result, side-effect, memory-ordering,
  address-space, immediate, and register/home authority needed by later
  AArch64 selection.
- Publish prepared intrinsic carriers that can distinguish complete supported
  representatives from incomplete, malformed, target-mismatched, or unsupported
  calls.
- Add diagnostics and proof surfaces showing complete carrier fields without
  implying selected-machine support.
- Add tests for at least one accepted representative per completed family and
  nearby fail-closed cases.

## Out Of Scope

- Selecting or printing AArch64 machine records for these families.
- Replacing existing atomic fence, inline asm, ordinary call, frame lowering,
  binary128, or address materialization routes.
- Treating x86-only intrinsic names as supported on AArch64.
- Emitting assembly text from intrinsic spelling, archived scratch registers,
  or generic call plans.

## Acceptance Criteria

- Accepted barrier/cache/pause-hint/builtin-address cases have explicit
  semantic family and operation facts rather than name-only recognition.
- Prepared intrinsic carriers preserve the complete facts needed by a later
  AArch64 selector, including side effects and register/home authority.
- Incomplete, malformed, unsupported, target-mismatched, and missing-authority
  cases remain diagnostic-only or missing rather than producing selected
  machine records.
- Tests prove complete carrier representatives and fail-closed neighbors.
- A later AArch64 machine-node route can consume these carriers without
  recovering semantics from intrinsic names, text output, or unrelated carrier
  systems.

## Reviewer Reject Signals

- A diff selects or prints barrier/cache/pause-hint/builtin-address AArch64
  instructions before complete semantic and prepared intrinsic carrier facts
  exist.
- A diff treats atomic fence carriers or prepared address materialization as
  intrinsic-family authority without defining the missing carrier contract.
- A diff relies on intrinsic-name matching, archived scratch-register
  conventions, generic call plans, or fixture-shaped shortcuts as proof of
  support.
- A diff downgrades unsupported-path tests, weakens diagnostics, or claims
  capability progress through expectation rewrites only.
- A diff broadly rewrites unrelated backend routes instead of establishing the
  specific carrier facts this idea owns.
