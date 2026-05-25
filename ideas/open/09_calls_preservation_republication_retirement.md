# Calls Preservation Republication Retirement

## Goal

Separate and retire target-local preservation/republication reconstruction that
belongs to prepared call planning.

## Why This Exists

Call preservation and post-call republication are high-risk because they decide
which values survive clobbers and where later consumers should reload them
from. These decisions must be prepared facts, not late AArch64 guesses.

## In Scope

- Audit preservation and republication paths used by AArch64 calls.
- Move reusable preservation source/destination facts into prepared records.
- Keep only target instruction emission in AArch64.
- Preserve later non-call scalar use and later call-argument reuse behavior.
- Add tests for clobbered register, stack-preserved, and cross-block cases.

## Out Of Scope

- Full register allocator redesign.
- Dispatch edge-copy cleanup outside preservation consumers.
- Assembly printer-only refactors.

## Acceptance Criteria

- AArch64 no longer reconstructs preservation/republication semantics from raw
  BIR shape or broad value-home scans.
- Prepared records explain which preserved value is republished and why.
- Existing clobber and later-use tests remain green.
- Representative c_testsuite call-preservation cases pass.

## Reviewer Reject Signals

- A patch restores broad post-call overwrite or reload behavior to make a
  narrow case pass.
- A patch loses cross-block preservation correctness.
- A patch leaves preservation authority split between prepared and AArch64 with
  no conflict rule.
- A patch downgrades tests or hides failures in diagnostics.
