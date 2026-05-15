# F128 Full-Width Constant Carriers

Status: Closed
Created: 2026-05-15
Closed: 2026-05-15

Parent Context: ideas/open/237_aarch64_binary128_softfloat_lowering.md

## Goal

Define and expose a prepared full-width constant carrier for binary128 values
so target backends can lower F128 constants from structured 16-byte payload
facts instead of inventing target-local immediate shortcuts.

## Why This Exists

The AArch64 binary128 transport route can now consume named prepared F128
carriers and optional 16-byte memory operands, but F128 literals do not yet
have equivalent full-width carrier authority. Current BIR immediate fields only
carry 64-bit immediate data, and prepared F128 carrier construction is derived
from named regalloc or storage facts. AArch64 constant transport therefore has
no source of truth for both 64-bit halves or equivalent 16-byte payload
provenance.

This dependency should be solved before any backend claims structured F128
constant transport support.

## In Scope

- Define how binary128 constants are represented as full 16-byte payload facts
  before target instruction selection.
- Preserve both 64-bit halves or an equivalent exact 16-byte provenance record
  through BIR or prepared state.
- Expose that constant carrier to backend selection through structured data,
  not rendered assembly text or scalar floating approximations.
- Add focused tests proving constant payload preservation and fail-closed
  diagnostics when the full-width fact is missing.
- Keep the carrier target-neutral when practical, while allowing AArch64 to
  consume it for the parent binary128 lowering route.

## Out Of Scope

- Do not add AArch64 constant assembly printing before a structured full-width
  constant carrier exists.
- Do not lower F128 constants through `F64`, scalar immediate bits, or a
  single 64-bit payload.
- Do not add arithmetic, comparison, cast, or helper-call lowering in this
  initiative except where needed to prove the constant carrier survives to a
  normal backend consumer.
- Do not rewrite unrelated i128, scalar floating, atomic, intrinsic, or
  inline-assembly routes.

## Acceptance Criteria

- Binary128 constants have a structured full-width carrier that records both
  halves or equivalent exact 16-byte payload provenance.
- Prepared or shared carrier state exposes that fact to target selection
  without requiring target-local reconstruction from text.
- Missing or partial F128 constant carrier facts produce explicit diagnostics
  instead of falling through to scalar or unsupported lowering.
- A focused backend or prepared-record proof demonstrates that AArch64 can
  distinguish a valid full-width F128 constant carrier from unsupported
  constant transport.
- Existing scalar FP and i128 immediate behavior remains unchanged.

## Completion Notes

The active runbook completed all five steps. Binary128 constants now have a
target-neutral structured payload carrier, prepared state transports that
low/high payload through rematerializable F128 constant identities, and AArch64
call-boundary selection consumes only complete structured carrier facts.
Missing source ids, missing payloads, and scalar-only literal facts remain
fail-closed with explicit diagnostics.

The final dependency proof routed semantic BIR F128 immediate call arguments
through prepared rematerialized constant moves into the existing AArch64
call-boundary consumer. No AArch64 constant assembly-printing support was
added; that remains outside this idea's scope.

Close-time regression guard used the executor's matching backend before/after
logs:

`set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } 2>&1 | tee test_after.log`

Result: 139/139 backend tests passed before and after; the non-decreasing
guard mode passed for this lifecycle-only close.

## Reviewer Reject Signals

- Reject any slice that claims F128 constant progress by using only
  `immediate`, `immediate_bits`, `double`, `F64`, or any other single-lane
  64-bit payload for a binary128 value.
- Reject named-testcase shortcuts, fixture-string matching, or backend-local
  constants that bypass a structured prepared or BIR carrier.
- Reject expectation downgrades, `unsupported` rewrites, or weaker test
  contracts used to claim progress without adding full-width constant payload
  authority.
- Reject helper renames, classification-only changes, or printer text changes
  presented as constant-carrier support when no exact 16-byte payload fact is
  available to instruction selection.
- Reject broad rewrites of unrelated arithmetic, intrinsic, atomic, inline
  assembly, scalar FP, or i128 routes unless the diff proves identical existing
  behavior and the change is necessary for the F128 constant carrier.
- Reject abstractions that preserve the old failure mode where AArch64
  selection cannot tell the low and high halves, or equivalent full 16-byte
  provenance, of an F128 constant.
