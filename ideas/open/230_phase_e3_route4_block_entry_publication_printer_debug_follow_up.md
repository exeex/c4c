# 230 Phase E3 Route 4 block-entry publication printer/debug follow-up

## Goal

Augment or replace one Route 4 `block_entry_publication` available-register
printer/debug row with Route 4 publication attribution under prepared agreement
and fail-closed fallback.

## Why This Exists

The Route 4 block-entry publication closure proved that route attribution can
reproduce one available-register prepared value-location printer row under
prepared agreement. Phase E3 accepted this exact row for a later follow-up while
retaining prepared publication mechanics, wrapper behavior, CLI dump scope, row
text, and expected strings.

## In Scope

- One `block_entry_publication` available-register printer/debug row.
- Positive Route 4 publication attribution under prepared agreement.
- Absent route evidence, wrong-reference, mismatch, duplicate-reference
  fallback, and prepared publication mechanics retention.
- Wrapper no-change proof for x86/riscv/AArch64 compatibility output where the
  row is visible.
- Stable row text, printer/debug sections, CLI dump scope, and expected strings.

## Out Of Scope

- Broad printer/debug, CLI dump, publication-mechanics, block-order, wrapper, or
  prepared publication replacement.
- Wrapper migration or cross-target compatibility work owned by E4.
- Row-text relabeling, expected-string rewrites, baseline refreshes,
  unsupported downgrades, or target-policy migration.
- Aggregate prepared retirement, draft 155, E5, or broad diagnostic/oracle
  replacement.

## Acceptance Criteria

- The available-register row uses Route 4 publication attribution only after
  prepared agreement.
- Absent, wrong-reference, duplicate-reference, mismatch, and fallback paths
  preserve prepared printer/debug authority.
- Wrapper output, CLI dump scope, row text, and expected strings remain
  byte-stable.
- Proof covers nearby same-feature publication rows and negative cases.
- The slice does not move block-order, wrapper, or emitted-output policy into
  route diagnostic ownership.

## Reviewer Reject Signals

- The change matches one `block_entry_publication` output string or fixture
  instead of using Route 4 publication attribution under agreement.
- It changes row text, printer/debug section names, CLI dump scope, wrapper
  output, or expected strings to make proof pass.
- It weakens fallback for absent route, wrong-reference, duplicate-reference,
  mismatch, or prepared-only cases.
- It claims broad printer/debug, block-order, publication-mechanics, or wrapper
  replacement from this one row.
- It leaves the old prepared printer decision under a renamed route/debug
  helper without proving Route 4 attribution.
