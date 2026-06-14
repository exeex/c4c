# 255 Phase F3 prepared private pass-context metadata gate

## Idea Type

private-pass-context demotion.

## Goal

Gate a narrow private-pass-context demotion for
`PreparedBirModule::route`, `PreparedBirModule::invariants`,
`PreparedBirModule::completed_phases`, and `PreparedBirModule::notes`.

## Why This Exists

Phase F2 identified these four fields as the only near-term private
pass-context candidates. They are not deletion-ready, and demotion is safe only
after printer/status/debug compatibility is preserved and no target-facing
public consumer depends on direct field access.

## In Scope

- Confirm all current consumers of `route`, `invariants`,
  `completed_phases`, and `notes`.
- Preserve prepared printer route output, invariant output,
  completed-phase text/status rows, notes text/status rows, absent-note
  fallback behavior, and any debug/status strings.
- Define the smallest private accessor or adapter shape needed to keep
  compatibility without exposing public prepared semantic authority.
- Prove no x86/riscv target-facing public consumer requires direct field
  access, or mark the specific field blocked.

## Out Of Scope

- Deleting these fields.
- Demoting `liveness` or any policy-bearing field.
- Reclassifying metadata as BIR semantic fact or target policy.
- Rewriting prepared printer/status/debug text or weakening diagnostics.
- Broad `PreparedBirModule` aggregate retirement.

## Acceptance Criteria

- Each of the four fields is accepted for private-pass-context demotion,
  retained as public compatibility, or blocked with a named consumer.
- Any accepted demotion preserves printer/status/debug behavior exactly and
  keeps fail-closed handling for invalid or mismatched state.
- `liveness` remains explicitly outside this packet.

## Reviewer Reject Signals

- Reject named-case shortcuts that check only one printer row or one field
  access while claiming all metadata fields are private-pass-context ready.
- Reject unsupported expectation downgrades, weaker printer/status/debug
  contracts, or absent-note fallback weakening without explicit approval.
- Reject helper renames, field renames, or classification-only notes claimed
  as demotion progress.
- Reject broad `PreparedBirModule` retirement, liveness, regalloc, target
  policy, or aggregate handoff rewrites outside these four metadata fields.
- Reject any route that keeps the exact old public field dependency but hides
  it behind a private accessor name.
