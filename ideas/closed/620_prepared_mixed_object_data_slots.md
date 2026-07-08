# Prepared Mixed Object Data Slots

Status: Closed
Type: Implementation
Parent: `ideas/open/608_prepared_global_data_authority.md`
Related:
- `ideas/open/608_prepared_global_data_authority.md`
- `ideas/open/609_rv64_global_data_consumer.md`
Owning Layer: prepared/global authority
Queue Order: 7.1
Prerequisites: accepted relocation-only selected object-data authority from `608`
Proof Surface: mixed selected global object-data rows with ordinary bytes and relocation slots

## Goal

Teach prepared object data to represent mixed initialized bytes and relocation
slots coherently, including relocation slot offsets and target identity, before
RV64 object emission consumes those facts.

## Why This Exists

`608` found that relocation-only pointer object data can publish prepared
authority, but mixed aggregate rows such as `src/20010924-1.c` still report
`unsupported_but_coherent` with `emitted_byte_count=0` and
`zero_fill_byte_count=0`. A narrow producer/schema experiment did not move the
allowlist, which means the current `PreparedGlobalObjectData` surface cannot
prove mixed emitted bytes plus relocation slots safely.

## In Scope

- Prepared object-data schema for relocation slots with byte offsets and target
  identity.
- Producer logic that populates ordinary emitted byte spans and relocation
  slots from BIR global initializer evidence.
- Prepared contract verification for coherent mixed byte-plus-relocation
  records.
- Diagnostic preservation for incomplete, contradictory, or unsupported mixed
  object data.

## Out Of Scope

- RV64 relocation-record emission, byte emission, symbol materialization, or
  access-width policy.
- Prepared global memory facts and direct global-symbol base-plus-offset
  authority already parked in `608`.
- BIR initializer bootstrap, unsupported-marker policy changes, expectation
  rewrites, allowlist changes, timeout/runtime/accounting changes, or
  testcase-shaped shortcuts.

## Acceptance Criteria

- At least one mixed selected object-data row moves past the prepared selected
  object-data contract stop for a semantic prepared-fact reason.
- The prepared facts include ordinary emitted bytes plus relocation slots with
  explicit slot offsets and target identity.
- Neighboring mixed rows either move for the same rule or retain precise
  fail-closed diagnostics.
- Remaining RV64 relocation-record and object emission diagnostics stay owned
  by `609`.

## Reviewer Reject Signals

- Reject marking mixed aggregate object data coherent without both emitted
  byte spans and relocation slot offsets plus target identity.
- Reject reconstructing relocation records, bytes, or target symbols in RV64 as
  progress for this idea.
- Reject testcase-name matching for `src/20010924-1.c` or its neighbors.
- Reject helper-only schema or publication changes that leave rows at the exact
  prepared selected object-data contract stop.
- Reject expectation, unsupported-marker, allowlist, timeout, runtime,
  accounting, or diagnostic wording changes claimed as capability progress.
- Reject broad rewrites of BIR initializer bootstrap or RV64 object emission
  outside the prepared object-data fact boundary.

## Completion Note

Closed after the prepared object-data route gained explicit relocation slots
and mixed byte-plus-relocation producer population. Step 3 moved
`src/20010924-1.c` from the prepared selected object-data contract
`unsupported_but_coherent` stop to the later RV64 relocation-record diagnostic:
`RV64 object route cannot emit prepared relocation object data without
relocation records`.

Neighboring mixed rows `src/pr61517.c`, `src/pr57877.c`, `src/pr57860.c`, and
`src/20030224-2.c` remained fail-closed at the prepared selected object-data
contract stop. Remaining RV64 relocation-record, object emission, symbol
materialization, and access-width work is handed off to
`ideas/open/609_rv64_global_data_consumer.md`.
