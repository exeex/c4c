# Calls Argument Sources Retirement

## Goal

Retire or shrink AArch64 `calls_argument_sources.cpp` by moving source choice
authority to prepared call plans.

## Why This Exists

`calls_argument_sources.cpp` exists because target-local emission still needs
to recover where a call argument should come from. Once
`PreparedCallArgumentSourceSelection` is complete, most of that recovery should
be deleted or reduced to target operand construction.

## In Scope

- Audit every public helper in `calls_argument_sources.cpp`.
- Move source-choice logic to prepared planning or prepared lookups.
- Keep AArch64-only operand construction local when necessary.
- Delete helpers whose only job is reconstructing prepared source facts.
- Update `calls.hpp` to expose a smaller calls surface.

## Out Of Scope

- Reworking preservation republication.
- Dispatch materialization cleanup.
- Unrelated memory/prologue/variadic lowering changes.

## Acceptance Criteria

- The argument-source helper surface is smaller and emission-oriented.
- Prepared plans carry the facts formerly recovered target-locally.
- Build metadata and includes are updated if files are retired.
- Focused call argument tests and representative backend tests pass.

## Completion Note

Closed after `calls_argument_sources.cpp` retirement and full-suite close-gate
validation. Matching canonical regression logs recorded 3410/3410 passing tests
before and after, and the regression guard passed with no new failures.

## Reviewer Reject Signals

- A patch only moves helpers between AArch64 files.
- A patch duplicates the same source-selection logic in prepared and AArch64.
- A patch removes diagnostics without replacing them at the prepared layer.
- A patch weakens call argument tests.
